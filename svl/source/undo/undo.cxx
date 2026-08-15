/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <svl/undo.hxx>
#include <svl/docskill_undo_log.hxx>

#include <osl/mutex.hxx>
#include <sal/log.hxx>
#include <comphelper/flagguard.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/long.hxx>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <tools/XmlWriter.hxx>
#include <boost/property_tree/json_parser.hpp>
#include <unotools/datetime.hxx>
#include <rtl/string.hxx>
#include <rtl/ustrbuf.hxx>
#if defined(_WIN32)
#include <windows.h>
#endif

#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <map>
#include <cstring>
#include <limits.h>
#include <algorithm>

namespace
{
class SfxMarkedUndoContext final : public SfxUndoContext
{
public:
    SfxMarkedUndoContext(SfxUndoManager& manager, UndoStackMark mark)
    {
        m_offset = manager.RemoveMark(mark);
        size_t count = manager.GetUndoActionCount();
        if (m_offset < count)
            m_offset = count - m_offset - 1;
        else
            m_offset = std::numeric_limits<size_t>::max();
    }
    size_t GetUndoOffset() override { return m_offset; }

private:
    size_t m_offset;
};
}

SfxRepeatTarget::~SfxRepeatTarget()
{
}


SfxUndoContext::~SfxUndoContext()
{
}


SfxUndoAction::~SfxUndoAction() COVERITY_NOEXCEPT_FALSE
{
}


SfxUndoAction::SfxUndoAction()
    : m_aDateTime(DateTime::SYSTEM)
{
    m_aDateTime.ConvertToUTC();
}


bool SfxUndoAction::Merge( SfxUndoAction * )
{
    return false;
}


OUString SfxUndoAction::GetComment() const
{
    return OUString();
}


OUString SfxUndoAction::GetObjDescription() const
{
    return OUString();
}


void SfxUndoAction::SetComment(const OUString& /*rStr*/)
{
}


void SfxUndoAction::SetObjDescription(const OUString& /*rStr*/)
{
}


css::uno::Sequence<css::beans::PropertyValue> SfxUndoAction::GetActionDetails() const
{
    return css::uno::Sequence<css::beans::PropertyValue>();
}

#if defined(_WIN32)
namespace
{
struct ActionDetailsSehArg
{
    SfxUndoAction* pAction;
    css::uno::Sequence<css::beans::PropertyValue>* pOut;
};

void lcl_ActionDetailsTrampoline(void* pArg)
{
    auto* p = static_cast<ActionDetailsSehArg*>(pArg);
    *p->pOut = p->pAction->GetActionDetails();
}

int lcl_SehInvoke(void (*pFn)(void*), void* pArg)
{
    __try
    {
        pFn(pArg);
        return 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }
}

css::uno::Sequence<css::beans::PropertyValue>
lcl_SafeGetActionDetails(SfxUndoAction* pAction)
{
    // Partial DLL rebuilds may leave Writer undo objects on older vtables;
    // calling a newly appended virtual then AVs. Swallow and return empty.
    css::uno::Sequence<css::beans::PropertyValue> aOut;
    ActionDetailsSehArg aArg{ pAction, &aOut };
    if (!lcl_SehInvoke(&lcl_ActionDetailsTrampoline, &aArg))
    {
        docskill::undo_log::writeLine(
            "L2-svl-seh",
            u"GetActionDetails AV/SEH — vtable mismatch? falling back to dumpAsXml"_ustr);
        return {};
    }
    return aOut;
}
}
#else
namespace
{
css::uno::Sequence<css::beans::PropertyValue>
lcl_SafeGetActionDetails(SfxUndoAction* pAction)
{
    return pAction->GetActionDetails();
}
}
#endif

