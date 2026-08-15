/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * DocSkill debug trace for Undo ActionDetails pipeline.
 *
 * Layers (correlate with Python %APPDATA%/docskill/user_actions.log):
 *   L1-sw     Writer SwUndoAttr / SwUndoFormatAttr::GetActionDetails
 *   L2-svl    SfxUndoManager::GetUndoActionDetails (+ SEH / dumpAsXml)
 *   L3-fwk-ev UndoManagerHelper buildEvent → UndoActionDetails on listener
 *   L3-fwk-q  UndoManagerHelper::getAllUndoActionDetails (live stack query)
 *
 * Sink: %APPDATA%/docskill/undo_details_kernel.log
 * Gate: set DOCSKILL_UNDO_DETAILS_LOG=0 to disable; unset or any other value = on.
 *
 * NOTE: Do not include windows.h here — Writer units pull vcl/wintypes.hxx
 * which conflicts with Win32 macros (GetObject etc.).
 */
#pragma once

#include <sal/config.h>

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/TypeClass.hpp>

#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

#if defined(_WIN32)
#include <direct.h>
#endif

namespace docskill::undo_log
{
inline bool isEnabled()
{
    const char* p = std::getenv("DOCSKILL_UNDO_DETAILS_LOG");
    if (p && p[0] == '0' && p[1] == '\0')
        return false;
    return true;
}

inline OUString anyBrief(const css::uno::Any& rAny)
{
    try
    {
        switch (rAny.getValueTypeClass())
        {
            case css::uno::TypeClass_BOOLEAN:
            {
                bool b = false;
                rAny >>= b;
                return b ? u"true"_ustr : u"false"_ustr;
            }
            case css::uno::TypeClass_BYTE:
            case css::uno::TypeClass_SHORT:
            case css::uno::TypeClass_UNSIGNED_SHORT:
            case css::uno::TypeClass_LONG:
            case css::uno::TypeClass_UNSIGNED_LONG:
            case css::uno::TypeClass_HYPER:
            case css::uno::TypeClass_UNSIGNED_HYPER:
            {
                sal_Int64 n = 0;
                rAny >>= n;
                return OUString::number(n);
            }
            case css::uno::TypeClass_FLOAT:
            case css::uno::TypeClass_DOUBLE:
            {
                double f = 0;
                rAny >>= f;
                return OUString::number(f);
            }
            case css::uno::TypeClass_STRING:
            {
                OUString s;
                rAny >>= s;
                if (s.getLength() > 40)
                    return s.copy(0, 40) + u"..."_ustr;
                return s;
            }
            default:
                return OUString::Concat(u"<") + rAny.getValueTypeName() + u">"_ustr;
        }
    }
    catch (...)
    {
        return u"<err>"_ustr;
    }
}

inline OUString detailsBrief(const css::uno::Sequence<css::beans::PropertyValue>& rDetails)
{
    if (!rDetails.hasElements())
        return u"(empty)"_ustr;
    OUStringBuffer aBuf;
    aBuf.append(u"n=");
    aBuf.append(static_cast<sal_Int32>(rDetails.getLength()));
    aBuf.append(u" {");
    const sal_Int32 nMax = std::min<sal_Int32>(rDetails.getLength(), 16);
    for (sal_Int32 i = 0; i < nMax; ++i)
    {
        if (i)
            aBuf.append(u"; ");
        aBuf.append(rDetails[i].Name);
        aBuf.append(u"=");
        aBuf.append(anyBrief(rDetails[i].Value));
    }
    if (rDetails.getLength() > nMax)
        aBuf.append(u"; ...");
    aBuf.append(u"}");
    return aBuf.makeStringAndClear();
}

inline void writeLine(const char* pLayer, const OUString& rMsg)
{
    if (!isEnabled())
        return;

    SAL_INFO("svl.docskill.undo", "[" << pLayer << "] " << rMsg);

    const char* pAppData = std::getenv("APPDATA");
    if (!pAppData || !*pAppData)
        return;

    std::string aDir = std::string(pAppData) + "\\docskill";
#if defined(_WIN32)
    _mkdir(aDir.c_str());
#endif
    const std::string aPath = aDir + "\\undo_details_kernel.log";

    FILE* pFile = nullptr;
#if defined(_WIN32)
    if (fopen_s(&pFile, aPath.c_str(), "a") != 0 || !pFile)
        return;
#else
    pFile = std::fopen(aPath.c_str(), "a");
    if (!pFile)
        return;
#endif

    const time_t now = time(nullptr);
    char aStamp[32] = {};
#if defined(_WIN32)
    struct tm tmBuf;
    if (localtime_s(&tmBuf, &now) == 0)
        strftime(aStamp, sizeof(aStamp), "%Y-%m-%d %H:%M:%S", &tmBuf);
    else
        snprintf(aStamp, sizeof(aStamp), "?");
#else
    if (struct tm* pTm = localtime(&now))
        strftime(aStamp, sizeof(aStamp), "%Y-%m-%d %H:%M:%S", pTm);
    else
        snprintf(aStamp, sizeof(aStamp), "?");
#endif

    const OString aUtf8 = OUStringToOString(rMsg, RTL_TEXTENCODING_UTF8);
    std::fprintf(pFile, "%s LAYER=%-10s %s\n", aStamp, pLayer, aUtf8.getStr());
    std::fclose(pFile);
}

inline void logDetails(const char* pLayer, const OUString& rPrefix,
                       const css::uno::Sequence<css::beans::PropertyValue>& rDetails)
{
    writeLine(pLayer, rPrefix + u" "_ustr + detailsBrief(rDetails));
}

} // namespace docskill::undo_log

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
