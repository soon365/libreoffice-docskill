/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "scriptdir.hxx"

#include <com/sun/star/script/browse/BrowseNodeTypes.hpp>
#include <com/sun/star/script/provider/XScriptURIHelper.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <rtl/uri.hxx>
#include <vector>

#include <singleprov/singlescriptfactory.hxx>

#include "provcontext.hxx"
#include "scriptfile.hxx"

namespace singleprovider
{
ScriptDir::ScriptDir(const std::shared_ptr<ProviderContext>& pProviderContext)
    : ScriptDir(pProviderContext, pProviderContext->m_pSingleScriptFactory->getLanguageName(),
                pProviderContext->m_xUriHelper->getRootStorageURI())
{
}

ScriptDir::ScriptDir(const std::shared_ptr<ProviderContext>& pProviderContext,
                     const OUString& sName, const OUString& sBaseUri)
    : ScriptBrowser(pProviderContext, sName, sBaseUri)
{
}

css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
    SAL_CALL ScriptDir::getChildNodes()
{
    const css::uno::Reference<css::ucb::XSimpleFileAccess3>& xFileAccess
        = m_pProviderContext->m_xFileAccess;

    try
    {
        css::uno::Sequence<OUString> aChildren = xFileAccess->getFolderContents(m_sBaseUri, true);

        std::vector<css::uno::Reference<css::script::browse::XBrowseNode>> aNodes;
        aNodes.reserve(aChildren.getLength());

        OUString sExtension = m_pProviderContext->m_pSingleScriptFactory->getExtension();

        for (int i = 0, nCount = aChildren.getLength(); i < nCount; ++i)
        {
            if (xFileAccess->isFolder(aChildren[i]))
            {
                OUString sName = nameFromUrl(aChildren[i]);

                aNodes.emplace_back(new ScriptDir(m_pProviderContext, sName, aChildren[i]));
            }
            else if (aChildren[i].endsWith(sExtension))
            {
                OUString sName = nameFromUrl(aChildren[i]);

                aNodes.emplace_back(new ScriptFile(m_pProviderContext, sName, aChildren[i]));
            }
        }

        css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>> aNodesSequence(
            aNodes.size());
        css::uno::Reference<css::script::browse::XBrowseNode>* pNodesSequence
            = aNodesSequence.getArray();

        for (int i = 0, nCount = aNodes.size(); i < nCount; ++i)
            pNodesSequence[i] = std::move(aNodes[i]);

        return aNodesSequence;
    }
    catch (const css::uno::RuntimeException&)
    {
        throw;
    }
    catch (const css::uno::Exception&)
    {
        return css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>();
    }
}

sal_Bool SAL_CALL ScriptDir::hasChildNodes() { return true; }

sal_Int16 SAL_CALL ScriptDir::getType() { return css::script::browse::BrowseNodeTypes::CONTAINER; }

css::uno::Any SAL_CALL ScriptDir::getPropertyValue(const OUString& sPropertyName)
{
    css::uno::Any xRet;

    if (sPropertyName == "Creatable")
        xRet <<= true;
    else
        xRet = ScriptBrowser::getPropertyValue(sPropertyName);

    return xRet;
}

OUString ScriptDir::nameFromUrl(const OUString& sUrl)
{
    sal_Int32 nSlashPos = sUrl.lastIndexOf('/');
    sal_Int32 nStart = nSlashPos == -1 ? 0 : nSlashPos + 1;

    sal_Int32 nCount = sUrl.getLength() - nStart;

    OUString sExtension = m_pProviderContext->m_pSingleScriptFactory->getExtension();

    if (sUrl.endsWith(sExtension))
        nCount -= sExtension.getLength();

    return rtl::Uri::decode(sUrl.copy(nStart, nCount), rtl_UriDecodeWithCharset,
                            RTL_TEXTENCODING_UTF8);
}

css::uno::Any SAL_CALL ScriptDir::invoke(const OUString& sFunctionName,
                                         const css::uno::Sequence<css::uno::Any>& aParams,
                                         css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                         css::uno::Sequence<css::uno::Any>& aOutParam)
{
    css::uno::Any xRet;

    if (sFunctionName == "Creatable")
    {
        OUString sMacroName;

        if (aParams.getLength() > 0 && (aParams[0] >>= sMacroName))
            xRet <<= createMacro(sMacroName);
    }
    else
        xRet = ScriptBrowser::invoke(sFunctionName, aParams, aOutParamIndex, aOutParam);

    return xRet;
}

sal_Bool SAL_CALL ScriptDir::hasMethod(const OUString& sFunctionName)
{
    if (sFunctionName == "Creatable")
        return true;
    else
        return ScriptBrowser::hasMethod(sFunctionName);
}

css::uno::Reference<css::script::browse::XBrowseNode>
ScriptDir::createMacro(const OUString& sMacroName)
{
    OUString sEncodedFilename = rtl::Uri::encode(sMacroName, rtl_UriCharClassPchar,
                                                 rtl_UriEncodeIgnoreEscapes, RTL_TEXTENCODING_UTF8);
    OUString sUrl = m_sBaseUri + OUStringChar(u'/') + sEncodedFilename
                    + m_pProviderContext->m_pSingleScriptFactory->getExtension();

    css::uno::Reference<css::io::XOutputStream> xOutput
        = m_pProviderContext->m_xFileAccess->openFileWrite(sUrl);

    OUString sSourceCode = m_pProviderContext->m_pSingleScriptFactory->getExampleMacro();
    OString sSourceCodeUtf8 = OUStringToOString(sSourceCode, RTL_TEXTENCODING_UTF8);
    css::uno::Sequence<sal_Int8> sSourceCodeSequence(
        reinterpret_cast<const sal_Int8*>(sSourceCodeUtf8.getStr()), sSourceCodeUtf8.getLength());

    xOutput->writeBytes(sSourceCodeSequence);

    xOutput->closeOutput();

    return new ScriptFile(m_pProviderContext, sMacroName, sUrl);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