namespace
{
css::beans::PropertyValue lcl_PV(const OUString& rName, const css::uno::Any& rValue)
{
    css::beans::PropertyValue aPV;
    aPV.Name = rName;
    aPV.Value = rValue;
    return aPV;
}

void lcl_CollectWhitelistItem(xmlNodePtr pNode, bool bFrom,
                              std::map<OUString, css::uno::Any>& rOut)
{
    if (!pNode || pNode->type != XML_ELEMENT_NODE || !pNode->name)
        return;
    const char* pName = reinterpret_cast<const char*>(pNode->name);
    auto attr = [&](const char* key) -> OString {
        xmlChar* p = xmlGetProp(pNode, BAD_CAST(key));
        if (!p)
            return {};
        OString a(reinterpret_cast<const char*>(p));
        xmlFree(p);
        return a;
    };
    if (std::strcmp(pName, "SvxWeightItem") == 0)
    {
        const OString aVal = attr("value");
        if (!aVal.isEmpty())
        {
            // dumpAsXml stores FontWeight enum ordinal (WEIGHT_NORMAL=5,
            // WEIGHT_SEMIBOLD=7, WEIGHT_BOLD=8), not UNO CharWeight (100/150).
            // Match SwUndoAttr::GetActionDetails: bold if heavier than NORMAL.
            const sal_Int32 n = aVal.toInt32();
            const bool bBold = n > 5; // WEIGHT_NORMAL
            const OUString aKey = bFrom ? u"boldFrom"_ustr : u"boldTo"_ustr;
            auto it = rOut.find(aKey);
            if (it == rOut.end())
                rOut[aKey] <<= bBold;
            else
            {
                bool bPrev = false;
                it->second >>= bPrev;
                it->second <<= (bPrev || bBold);
            }
        }
    }
    else if (std::strcmp(pName, "SvxFontItem") == 0)
    {
        const OString aFam = attr("familyName");
        if (!aFam.isEmpty())
            rOut[bFrom ? u"fontFrom"_ustr : u"fontTo"_ustr]
                <<= OStringToOUString(aFam, RTL_TEXTENCODING_UTF8);
    }
    else if (std::strcmp(pName, "SvxFontHeightItem") == 0)
    {
        const OString aH = attr("height");
        if (!aH.isEmpty())
        {
            // Item height is twips; UNO CharHeight is points.
            rOut[bFrom ? u"sizeFrom"_ustr : u"sizeTo"_ustr]
                <<= static_cast<double>(aH.toInt32()) / 20.0;
        }
    }
}

void lcl_WalkXml(xmlNodePtr pNode, bool bInHistory, bool bInAttrSet,
                 std::map<OUString, css::uno::Any>& rOut, OUString& rKind,
                 bool& rHaveRange)
{
    for (; pNode; pNode = pNode->next)
    {
        if (pNode->type != XML_ELEMENT_NODE || !pNode->name)
            continue;
        const char* pName = reinterpret_cast<const char*>(pNode->name);
        if (std::strcmp(pName, "SwUndoAttr") == 0)
        {
            rKind = u"ApplyAttributes"_ustr;
            if (!rHaveRange)
            {
                auto get = [&](const char* key) -> sal_Int32 {
                    xmlChar* p = xmlGetProp(pNode, BAD_CAST(key));
                    if (!p)
                        return -1;
                    const sal_Int32 n = OString(reinterpret_cast<const char*>(p)).toInt32();
                    xmlFree(p);
                    return n;
                };
                const sal_Int32 nSttNode = get("start-node");
                const sal_Int32 nEndNode = get("end-node");
                const sal_Int32 nSttContent = get("start-content");
                const sal_Int32 nEndContent = get("end-content");
                if (nSttNode >= 0)
                {
                    rOut[u"StartNode"_ustr] <<= nSttNode;
                    rOut[u"EndNode"_ustr] <<= (nEndNode >= 0 ? nEndNode : nSttNode);
                    if (nSttContent >= 0)
                        rOut[u"StartContent"_ustr] <<= nSttContent;
                    if (nEndContent >= 0)
                        rOut[u"EndContent"_ustr] <<= nEndContent;
                    rHaveRange = true;
                }
            }
        }
        else if (std::strcmp(pName, "SwUndoFormatAttr") == 0)
            rKind = u"FormatAttributes"_ustr;
        else if (std::strcmp(pName, "SwHistorySetText") == 0)
        {
            if (!rHaveRange)
            {
                auto get = [&](const char* key) -> sal_Int32 {
                    xmlChar* p = xmlGetProp(pNode, BAD_CAST(key));
                    if (!p)
                        return -1;
                    const sal_Int32 n = OString(reinterpret_cast<const char*>(p)).toInt32();
                    xmlFree(p);
                    return n;
                };
                const sal_Int32 nNode = get("node-index");
                const sal_Int32 nStart = get("start");
                const sal_Int32 nEnd = get("end");
                if (nNode >= 0)
                {
                    rOut[u"StartNode"_ustr] <<= nNode;
                    rOut[u"EndNode"_ustr] <<= nNode;
                    if (nStart >= 0)
                        rOut[u"StartContent"_ustr] <<= nStart;
                    if (nEnd >= 0)
                        rOut[u"EndContent"_ustr] <<= nEnd;
                    rHaveRange = true;
                }
            }
            lcl_WalkXml(pNode->children, true, false, rOut, rKind, rHaveRange);
            continue;
        }
        else if (std::strcmp(pName, "SfxItemSet") == 0)
        {
            lcl_WalkXml(pNode->children, bInHistory, true, rOut, rKind, rHaveRange);
            continue;
        }
        else if (bInAttrSet || bInHistory)
        {
            lcl_CollectWhitelistItem(pNode, bInHistory, rOut);
        }
        lcl_WalkXml(pNode->children, bInHistory, bInAttrSet, rOut, rKind, rHaveRange);
    }
}

css::uno::Sequence<css::beans::PropertyValue>
lcl_DetailsFromDumpAsXml(SfxUndoAction* pAction)
{
    // ABI-safe path: dumpAsXml already existed on Writer undo objects before
    // GetActionDetails was added. Works with older swlo.dll without vtable growth.
    xmlBufferPtr pBuf = xmlBufferCreate();
    if (!pBuf)
        return {};
    xmlTextWriterPtr pWriter = xmlNewTextWriterMemory(pBuf, 0);
    if (!pWriter)
    {
        xmlBufferFree(pBuf);
        return {};
    }
    xmlTextWriterStartDocument(pWriter, nullptr, nullptr, nullptr);
    pAction->dumpAsXml(pWriter);
    xmlTextWriterEndDocument(pWriter);
    xmlFreeTextWriter(pWriter);

    const xmlChar* pContent = xmlBufferContent(pBuf);
    const int nSize = xmlBufferLength(pBuf);
    if (!pContent || nSize <= 0)
    {
        xmlBufferFree(pBuf);
        return {};
    }
    xmlDocPtr pDoc = xmlReadMemory(reinterpret_cast<const char*>(pContent), nSize,
                                   "undo.xml", nullptr, XML_PARSE_NONET);
    xmlBufferFree(pBuf);
    if (!pDoc)
        return {};

    std::map<OUString, css::uno::Any> aMap;
    OUString aKind;
    bool bHaveRange = false;
    lcl_WalkXml(xmlDocGetRootElement(pDoc), false, false, aMap, aKind, bHaveRange);
    xmlFreeDoc(pDoc);

    if (aKind.isEmpty() && aMap.empty())
    {
        docskill::undo_log::writeLine(
            "L2-svl-xml", u"dumpAsXml parse produced empty whitelist"_ustr);
        return {};
    }

    std::vector<css::beans::PropertyValue> aProps;
    if (!aKind.isEmpty())
        aProps.push_back(lcl_PV(u"Kind"_ustr, css::uno::Any(aKind)));
    // Stable order for journal consumers
    static const OUString aKeys[] = {
        u"StartNode"_ustr, u"EndNode"_ustr, u"StartContent"_ustr, u"EndContent"_ustr,
        u"boldFrom"_ustr,  u"boldTo"_ustr,  u"fontFrom"_ustr,     u"fontTo"_ustr,
        u"sizeFrom"_ustr,  u"sizeTo"_ustr,  u"alignFrom"_ustr,    u"alignTo"_ustr,
    };
    for (const OUString& rKey : aKeys)
    {
        auto it = aMap.find(rKey);
        if (it != aMap.end())
            aProps.push_back(lcl_PV(rKey, it->second));
    }
    css::uno::Sequence<css::beans::PropertyValue> aOut(
        aProps.data(), static_cast<sal_Int32>(aProps.size()));
    docskill::undo_log::logDetails("L2-svl-xml", u"dumpAsXml->Details"_ustr, aOut);
    return aOut;
}
}

ViewShellId SfxUndoAction::GetViewShellId() const
{
    return ViewShellId(-1);
}

const DateTime& SfxUndoAction::GetDateTime() const
{
    return m_aDateTime;
}

OUString SfxUndoAction::GetRepeatComment(SfxRepeatTarget&) const
{
    return GetComment();
}


void SfxUndoAction::Undo()
{
    // These are only conceptually pure virtual
    assert(!"pure virtual function called: SfxUndoAction::Undo()");
}


void SfxUndoAction::UndoWithContext( SfxUndoContext& )
{
    Undo();
}


void SfxUndoAction::Redo()
{
    // These are only conceptually pure virtual
    assert(!"pure virtual function called: SfxUndoAction::Redo()");
}


void SfxUndoAction::RedoWithContext( SfxUndoContext& )
{
    Redo();
}


void SfxUndoAction::Repeat(SfxRepeatTarget&)
{
    // These are only conceptually pure virtual
    assert(!"pure virtual function called: SfxUndoAction::Repeat()");
}


bool SfxUndoAction::CanRepeat(SfxRepeatTarget&) const
{
    return true;
}

void SfxUndoAction::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    tools::XmlWriter aWriter(pWriter);
    aWriter.startElement("SfxUndoAction");
    aWriter.attribute("ptr", reinterpret_cast<sal_IntPtr>(this));
    aWriter.attribute("symbol", typeid(*this).name());
    aWriter.attribute("comment", GetComment());
    aWriter.attribute("viewShellId", sal_Int32(GetViewShellId()));
    aWriter.attribute("dateTime", utl::toISO8601(m_aDateTime.GetUNODateTime()));
    aWriter.endElement();
}

std::unique_ptr<SfxUndoAction> SfxUndoArray::Remove(int idx)
{
    auto ret = std::move(maUndoActions[idx].pAction);
    maUndoActions.erase(maUndoActions.begin() + idx);
    return ret;
}

void SfxUndoArray::Remove( size_t i_pos, size_t i_count )
{
    maUndoActions.erase(maUndoActions.begin() + i_pos, maUndoActions.begin() + i_pos + i_count);
}

void SfxUndoArray::Insert( std::unique_ptr<SfxUndoAction> i_action, size_t i_pos )
{
    maUndoActions.insert( maUndoActions.begin() + i_pos, MarkedUndoAction(std::move(i_action)) );
}

typedef ::std::vector< SfxUndoListener* >   UndoListeners;

struct SfxUndoManager_Data
{
    ::osl::Mutex    aMutex;
    SfxUndoArray    maUndoArray;
    SfxUndoArray*   pActUndoArray;

    sal_Int32       mnMarks;
    sal_Int32       mnEmptyMark;
    bool            mbUndoEnabled;
    bool            mbDoing;
    bool            mbClearUntilTopLevel;
    bool            mbEmptyActions;
    std::optional<bool> moNeedsClearRedo; // holds a requested ClearRedo until safe to clear stack

    UndoListeners   aListeners;

    explicit SfxUndoManager_Data( size_t i_nMaxUndoActionCount )
        :maUndoArray( i_nMaxUndoActionCount )
        ,pActUndoArray( nullptr )
        ,mnMarks( 0 )
        ,mnEmptyMark(MARK_INVALID)
        ,mbUndoEnabled( true )
        ,mbDoing( false )
        ,mbClearUntilTopLevel( false )
        ,mbEmptyActions( true )
    {
        pActUndoArray = &maUndoArray;
    }

