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

#include <osl/thread.h>

#include <unx/fontmanager.hxx>
#include <unx/gendata.hxx>
#include <unx/helper.hxx>

#include <tools/urlobj.hxx>

#include <sal/log.hxx>

#if OSL_DEBUG_LEVEL > 1
#include <sys/times.h>
#include <stdio.h>
#endif

#ifdef CALLGRIND_COMPILE
#include <valgrind/callgrind.h>
#endif

using namespace psp;

/*
 *  one instance only
 */
PrintFontManager& PrintFontManager::get()
{
    GenericUnixSalData* const pSalData(GetGenericUnixSalData());
    assert(pSalData);
    return *pSalData->GetPrintFontManager();
}

/*
 *  the PrintFontManager
 */

PrintFontManager::PrintFontManager()
    : m_nNextFontID( 1 )
    , m_aFontInstallerTimer("PrintFontManager m_aFontInstallerTimer")
{
    m_aFontInstallerTimer.SetInvokeHandler(LINK(this, PrintFontManager, autoInstallFontLangSupport));
    m_aFontInstallerTimer.SetTimeout(5000);

#ifdef CALLGRIND_COMPILE
    CALLGRIND_TOGGLE_COLLECT();
    CALLGRIND_ZERO_STATS();
#endif

#if OSL_DEBUG_LEVEL > 1
    clock_t aStart;
    clock_t aStep1;
    clock_t aStep2;

    struct tms tms;

    aStart = times(&tms);
#endif

    // first try fontconfig
    initFontconfig();

    // part one - look for downloadable fonts
    rtl_TextEncoding aEncoding = osl_getThreadTextEncoding();
    const OUString& rSalPrivatePath = psp::getFontPath();

    // search for the fonts in SAL_PRIVATE_FONTPATH first; those are
    // the fonts installed with the office
    if (!rSalPrivatePath.isEmpty())
    {
        OString aPath = OUStringToOString(rSalPrivatePath, aEncoding);
        sal_Int32 nIndex = 0;
        do
        {
            OString aToken = aPath.getToken(0, ';', nIndex);
            normPath(aToken);
            if (!aToken.isEmpty())
                addFontconfigDir(aToken);
        } while (nIndex >= 0);
    }

    countFontconfigFonts();

#if OSL_DEBUG_LEVEL > 1
    aStep1 = times(&tms);

    aStep2 = times(&tms);
    SAL_INFO("vcl.fonts",
             "PrintFontManager::PrintFontManager: collected " << m_aFonts.size() << " fonts.");
    double fTick = (double)sysconf(_SC_CLK_TCK);
    SAL_INFO("vcl.fonts", "Step 1 took " << ((double)(aStep1 - aStart) / fTick) << " seconds.");
    SAL_INFO("vcl.fonts", "Step 2 took " << ((double)(aStep2 - aStep1) / fTick) << " seconds.");
#endif

#ifdef CALLGRIND_COMPILE
    CALLGRIND_DUMP_STATS();
    CALLGRIND_TOGGLE_COLLECT();
#endif
}

PrintFontManager::~PrintFontManager()
{
    m_aFontInstallerTimer.Stop();
    deinitFontconfig();
}

std::vector<fontID> PrintFontManager::findFontFileIDs( std::u16string_view rFileUrl ) const
{
    INetURLObject aPath( rFileUrl );
    OString aFullPath(OUStringToOString(aPath.GetFull(), osl_getThreadTextEncoding()));
    return findFontFileIDs(aFullPath);
}

std::vector<fontID> PrintFontManager::addFontFile( std::u16string_view rFileUrl )
{
    INetURLObject aPath( rFileUrl );
    OString aFullPath = OUStringToOString(aPath.GetFull(), osl_getThreadTextEncoding());
    std::vector<fontID> aFontIds = findFontFileIDs(aFullPath);
    if( aFontIds.empty() )
    {
        addFontconfigFile(aFullPath);

        std::vector<PrintFont> aNewFonts = fontsFromFontconfigFile(aFullPath);
        for (auto & font : aNewFonts)
        {
            fontID nFontId = m_nNextFontID++;
            m_aFonts[nFontId] = std::move(font);
            m_aFontFileToFontID[ aFullPath ].insert( nFontId );
            aFontIds.push_back(nFontId);
        }
    }
    return aFontIds;
}

void PrintFontManager::removeFontFile(std::u16string_view rFileUrl)
{
    INetURLObject aPath(rFileUrl);
    rtl_TextEncoding aEncoding = osl_getThreadTextEncoding();
    OString aFullPath(OUStringToOString(aPath.GetFull(), aEncoding));
    if (auto ids = findFontFileIDs(aFullPath); !ids.empty())
    {
        for (auto nFontID : ids)
        {
            m_aFonts.erase(nFontID);
            m_aFontFileToFontID[aFullPath].erase(nFontID);
        }
    }

    removeFontconfigFile(aFullPath);
}

fontID PrintFontManager::findFontFileID(const OString& rFontFile, int nFaceIndex, int nVariationIndex) const
{
    fontID nID = 0;

    auto set_it = m_aFontFileToFontID.find( rFontFile );
    if( set_it == m_aFontFileToFontID.end() )
        return nID;

    for (fontID elem : set_it->second)
    {
        auto it = m_aFonts.find(elem);
        if( it == m_aFonts.end() )
            continue;
        const PrintFont& rFont = (*it).second;
        if (rFont.m_aFontFile == rFontFile &&
            rFont.m_nCollectionEntry == nFaceIndex &&
            rFont.m_nVariationEntry == nVariationIndex)
        {
            nID = it->first;
            if (nID)
                break;
        }
    }

    return nID;
}

std::vector<fontID> PrintFontManager::findFontFileIDs(const OString& rFontFile) const
{
    std::vector<fontID> aIds;

    auto set_it = m_aFontFileToFontID.find( rFontFile );
    if( set_it == m_aFontFileToFontID.end() )
        return aIds;

    for (auto const& elem : set_it->second)
    {
        auto it = m_aFonts.find(elem);
        if( it == m_aFonts.end() )
            continue;
        aIds.push_back(it->first);
    }

    return aIds;
}

std::vector<fontID> PrintFontManager::getFontList()
{
    std::vector<fontID> aFontIDs;
    for (auto const& font : m_aFonts)
        aFontIDs.push_back(font.first);

    return aFontIDs;
}

int PrintFontManager::getFontFaceNumber( fontID nFontID ) const
{
    int nRet = 0;
    const PrintFont* pFont = getFont( nFontID );
    if (pFont)
    {
        nRet = pFont->m_nCollectionEntry;
        if (nRet < 0)
            nRet = 0;
    }
    return nRet;
}

int PrintFontManager::getFontFaceVariation( fontID nFontID ) const
{
    int nRet = 0;
    const PrintFont* pFont = getFont( nFontID );
    if (pFont)
    {
        nRet = pFont->m_nVariationEntry;
        if (nRet < 0)
            nRet = 0;
    }
    return nRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
