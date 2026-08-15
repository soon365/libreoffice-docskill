/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "externaledit.hxx"

#include <com/sun/star/system/SystemShellExecute.hpp>
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/ucb/XSimpleFileAccess3.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <osl/file.hxx>

#ifdef _WIN32
#include <shlwapi.h>
#include <vector>
#include <singleprov/singlescriptfactory.hxx>
#endif

#include "provcontext.hxx"

namespace singleprovider
{
#ifdef _WIN32
namespace
{
bool windowsDefaultAppMightExecute(const std::shared_ptr<ProviderContext>& pProviderContext)
{
    ASSOCF nFlags = 0;
    ASSOCSTR nAssocStr = ASSOCSTR_EXECUTABLE;
    DWORD nLength = 0;

    // Include a '\0' to make the string null terminated
    OUString sExtension
        = pProviderContext->m_pSingleScriptFactory->getExtension() + OUStringChar(u'\0');
    LPCWSTR pExtension = reinterpret_cast<const wchar_t*>(sExtension.getStr());

    // First get the length of the string
    if (AssocQueryStringW(nFlags, nAssocStr, pExtension, nullptr, nullptr, &nLength) != S_FALSE)
        return true;

    std::vector<wchar_t> aExeBuf(nLength);

    if (AssocQueryStringW(nFlags, nAssocStr, pExtension, nullptr, aExeBuf.data(), &nLength) != S_OK)
        return true;

    OUString sExe(reinterpret_cast<const sal_Unicode*>(aExeBuf.data()));

    sal_Int32 lastSlash = sExe.lastIndexOf(u'\\');

    if (lastSlash != -1)
        sExe = sExe.copy(lastSlash + 1);

    return pProviderContext->m_pSingleScriptFactory->appMightExecute(sExe);
}
}
#endif

void externalEdit(const std::shared_ptr<ProviderContext>& pProviderContext, const OUString& sUrl)
{
    css::uno::Reference<css::system::XSystemShellExecute> xSystemShellExecute
        = css::system::SystemShellExecute::create(pProviderContext->m_xContext);

#ifdef _WIN32

    OUString sPath;

    if (osl::FileBase::getSystemPathFromFileURL(sUrl, sPath) != osl::FileBase::E_None)
        return;

    // If it looks like the default windows app might be a Python interpreter then open the file in
    // Notepad instead
    if (windowsDefaultAppMightExecute(pProviderContext))
    {
        xSystemShellExecute->execute("notepad", "\"" + sPath + "\"",
                                     css::system::SystemShellExecuteFlags::DEFAULTS);
    }
    else
    {
        OUString sPathWithNull = sPath + OUStringChar(u'\0');
        LPCWSTR pPathWithNull = reinterpret_cast<const wchar_t*>(sPathWithNull.getStr());
        ShellExecuteW(nullptr, L"open", pPathWithNull, nullptr, nullptr, SW_SHOWNORMAL);
    }

#else

    xSystemShellExecute->execute(sUrl, "", css::system::SystemShellExecuteFlags::URIS_ONLY);

#endif
}

bool isEditable(const std::shared_ptr<ProviderContext>& pProviderContext, const OUString& sUrl)
{
    if (pProviderContext->m_xFileAccess->isReadOnly(sUrl))
        return false;

    OUString sPath;

    if (osl::FileBase::getSystemPathFromFileURL(sUrl, sPath) != osl::FileBase::E_None)
        return false;

    return true;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