    // Copy assignment is forbidden and not implemented.
    SfxUndoManager_Data (const SfxUndoManager_Data &) = delete;
    SfxUndoManager_Data & operator= (const SfxUndoManager_Data &) = delete;
};

namespace svl::undo::impl
{
    class LockGuard
    {
    public:
        explicit LockGuard( SfxUndoManager& i_manager )
            :m_manager( i_manager )
        {
            m_manager.ImplEnableUndo_Lock( false );
        }

        ~LockGuard()
        {
            m_manager.ImplEnableUndo_Lock( true );
        }

    private:
        SfxUndoManager& m_manager;
    };

    typedef void ( SfxUndoListener::*UndoListenerVoidMethod )();
    typedef void ( SfxUndoListener::*UndoListenerStringMethod )( const OUString& );

    namespace {

    struct NotifyUndoListener
    {
        explicit NotifyUndoListener( UndoListenerVoidMethod i_notificationMethod )
            :m_notificationMethod( i_notificationMethod )
            ,m_altNotificationMethod( nullptr )
        {
        }

        NotifyUndoListener( UndoListenerStringMethod i_notificationMethod, OUString i_actionComment )
            :m_notificationMethod( nullptr )
            ,m_altNotificationMethod( i_notificationMethod )
            ,m_sActionComment(std::move( i_actionComment ))
        {
        }

        bool is() const
        {
            return ( m_notificationMethod != nullptr ) || ( m_altNotificationMethod != nullptr );
        }

        void operator()( SfxUndoListener* i_listener ) const
        {
            assert( is() && "NotifyUndoListener: this will crash!" );
            if ( m_altNotificationMethod != nullptr )
            {
                ( i_listener->*m_altNotificationMethod )( m_sActionComment );
            }
            else
            {
                ( i_listener->*m_notificationMethod )();
            }
        }

    private:
        UndoListenerVoidMethod      m_notificationMethod;
        UndoListenerStringMethod    m_altNotificationMethod;
        OUString                    m_sActionComment;
    };

    }

    class UndoManagerGuard
    {
    public:
        explicit UndoManagerGuard( SfxUndoManager_Data& i_managerData )
            :m_rManagerData( i_managerData )
            ,m_aGuard( i_managerData.aMutex )
        {
        }

        ~UndoManagerGuard();

        auto clear() { return osl::ResettableMutexGuardScopedReleaser(m_aGuard); }

        void cancelNotifications()
        {
            m_notifiers.clear();
        }

        /** marks the given Undo action for deletion

            The Undo action will be put into a list, whose members will be deleted from within the destructor of the
            UndoManagerGuard. This deletion will happen without the UndoManager's mutex locked.
        */
        void    markForDeletion( std::unique_ptr<SfxUndoAction> i_action )
        {
            // remember
            assert ( i_action );
            m_aUndoActionsCleanup.emplace_back( std::move(i_action) );
        }

        /** schedules the given SfxUndoListener method to be called for all registered listeners.

            The notification will happen after the Undo manager's mutex has been released, and after all pending
            deletions of Undo actions are done.
        */
        void    scheduleNotification( UndoListenerVoidMethod i_notificationMethod )
        {
            m_notifiers.emplace_back( i_notificationMethod );
        }

        void    scheduleNotification( UndoListenerStringMethod i_notificationMethod, const OUString& i_actionComment )
        {
            m_notifiers.emplace_back( i_notificationMethod, i_actionComment );
        }

    private:
        SfxUndoManager_Data&                m_rManagerData;
        ::osl::ResettableMutexGuard         m_aGuard;
        ::std::vector< std::unique_ptr<SfxUndoAction> > m_aUndoActionsCleanup;
        ::std::vector< NotifyUndoListener > m_notifiers;
    };

    UndoManagerGuard::~UndoManagerGuard()
    {
        // copy members
        UndoListeners aListenersCopy( m_rManagerData.aListeners );

        // release mutex
        m_aGuard.clear();

        // delete all actions
        m_aUndoActionsCleanup.clear();

        // handle scheduled notification
        for (auto const& notifier : m_notifiers)
        {
            if ( notifier.is() )
                ::std::for_each( aListenersCopy.begin(), aListenersCopy.end(), notifier );
        }
    }
}

using namespace ::svl::undo::impl;


SfxUndoManager::SfxUndoManager( size_t nMaxUndoActionCount )
    :m_xData( new SfxUndoManager_Data( nMaxUndoActionCount ) )
{
    m_xData->mbEmptyActions = !ImplIsEmptyActions();
}


SfxUndoManager::~SfxUndoManager()
{
}

void SfxUndoManager::SetUndoComment(const OUString& rComment)
{
    if (rComment.isEmpty())
        return;

    SfxUndoArray* pSfxUndoArray(m_xData->pActUndoArray);

    if (nullptr == pSfxUndoArray)
        return;

    // get to outmost Undos
    while (nullptr != pSfxUndoArray->pFatherUndoArray)
        pSfxUndoArray = pSfxUndoArray->pFatherUndoArray;

    if (pSfxUndoArray->maUndoActions.empty())
        return;

    // the outmost 1st undo should be a UndoGroup
    SfxUndoAction* pSfxUndoAction(pSfxUndoArray->GetUndoAction(0));
    if (nullptr == pSfxUndoAction)
        return;

    pSfxUndoAction->SetComment(rComment);
}

void SfxUndoManager::SetUndoComment(const OUString& rComment, const OUString& rObjDescr)
{
    if (rComment.isEmpty())
        return;

    SfxUndoArray* pSfxUndoArray(m_xData->pActUndoArray);

    if (nullptr == pSfxUndoArray)
        return;

    // get to outmost Undos
    while (nullptr != pSfxUndoArray->pFatherUndoArray)
        pSfxUndoArray = pSfxUndoArray->pFatherUndoArray;

    if (pSfxUndoArray->maUndoActions.empty())
        return;

    // the outmost 1st undo should be a UndoGroup
    SfxUndoAction* pSfxUndoAction(pSfxUndoArray->GetUndoAction(0));
    if (nullptr == pSfxUndoAction)
        return;

    // apply text replacement when given
    OUString aComment(rComment);
    if (!rObjDescr.isEmpty())
        aComment = aComment.replaceFirst("%1", rObjDescr);

    pSfxUndoAction->SetComment(aComment);
    pSfxUndoAction->SetObjDescription(rObjDescr);
}

void SfxUndoManager::EnableUndo( bool i_enable )
{
    UndoManagerGuard aGuard( *m_xData );
    ImplEnableUndo_Lock( i_enable );

}


void SfxUndoManager::ImplEnableUndo_Lock( bool const i_enable )
{
    if ( m_xData->mbUndoEnabled == i_enable )
        return;
    m_xData->mbUndoEnabled = i_enable;
}


bool SfxUndoManager::IsUndoEnabled() const
{
    UndoManagerGuard aGuard( *m_xData );
    return ImplIsUndoEnabled_Lock();
}


bool SfxUndoManager::ImplIsUndoEnabled_Lock() const
{
    return m_xData->mbUndoEnabled;
}

void SfxUndoManager::SetMaxUndoActionCount( size_t nMaxUndoActionCount )
{
    UndoManagerGuard aGuard( *m_xData );

    // Remove entries from the pActUndoArray when we have to reduce
    // the number of entries due to a lower nMaxUndoActionCount.
    // Both redo and undo action entries will be removed until we reached the
    // new nMaxUndoActionCount.

    tools::Long nNumToDelete = m_xData->pActUndoArray->maUndoActions.size() - nMaxUndoActionCount;
    while ( nNumToDelete > 0 )
    {
        size_t nPos = m_xData->pActUndoArray->maUndoActions.size();
        if ( nPos > m_xData->pActUndoArray->nCurUndoAction )
        {
            aGuard.markForDeletion( m_xData->pActUndoArray->Remove( nPos-1 ) );
            --nNumToDelete;
        }

        if ( nNumToDelete > 0 && m_xData->pActUndoArray->nCurUndoAction > 0 )
        {
            aGuard.markForDeletion( m_xData->pActUndoArray->Remove(0) );
            --m_xData->pActUndoArray->nCurUndoAction;
            --nNumToDelete;
        }

        if ( nPos == m_xData->pActUndoArray->maUndoActions.size() )
            break; // Cannot delete more entries
    }

    m_xData->pActUndoArray->nMaxUndoActions = nMaxUndoActionCount;
    ImplCheckEmptyActions();
}

size_t SfxUndoManager::GetMaxUndoActionCount() const
{
    return m_xData->pActUndoArray->nMaxUndoActions;
}

void SfxUndoManager::ImplClearCurrentLevel_NoNotify( UndoManagerGuard& i_guard )
{
    // clear array
    while ( !m_xData->pActUndoArray->maUndoActions.empty() )
    {
        size_t deletePos = m_xData->pActUndoArray->maUndoActions.size() - 1;
        i_guard.markForDeletion( m_xData->pActUndoArray->Remove( deletePos ) );
    }

    m_xData->pActUndoArray->nCurUndoAction = 0;

    m_xData->mnMarks = 0;
    m_xData->mnEmptyMark = MARK_INVALID;
    ImplCheckEmptyActions();
}


void SfxUndoManager::Clear()
{
    UndoManagerGuard aGuard( *m_xData );

    SAL_WARN_IF( ImplIsInListAction_Lock(), "svl",
        "SfxUndoManager::Clear: suspicious call - do you really wish to clear the current level?" );
    ImplClearCurrentLevel_NoNotify( aGuard );

    // notify listeners
    aGuard.scheduleNotification( &SfxUndoListener::cleared );
}


void SfxUndoManager::ClearAllLevels()
{
    UndoManagerGuard aGuard( *m_xData );
    ImplClearCurrentLevel_NoNotify( aGuard );

    if ( ImplIsInListAction_Lock() )
    {
        m_xData->mbClearUntilTopLevel = true;
    }
    else
    {
        aGuard.scheduleNotification( &SfxUndoListener::cleared );
    }
}


void SfxUndoManager::ImplClearRedo_NoLock( bool const i_currentLevel )
{
    if (IsDoing())
    {
        // cannot clear redo while undo/redo is in process. Delay ClearRedo until safe to clear.
        // (assuming if TopLevel requests a clear, it should have priority over CurrentLevel)
        if (!m_xData->moNeedsClearRedo.has_value() || i_currentLevel == TopLevel)
            m_xData->moNeedsClearRedo = i_currentLevel;
        return;
    }
    UndoManagerGuard aGuard( *m_xData );
    ImplClearRedo( aGuard, i_currentLevel );
}


void SfxUndoManager::ClearRedo()
{
    SAL_WARN_IF( IsInListAction(), "svl",
        "SfxUndoManager::ClearRedo: suspicious call - do you really wish to clear the current level?" );
    ImplClearRedo_NoLock( CurrentLevel );
}

void SfxUndoManager::Reset()
{
    UndoManagerGuard aGuard( *m_xData );

    // clear all locks
    while ( !ImplIsUndoEnabled_Lock() )
        ImplEnableUndo_Lock( true );

    // cancel all list actions
    while ( IsInListAction() )
        ImplLeaveListAction( false, aGuard );

    // clear both stacks
    ImplClearCurrentLevel_NoNotify( aGuard );

    // cancel the notifications scheduled by ImplLeaveListAction,
    // as we want to do an own, dedicated notification
    aGuard.cancelNotifications();

    // schedule notification
    aGuard.scheduleNotification( &SfxUndoListener::resetAll );
}


void SfxUndoManager::ImplClearUndo( UndoManagerGuard& i_guard )
{
    while ( m_xData->pActUndoArray->nCurUndoAction > 0 )
    {
        i_guard.markForDeletion( m_xData->pActUndoArray->Remove( 0 ) );
        --m_xData->pActUndoArray->nCurUndoAction;
    }
    ImplCheckEmptyActions();
    // TODO: notifications? We don't have clearedUndo, only cleared and clearedRedo at the SfxUndoListener
}


void SfxUndoManager::ImplClearRedo( UndoManagerGuard& i_guard, bool const i_currentLevel )
{
    SfxUndoArray* pUndoArray = ( i_currentLevel == SfxUndoManager::CurrentLevel ) ? m_xData->pActUndoArray : &m_xData->maUndoArray;

    // clearance
    while ( pUndoArray->maUndoActions.size() > pUndoArray->nCurUndoAction )
    {
        size_t deletePos = pUndoArray->maUndoActions.size() - 1;
        i_guard.markForDeletion( pUndoArray->Remove( deletePos ) );
    }

    ImplCheckEmptyActions();
    // notification - only if the top level's stack was cleared
    if ( i_currentLevel == SfxUndoManager::TopLevel )
        i_guard.scheduleNotification( &SfxUndoListener::clearedRedo );
}


bool SfxUndoManager::ImplAddUndoAction_NoNotify( std::unique_ptr<SfxUndoAction> pAction, bool bTryMerge, bool bClearRedo, UndoManagerGuard& i_guard )
{
    if ( !ImplIsUndoEnabled_Lock() || ( m_xData->pActUndoArray->nMaxUndoActions == 0 ) )
    {
        i_guard.markForDeletion( std::move(pAction) );
        return false;
    }

    // merge, if required
    SfxUndoAction* pMergeWithAction = m_xData->pActUndoArray->nCurUndoAction ?
        m_xData->pActUndoArray->maUndoActions[m_xData->pActUndoArray->nCurUndoAction-1].pAction.get() : nullptr;
    if ( bTryMerge && pMergeWithAction )
    {
        bool bMerged = pMergeWithAction->Merge( pAction.get() );
        if ( bMerged )
        {
            i_guard.markForDeletion( std::move(pAction) );
            return false;
        }
    }

    // clear redo stack, if requested
    if ( bClearRedo && ( ImplGetRedoActionCount_Lock() > 0 ) )
        ImplClearRedo( i_guard, SfxUndoManager::CurrentLevel );

    // respect max number
    if( m_xData->pActUndoArray == &m_xData->maUndoArray )
    {
        while(m_xData->pActUndoArray->maUndoActions.size() >= m_xData->pActUndoArray->nMaxUndoActions)
        {
            i_guard.markForDeletion( m_xData->pActUndoArray->Remove(0) );
            if (m_xData->pActUndoArray->nCurUndoAction > 0)
            {
                --m_xData->pActUndoArray->nCurUndoAction;
                // fdo#66071 invalidate the current empty mark when removing
                --m_xData->mnEmptyMark;
            }
        }
    }

    // append new action
    m_xData->pActUndoArray->Insert( std::move(pAction), m_xData->pActUndoArray->nCurUndoAction++ );
    ImplCheckEmptyActions();
    return true;
}


void SfxUndoManager::AddUndoAction( std::unique_ptr<SfxUndoAction> pAction, bool bTryMerge )
{
    UndoManagerGuard aGuard( *m_xData );

    // add
    auto pActionTmp = pAction.get();
    if ( ImplAddUndoAction_NoNotify( std::move(pAction), bTryMerge, true, aGuard ) )
    {
        // notify listeners
        aGuard.scheduleNotification( &SfxUndoListener::undoActionAdded, pActionTmp->GetComment() );
    }
}


size_t SfxUndoManager::GetUndoActionCount( bool const i_currentLevel ) const
{
    UndoManagerGuard aGuard( *m_xData );
    const SfxUndoArray* pUndoArray = i_currentLevel ? m_xData->pActUndoArray : &m_xData->maUndoArray;
    return pUndoArray->nCurUndoAction;
}


OUString SfxUndoManager::GetUndoActionComment( size_t nNo, bool const i_currentLevel ) const
{
    UndoManagerGuard aGuard( *m_xData );

    OUString sComment;
    const SfxUndoArray* pUndoArray = i_currentLevel ? m_xData->pActUndoArray : &m_xData->maUndoArray;
    assert(nNo < pUndoArray->nCurUndoAction);
    if( nNo < pUndoArray->nCurUndoAction )
        sComment = pUndoArray->maUndoActions[ pUndoArray->nCurUndoAction - 1 - nNo ].pAction->GetComment();
    return sComment;
}


SfxUndoAction* SfxUndoManager::GetUndoAction( size_t nNo ) const
{
    UndoManagerGuard aGuard( *m_xData );

    assert(nNo < m_xData->pActUndoArray->nCurUndoAction);
    if( nNo >= m_xData->pActUndoArray->nCurUndoAction )
        return nullptr;
    return m_xData->pActUndoArray->maUndoActions[m_xData->pActUndoArray->nCurUndoAction-1-nNo].pAction.get();
}


css::uno::Sequence<css::beans::PropertyValue>
SfxUndoManager::GetUndoActionDetails( size_t nNo, bool const i_currentLevel ) const
{
    UndoManagerGuard aGuard( *m_xData );

    const SfxUndoArray* pUndoArray = i_currentLevel ? m_xData->pActUndoArray : &m_xData->maUndoArray;
    if( nNo >= pUndoArray->nCurUndoAction )
        return {};
    SfxUndoAction* pAction = pUndoArray->maUndoActions[ pUndoArray->nCurUndoAction - 1 - nNo ].pAction.get();
    if( !pAction )
        return {};
    const OUString aComment = pAction->GetComment();
    OUStringBuffer aPref;
    aPref.append(u"nNo=");
    aPref.append(static_cast<sal_Int32>(nNo));
    aPref.append(i_currentLevel ? u" cur" : u" top");
    aPref.append(u" title=\"");
    aPref.append(aComment);
    aPref.append(u"\"");
    const OUString aPrefix = aPref.makeStringAndClear();
    // Prefer structured override when Writer was rebuilt with GetActionDetails.
    // Fall back to dumpAsXml parse — works with older swlo (vtable ABI safe).
#if defined(DOCSKILL_UNDO_DETAILS_STUB)
    css::uno::Sequence<css::beans::PropertyValue> aStub = lcl_DetailsFromDumpAsXml(pAction);
    docskill::undo_log::logDetails("L2-svl", aPrefix + u" path=STUB/dumpAsXml"_ustr, aStub);
    return aStub;
#else
    css::uno::Sequence<css::beans::PropertyValue> aDetails = lcl_SafeGetActionDetails(pAction);
    if (aDetails.hasElements())
    {
        docskill::undo_log::logDetails(
            "L2-svl", aPrefix + u" path=GetActionDetails"_ustr, aDetails);
        return aDetails;
    }
    css::uno::Sequence<css::beans::PropertyValue> aXml = lcl_DetailsFromDumpAsXml(pAction);
    docskill::undo_log::logDetails(
        "L2-svl",
        aPrefix + (aXml.hasElements() ? u" path=dumpAsXml-fallback"_ustr
                                      : u" path=EMPTY"_ustr),
        aXml);
    return aXml;
#endif
}


/** clears the redo stack and removes the top undo action */
void SfxUndoManager::RemoveLastUndoAction()
{
    UndoManagerGuard aGuard( *m_xData );

    ENSURE_OR_RETURN_VOID( m_xData->pActUndoArray->nCurUndoAction, "svl::SfxUndoManager::RemoveLastUndoAction(), no action to remove?!" );

    m_xData->pActUndoArray->nCurUndoAction--;

    // delete redo-actions and top action
    for ( size_t nPos = m_xData->pActUndoArray->maUndoActions.size(); nPos > m_xData->pActUndoArray->nCurUndoAction; --nPos )
    {
        aGuard.markForDeletion( std::move(m_xData->pActUndoArray->maUndoActions[nPos-1].pAction) );
    }

    m_xData->pActUndoArray->Remove(
        m_xData->pActUndoArray->nCurUndoAction,
        m_xData->pActUndoArray->maUndoActions.size() - m_xData->pActUndoArray->nCurUndoAction );
    ImplCheckEmptyActions();
}


bool SfxUndoManager::IsDoing() const
{
    UndoManagerGuard aGuard( *m_xData );
    return m_xData->mbDoing;
}


bool SfxUndoManager::Undo()
{
    return ImplUndo( nullptr );
}


bool SfxUndoManager::UndoWithContext( SfxUndoContext& i_context )
{
    return ImplUndo( &i_context );
}


bool SfxUndoManager::ImplUndo( SfxUndoContext* i_contextOrNull )
{
    UndoManagerGuard aGuard( *m_xData );
    assert( !IsDoing() && "SfxUndoManager::Undo: *nested* Undo/Redo actions? How this?" );

    ::comphelper::FlagGuard aDoingGuard( m_xData->mbDoing );
    m_xData->mbDoing = true;

    LockGuard aLockGuard( *this );

    if ( ImplIsInListAction_Lock() )
    {
        assert(!"SfxUndoManager::Undo: not possible when within a list action!");
        return false;
    }

    if ( m_xData->pActUndoArray->nCurUndoAction == 0 )
    {
        SAL_WARN("svl", "SfxUndoManager::Undo: undo stack is empty!" );
        return false;
    }

    if (i_contextOrNull && i_contextOrNull->GetUndoOffset() > 0)
    {
        size_t nCurrent = m_xData->pActUndoArray->nCurUndoAction;
        size_t nOffset = i_contextOrNull->GetUndoOffset();
        if (nCurrent >= nOffset + 1)
        {
            // Move the action we want to execute to the top of the undo stack.
            // data() + nCurrent - nOffset - 1 is the start, data() + nCurrent - nOffset is what we
            // want to move to the top, maUndoActions.data() + nCurrent is past the end/top of the
            // undo stack.
            std::rotate(m_xData->pActUndoArray->maUndoActions.data() + nCurrent - nOffset - 1,
                        m_xData->pActUndoArray->maUndoActions.data() + nCurrent - nOffset,
                        m_xData->pActUndoArray->maUndoActions.data() + nCurrent);
        }
    }

    SfxUndoAction* pAction = m_xData->pActUndoArray->maUndoActions[ --m_xData->pActUndoArray->nCurUndoAction ].pAction.get();
    const OUString sActionComment = pAction->GetComment();
    try
    {
        // clear the guard/mutex before calling into the SfxUndoAction - this can be an extension-implemented UNO component
        // nowadays ...
        auto aResetGuard(aGuard.clear());
        if ( i_contextOrNull != nullptr )
            pAction->UndoWithContext( *i_contextOrNull );
        else
            pAction->Undo();
    }
    catch( ... )
    {
        // in theory, somebody might have tampered with all of *m_xData while the mutex was unlocked. So, see if
        // we still find pAction in our current Undo array
        size_t nCurAction = 0;
        while ( nCurAction < m_xData->pActUndoArray->maUndoActions.size() )
        {
            if ( m_xData->pActUndoArray->maUndoActions[ nCurAction++ ].pAction.get() == pAction )
            {
                // the Undo action is still there ...
                // assume the error is a permanent failure, and clear the Undo stack
                ImplClearUndo( aGuard );
                throw;
            }
        }
        SAL_WARN("svl", "SfxUndoManager::Undo: can't clear the Undo stack after the failure - some other party was faster ..." );
        throw;
    }

    m_xData->mbDoing = false;
    if (m_xData->moNeedsClearRedo.has_value())
    {
        ImplClearRedo_NoLock(*m_xData->moNeedsClearRedo);
        m_xData->moNeedsClearRedo.reset();
    }

    aGuard.scheduleNotification( &SfxUndoListener::actionUndone, sActionComment );

    return true;
}


size_t SfxUndoManager::GetRedoActionCount( bool const i_currentLevel ) const
{
    UndoManagerGuard aGuard( *m_xData );
    return ImplGetRedoActionCount_Lock( i_currentLevel );
}


size_t SfxUndoManager::ImplGetRedoActionCount_Lock( bool const i_currentLevel ) const
{
    const SfxUndoArray* pUndoArray = i_currentLevel ? m_xData->pActUndoArray : &m_xData->maUndoArray;
    return pUndoArray->maUndoActions.size() - pUndoArray->nCurUndoAction;
}


SfxUndoAction* SfxUndoManager::GetRedoAction(size_t nNo) const
{
    UndoManagerGuard aGuard( *m_xData );

    const SfxUndoArray* pUndoArray = m_xData->pActUndoArray;
    if ( (pUndoArray->nCurUndoAction) > pUndoArray->maUndoActions.size() )
    {
        return nullptr;
    }
    return pUndoArray->maUndoActions[pUndoArray->nCurUndoAction + nNo].pAction.get();
}


OUString SfxUndoManager::GetRedoActionComment( size_t nNo, bool const i_currentLevel ) const
{
    OUString sComment;
    UndoManagerGuard aGuard( *m_xData );
    const SfxUndoArray* pUndoArray = i_currentLevel ? m_xData->pActUndoArray : &m_xData->maUndoArray;
    if ( (pUndoArray->nCurUndoAction + nNo) < pUndoArray->maUndoActions.size() )
    {
        sComment = pUndoArray->maUndoActions[ pUndoArray->nCurUndoAction + nNo ].pAction->GetComment();
    }
    return sComment;
}


bool SfxUndoManager::Redo()
{
    return ImplRedo( nullptr );
}


bool SfxUndoManager::RedoWithContext( SfxUndoContext& i_context )
{
    return ImplRedo( &i_context );
}


bool SfxUndoManager::ImplRedo( SfxUndoContext* i_contextOrNull )
{
    UndoManagerGuard aGuard( *m_xData );
    assert( !IsDoing() && "SfxUndoManager::Redo: *nested* Undo/Redo actions? How this?" );

    ::comphelper::FlagGuard aDoingGuard( m_xData->mbDoing );
    m_xData->mbDoing = true;

    LockGuard aLockGuard( *this );

    if ( ImplIsInListAction_Lock() )
    {
        assert(!"SfxUndoManager::Redo: not possible when within a list action!");
        return false;
    }

    if ( m_xData->pActUndoArray->nCurUndoAction >= m_xData->pActUndoArray->maUndoActions.size() )
    {
        SAL_WARN("svl", "SfxUndoManager::Redo: redo stack is empty!");
        return false;
    }

    SfxUndoAction* pAction = m_xData->pActUndoArray->maUndoActions[ m_xData->pActUndoArray->nCurUndoAction++ ].pAction.get();
    const OUString sActionComment = pAction->GetComment();
    try
    {
        // clear the guard/mutex before calling into the SfxUndoAction - this can be an extension-implemented UNO component
        // nowadays ...
        auto aResetGuard(aGuard.clear());
        if ( i_contextOrNull != nullptr )
            pAction->RedoWithContext( *i_contextOrNull );
        else
            pAction->Redo();
    }
    catch( ... )
    {
        // in theory, somebody might have tampered with all of *m_xData while the mutex was unlocked. So, see if
        // we still find pAction in our current Undo array
        size_t nCurAction = 0;
        while ( nCurAction < m_xData->pActUndoArray->maUndoActions.size() )
        {
            if ( m_xData->pActUndoArray->maUndoActions[ nCurAction ].pAction.get() == pAction )
            {
                // the Undo action is still there ...
                // assume the error is a permanent failure, and clear the Undo stack
                ImplClearRedo( aGuard, SfxUndoManager::CurrentLevel );
                throw;
            }
            ++nCurAction;
        }
        SAL_WARN("svl", "SfxUndoManager::Redo: can't clear the Undo stack after the failure - some other party was faster ..." );
        throw;
    }

    m_xData->mbDoing = false;
    assert(!m_xData->moNeedsClearRedo.has_value() && "Assuming I don't need to handle it here. What about if thrown?");
    ImplCheckEmptyActions();
    aGuard.scheduleNotification( &SfxUndoListener::actionRedone, sActionComment );

    return true;
}


size_t SfxUndoManager::GetRepeatActionCount() const
{
    UndoManagerGuard aGuard( *m_xData );
    return m_xData->pActUndoArray->maUndoActions.size();
}


OUString SfxUndoManager::GetRepeatActionComment(SfxRepeatTarget &rTarget) const
{
    UndoManagerGuard aGuard( *m_xData );
    return m_xData->pActUndoArray->maUndoActions[ m_xData->pActUndoArray->maUndoActions.size() - 1 ].pAction
        ->GetRepeatComment(rTarget);
}


bool SfxUndoManager::Repeat( SfxRepeatTarget &rTarget )
{
    UndoManagerGuard aGuard( *m_xData );
    if ( !m_xData->pActUndoArray->maUndoActions.empty() )
    {
        SfxUndoAction* pAction = m_xData->pActUndoArray->maUndoActions.back().pAction.get();
        auto aResetGuard(aGuard.clear());
        if ( pAction->CanRepeat( rTarget ) )
            pAction->Repeat( rTarget );
        return true;
    }

    return false;
}


bool SfxUndoManager::CanRepeat( SfxRepeatTarget &rTarget ) const
{
    UndoManagerGuard aGuard( *m_xData );
    if ( !m_xData->pActUndoArray->maUndoActions.empty() )
    {
        size_t nActionNo = m_xData->pActUndoArray->maUndoActions.size() - 1;
        return m_xData->pActUndoArray->maUndoActions[nActionNo].pAction->CanRepeat(rTarget);
    }
    return false;
}


void SfxUndoManager::AddUndoListener( SfxUndoListener& i_listener )
{
    UndoManagerGuard aGuard( *m_xData );
    m_xData->aListeners.push_back( &i_listener );
}


void SfxUndoManager::RemoveUndoListener( SfxUndoListener& i_listener )
{
    UndoManagerGuard aGuard( *m_xData );
    auto lookup = std::find(m_xData->aListeners.begin(), m_xData->aListeners.end(), &i_listener);
    if (lookup != m_xData->aListeners.end())
        m_xData->aListeners.erase( lookup );
}

/**
 * Inserts a ListUndoAction and sets its UndoArray as current.
 */
void SfxUndoManager::EnterListAction( const OUString& rComment,
                                      const OUString &rRepeatComment, sal_uInt16 nId,
                                      ViewShellId nViewShellId )
{
    UndoManagerGuard aGuard( *m_xData );

    if( !ImplIsUndoEnabled_Lock() )
        return;

    if ( !m_xData->maUndoArray.nMaxUndoActions )
        return;

    SfxListUndoAction* pAction = new SfxListUndoAction( rComment, rRepeatComment, nId, nViewShellId, m_xData->pActUndoArray );
    OSL_VERIFY( ImplAddUndoAction_NoNotify( std::unique_ptr<SfxUndoAction>(pAction), false, false, aGuard ) );
    // expected to succeed: all conditions under which it could fail should have been checked already
    m_xData->pActUndoArray = pAction;

    // notification
    aGuard.scheduleNotification( &SfxUndoListener::listActionEntered, rComment );
}


bool SfxUndoManager::IsInListAction() const
{
    UndoManagerGuard aGuard( *m_xData );
    return ImplIsInListAction_Lock();
}


bool SfxUndoManager::ImplIsInListAction_Lock() const
{
    return m_xData->pActUndoArray != &m_xData->maUndoArray;
}


size_t SfxUndoManager::GetListActionDepth() const
{
    UndoManagerGuard aGuard( *m_xData );
    size_t nDepth(0);

    SfxUndoArray* pLookup( m_xData->pActUndoArray );
    while ( pLookup != &m_xData->maUndoArray )
    {
        pLookup = pLookup->pFatherUndoArray;
        ++nDepth;
    }

    return nDepth;
}


size_t SfxUndoManager::LeaveListAction()
{
    UndoManagerGuard aGuard( *m_xData );
    size_t nCount = ImplLeaveListAction( false, aGuard );

    if ( m_xData->mbClearUntilTopLevel )
    {
        ImplClearCurrentLevel_NoNotify( aGuard );
        if ( !ImplIsInListAction_Lock() )
        {
            m_xData->mbClearUntilTopLevel = false;
            aGuard.scheduleNotification( &SfxUndoListener::cleared );
        }
        nCount = 0;
    }

    return nCount;
}


size_t SfxUndoManager::LeaveAndMergeListAction()
{
    UndoManagerGuard aGuard( *m_xData );
    return ImplLeaveListAction( true, aGuard );
}


size_t SfxUndoManager::ImplLeaveListAction( const bool i_merge, UndoManagerGuard& i_guard )
{
    if ( !ImplIsUndoEnabled_Lock() )
        return 0;

    if ( !m_xData->maUndoArray.nMaxUndoActions )
        return 0;

    if( !ImplIsInListAction_Lock() )
    {
        SAL_WARN("svl", "svl::SfxUndoManager::ImplLeaveListAction, called without calling EnterListAction()!" );
        return 0;
    }

    assert(m_xData->pActUndoArray->pFatherUndoArray);

    // the array/level which we're about to leave
    SfxUndoArray* pArrayToLeave = m_xData->pActUndoArray;
    // one step up
    m_xData->pActUndoArray = m_xData->pActUndoArray->pFatherUndoArray;

    // If no undo actions were added to the list, delete the list action
    const size_t nListActionElements = pArrayToLeave->nCurUndoAction;
    if ( nListActionElements == 0 )
    {
        i_guard.markForDeletion( m_xData->pActUndoArray->Remove( --m_xData->pActUndoArray->nCurUndoAction ) );
        i_guard.scheduleNotification( &SfxUndoListener::listActionCancelled );
        return 0;
    }

    // now that it is finally clear the list action is non-trivial, and does participate in the Undo stack, clear
    // the redo stack
    ImplClearRedo( i_guard, SfxUndoManager::CurrentLevel );

    SfxUndoAction* pCurrentAction= m_xData->pActUndoArray->maUndoActions[ m_xData->pActUndoArray->nCurUndoAction-1 ].pAction.get();
    SfxListUndoAction* pListAction = dynamic_cast< SfxListUndoAction * >( pCurrentAction );
    ENSURE_OR_RETURN( pListAction, "SfxUndoManager::ImplLeaveListAction: list action expected at this position!", nListActionElements );

    if ( i_merge )
    {
        // merge the list action with its predecessor on the same level
        SAL_WARN_IF( m_xData->pActUndoArray->nCurUndoAction <= 1, "svl",
            "SfxUndoManager::ImplLeaveListAction: cannot merge the list action if there's no other action on the same level - check this beforehand!" );
        if ( m_xData->pActUndoArray->nCurUndoAction > 1 )
        {
            std::unique_ptr<SfxUndoAction> pPreviousAction = m_xData->pActUndoArray->Remove( m_xData->pActUndoArray->nCurUndoAction - 2 );
            --m_xData->pActUndoArray->nCurUndoAction;
            pListAction->SetComment( pPreviousAction->GetComment() );
            pListAction->Insert( std::move(pPreviousAction), 0 );
            ++pListAction->nCurUndoAction;
        }
    }

    // if the undo array has no comment, try to get it from its children
    if ( pListAction->GetComment().isEmpty() )
    {
        for( size_t n = 0; n < pListAction->maUndoActions.size(); n++ )
        {
            if (!pListAction->maUndoActions[n].pAction->GetComment().isEmpty())
            {
                pListAction->SetComment( pListAction->maUndoActions[n].pAction->GetComment() );
                break;
            }
        }
    }

    ImplIsEmptyActions();
    // notify listeners
    i_guard.scheduleNotification( &SfxUndoListener::listActionLeft, pListAction->GetComment() );

    // outta here
    return nListActionElements;
}

UndoStackMark SfxUndoManager::MarkTopUndoAction()
{
    UndoManagerGuard aGuard( *m_xData );

    SAL_WARN_IF( IsInListAction(), "svl",
            "SfxUndoManager::MarkTopUndoAction(): suspicious call!" );
    assert((m_xData->mnMarks + 1) < (m_xData->mnEmptyMark - 1) &&
            "SfxUndoManager::MarkTopUndoAction(): mark overflow!");

    size_t const nActionPos = m_xData->maUndoArray.nCurUndoAction;
    if (0 == nActionPos)
    {
        --m_xData->mnEmptyMark;
        return m_xData->mnEmptyMark;
    }

    m_xData->maUndoArray.maUndoActions[ nActionPos-1 ].aMarks.push_back(
            ++m_xData->mnMarks );
    return m_xData->mnMarks;
}

size_t SfxUndoManager::RemoveMark(UndoStackMark i_mark)
{
    UndoManagerGuard aGuard( *m_xData );

    if ((m_xData->mnEmptyMark < i_mark) || (MARK_INVALID == i_mark))
    {
        return std::numeric_limits<size_t>::max(); // nothing to remove
    }
    else if (i_mark == m_xData->mnEmptyMark)
    {
        --m_xData->mnEmptyMark; // never returned from MarkTop => invalid
        return std::numeric_limits<size_t>::max();
    }

    for ( size_t i=0; i<m_xData->maUndoArray.maUndoActions.size(); ++i )
    {
        MarkedUndoAction& rAction = m_xData->maUndoArray.maUndoActions[i];
        auto markPos = std::find(rAction.aMarks.begin(), rAction.aMarks.end(), i_mark);
        if (markPos != rAction.aMarks.end())
        {
            rAction.aMarks.erase( markPos );
            return i;
        }
    }
    SAL_WARN("svl", "SfxUndoManager::RemoveMark: mark not found!");
        // TODO: this might be too offensive. There are situations where we implicitly remove marks
        // without our clients, in particular the client which created the mark, having a chance to know
        // about this.

    return std::numeric_limits<size_t>::max();
}

bool SfxUndoManager::HasTopUndoActionMark( UndoStackMark const i_mark )
{
    UndoManagerGuard aGuard( *m_xData );

    size_t nActionPos = m_xData->maUndoArray.nCurUndoAction;
    if ( nActionPos == 0 )
    {
        return (i_mark == m_xData->mnEmptyMark);
    }

    const MarkedUndoAction& rAction =
            m_xData->maUndoArray.maUndoActions[ nActionPos-1 ];

    return std::find(rAction.aMarks.begin(), rAction.aMarks.end(), i_mark) != rAction.aMarks.end();
}


void SfxUndoManager::UndoMark(UndoStackMark i_mark)
{
    SfxMarkedUndoContext context(*this, i_mark); // Removes the mark
    if (context.GetUndoOffset() == std::numeric_limits<size_t>::max())
        return; // nothing to undo

    UndoWithContext(context);
}


bool SfxUndoManager::RemoveOldestUndoAction()
{
    UndoManagerGuard aGuard( *m_xData );

    if ( IsInListAction() && ( m_xData->maUndoArray.nCurUndoAction == 1 ) )
    {
        // this can happen if we are performing a very large writer edit (e.g. removing a very large table)
        SAL_WARN("svl", "SfxUndoManager::RemoveOldestUndoActions: cannot remove a not-yet-closed list action!");
        return false;
    }

    aGuard.markForDeletion( m_xData->maUndoArray.Remove( 0 ) );
    --m_xData->maUndoArray.nCurUndoAction;
    ImplCheckEmptyActions();
    return true;
}

void SfxUndoManager::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    UndoManagerGuard aGuard(*m_xData);

    bool bOwns = false;
    if (!pWriter)
    {
        pWriter = xmlNewTextWriterFilename("undo.xml", 0);
        xmlTextWriterSetIndent(pWriter,1);
        (void)xmlTextWriterSetIndentString(pWriter, BAD_CAST("  "));
        (void)xmlTextWriterStartDocument(pWriter, nullptr, nullptr, nullptr);
        bOwns = true;
    }

    tools::XmlWriter aWriter(pWriter);
    aWriter.startElement("SfxUndoManager");
    aWriter.attribute("nUndoActionCount", GetUndoActionCount());
    aWriter.attribute("nRedoActionCount", GetRedoActionCount());
    aWriter.startElement("undoActions");
    for (size_t i = 0; i < GetUndoActionCount(); ++i)
    {
        const SfxUndoArray* pUndoArray = m_xData->pActUndoArray;
        pUndoArray->maUndoActions[pUndoArray->nCurUndoAction - 1 - i].pAction->dumpAsXml(pWriter);
    }
    aWriter.endElement();
    aWriter.startElement("redoActions");
    for (size_t i = 0; i < GetRedoActionCount(); ++i)
    {
        const SfxUndoArray* pUndoArray = m_xData->pActUndoArray;
        pUndoArray->maUndoActions[pUndoArray->nCurUndoAction + i].pAction->dumpAsXml(pWriter);
    }

    aWriter.endElement();
    aWriter.endElement();

    if (bOwns)
    {
        (void)xmlTextWriterEndDocument(pWriter);
        xmlFreeTextWriter(pWriter);
    }
}

/// Returns a JSON representation of pAction.
static boost::property_tree::ptree lcl_ActionToJson(size_t nIndex, SfxUndoAction const * pAction)
{
    boost::property_tree::ptree aRet;
    aRet.put("index", nIndex);
    aRet.put("comment", pAction->GetComment().toUtf8().getStr());
    aRet.put("viewId", static_cast<sal_Int32>(pAction->GetViewShellId()));
    aRet.put("dateTime", utl::toISO8601(pAction->GetDateTime().GetUNODateTime()).toUtf8().getStr());
    return aRet;
}

OUString SfxUndoManager::GetUndoActionsInfo() const
{
    boost::property_tree::ptree aActions;
    const SfxUndoArray* pUndoArray = m_xData->pActUndoArray;
    for (size_t i = 0; i < GetUndoActionCount(); ++i)
    {
        boost::property_tree::ptree aAction = lcl_ActionToJson(i, pUndoArray->maUndoActions[pUndoArray->nCurUndoAction - 1 - i].pAction.get());
        aActions.push_back(std::make_pair("", aAction));
    }

    boost::property_tree::ptree aTree;
    aTree.add_child("actions", aActions);
    std::stringstream aStream;
    boost::property_tree::write_json(aStream, aTree);
    return OUString::fromUtf8(aStream.str());
}

OUString SfxUndoManager::GetRedoActionsInfo() const
{
    boost::property_tree::ptree aActions;
    const SfxUndoArray* pUndoArray = m_xData->pActUndoArray;
    size_t nCount = GetRedoActionCount();
    for (size_t i = 0; i < nCount; ++i)
    {
        size_t nIndex = nCount - i - 1;
        boost::property_tree::ptree aAction = lcl_ActionToJson(nIndex, pUndoArray->maUndoActions[pUndoArray->nCurUndoAction + nIndex].pAction.get());
        aActions.push_back(std::make_pair("", aAction));
    }

    boost::property_tree::ptree aTree;
    aTree.add_child("actions", aActions);
    std::stringstream aStream;
    boost::property_tree::write_json(aStream, aTree);
    return OUString::fromUtf8(aStream.str());
}

bool SfxUndoManager::IsEmptyActions() const
{
    UndoManagerGuard aGuard(*m_xData);

    return ImplIsEmptyActions();
}

inline bool SfxUndoManager::ImplIsEmptyActions() const
{
    return m_xData->maUndoArray.nCurUndoAction || m_xData->maUndoArray.maUndoActions.size() - m_xData->maUndoArray.nCurUndoAction;
}

void SfxUndoManager::ImplCheckEmptyActions()
{
    bool bEmptyActions = ImplIsEmptyActions();
    if (m_xData->mbEmptyActions != bEmptyActions)
    {
        m_xData->mbEmptyActions = bEmptyActions;
        EmptyActionsChanged();
    }
}

void SfxUndoManager::EmptyActionsChanged()
{

}

struct SfxListUndoAction::Impl
{
    sal_uInt16 mnId;
    ViewShellId mnViewShellId;

    OUString maComment;
    OUString maRepeatComment;

    Impl( sal_uInt16 nId, ViewShellId nViewShellId, OUString aComment, OUString aRepeatComment ) :
        mnId(nId), mnViewShellId(nViewShellId), maComment(std::move(aComment)), maRepeatComment(std::move(aRepeatComment)) {}
};

sal_uInt16 SfxListUndoAction::GetId() const
{
    return mpImpl->mnId;
}

OUString SfxListUndoAction::GetComment() const
{
    return mpImpl->maComment;
}

void SfxListUndoAction::SetComment(const OUString& rComment)
{
    mpImpl->maComment = rComment;
}

ViewShellId SfxListUndoAction::GetViewShellId() const
{
    return mpImpl->mnViewShellId;
}

OUString SfxListUndoAction::GetRepeatComment(SfxRepeatTarget &) const
{
    return mpImpl->maRepeatComment;
}

SfxListUndoAction::SfxListUndoAction(
    const OUString &rComment,
    const OUString &rRepeatComment,
    sal_uInt16 nId,
    ViewShellId nViewShellId,
    SfxUndoArray *pFather ) :
    mpImpl(new Impl(nId, nViewShellId, rComment, rRepeatComment))
{
    pFatherUndoArray = pFather;
    nMaxUndoActions = USHRT_MAX;
}

SfxListUndoAction::~SfxListUndoAction()
{
}

void SfxListUndoAction::Undo()
{
    for(size_t i=nCurUndoAction;i>0;)
        maUndoActions[--i].pAction->Undo();
    nCurUndoAction=0;
}


void SfxListUndoAction::UndoWithContext( SfxUndoContext& i_context )
{
    for(size_t i=nCurUndoAction;i>0;)
        maUndoActions[--i].pAction->UndoWithContext( i_context );
    nCurUndoAction=0;
}


void SfxListUndoAction::Redo()
{
    for(size_t i=nCurUndoAction;i<maUndoActions.size();i++)
        maUndoActions[i].pAction->Redo();
    nCurUndoAction = maUndoActions.size();
}


void SfxListUndoAction::RedoWithContext( SfxUndoContext& i_context )
{
    for(size_t i=nCurUndoAction;i<maUndoActions.size();i++)
        maUndoActions[i].pAction->RedoWithContext( i_context );
    nCurUndoAction = maUndoActions.size();
}


void SfxListUndoAction::Repeat(SfxRepeatTarget&rTarget)
{
    for(size_t i=0;i<nCurUndoAction;i++)
        maUndoActions[i].pAction->Repeat(rTarget);
}


bool SfxListUndoAction::CanRepeat(SfxRepeatTarget&r)  const
{
    for(size_t i=0;i<nCurUndoAction;i++)
    {
        if(!maUndoActions[i].pAction->CanRepeat(r))
            return false;
    }
    return true;
}


bool SfxListUndoAction::Merge( SfxUndoAction *pNextAction )
{
    return !maUndoActions.empty() && maUndoActions[maUndoActions.size()-1].pAction->Merge( pNextAction );
}

css::uno::Sequence<css::beans::PropertyValue> SfxListUndoAction::GetActionDetails() const
{
    // Prefer first nested action that publishes details (Writer format undos).
    for (size_t i = 0; i < nCurUndoAction; ++i)
    {
        SfxUndoAction* pAction = maUndoActions[i].pAction.get();
        if (!pAction)
            continue;
        css::uno::Sequence<css::beans::PropertyValue> aDetails = lcl_SafeGetActionDetails(pAction);
        if (aDetails.hasElements())
            return aDetails;
    }
    return {};
}

void SfxListUndoAction::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    tools::XmlWriter aWriter(pWriter);
    aWriter.startElement("SfxListUndoAction");
    aWriter.attribute("size", maUndoActions.size());
    SfxUndoAction::dumpAsXml(pWriter);
    for (size_t i = 0; i < maUndoActions.size(); ++i)
        maUndoActions[i].pAction->dumpAsXml(pWriter);
    aWriter.endElement();
}

SfxUndoArray::~SfxUndoArray()
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
