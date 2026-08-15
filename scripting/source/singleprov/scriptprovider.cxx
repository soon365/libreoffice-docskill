/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <singleprov/scriptprovider.hxx>

#include <com/sun/star/script/provider/ScriptFrameworkErrorException.hpp>
#include <com/sun/star/script/provider/ScriptFrameworkErrorType.hpp>
#include <com/sun/star/script/provider/ScriptURIHelper.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>
#include <vector>

#include <singleprov/singlescriptfactory.hxx>

#include "provcontext.hxx"
#include "scriptdir.hxx"

namespace singleprovider
{
ScriptProvider::ScriptProvider(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                               const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory)
    : m_pProviderContext(std::make_shared<ProviderContext>(xContext, pSingleScriptFactory))
{
}

void SAL_CALL ScriptProvider::initialize(const css::uno::Sequence<css::uno::Any>& aArguments)
{
    if (aArguments.getLength() > 0)
    {
        OUString sLocation;
        if (aArguments[0] >>= sLocation)
            setLocation(sLocation);
    }
}

void ScriptProvider::setLocation(const OUString& sLocation)
{
    OUString sLanguageName = m_pProviderContext->m_pSingleScriptFactory->getLanguageName();

    m_pProviderContext->m_xUriHelper = css::script::provider::ScriptURIHelper::create(
        m_pProviderContext->m_xContext, sLanguageName, sLocation);
    m_xRootBrowser
        = css::uno::Reference<css::script::browse::XBrowseNode>(new ScriptDir(m_pProviderContext));
}

OUString SAL_CALL ScriptProvider::getName()
{
    return m_pProviderContext->m_pSingleScriptFactory->getLanguageName();
}

css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
    SAL_CALL ScriptProvider::getChildNodes()
{
    if (m_xRootBrowser.is())
        return m_xRootBrowser->getChildNodes();
    else
        return css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>();
}

sal_Bool SAL_CALL ScriptProvider::hasChildNodes()
{
    return m_xRootBrowser.is() && m_xRootBrowser->hasChildNodes();
}

sal_Int16 SAL_CALL ScriptProvider::getType()
{
    return m_xRootBrowser.is() ? m_xRootBrowser->getType() : 0;
}

css::uno::Reference<css::script::provider::XScript>
    SAL_CALL ScriptProvider::getScript(const OUString& sScriptUri)
{
    OUString sStorageUri = m_pProviderContext->m_xUriHelper->getStorageURI(sScriptUri);

    sal_Int32 nSlashPos = sStorageUri.lastIndexOf('/');
    OUString sName = sStorageUri.copy(nSlashPos == -1 ? 0 : nSlashPos + 1);

    try
    {
        std::vector<char> aSource;
        constexpr sal_Int32 nBufSize = 1024;
        css::uno::Sequence<sal_Int8> aBuf(nBufSize);

        css::uno::Reference<css::io::XInputStream> xIn
            = m_pProviderContext->m_xFileAccess->openFileRead(sStorageUri);

        while (true)
        {
            sal_Int32 nBytesRead = xIn->readBytes(aBuf, aBuf.getLength());
            const char* pBuf = reinterpret_cast<const char*>(aBuf.getConstArray());
            aSource.insert(aSource.end(), pBuf, pBuf + nBytesRead);

            if (nBytesRead < nBufSize)
                break;
        }

        return m_pProviderContext->m_pSingleScriptFactory->getScript(
            m_pProviderContext->m_xContext, sName,
            std::string_view(aSource.begin(), aSource.end()));
    }
    catch (const css::uno::Exception& e)
    {
        throw css::script::provider::ScriptFrameworkErrorException(
            e.Message, static_cast<css::uno::XWeak*>(this), sName,
            m_pProviderContext->m_pSingleScriptFactory->getLanguageName(),
            css::script::provider::ScriptFrameworkErrorType::UNKNOWN);
    }
}

css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL ScriptProvider::getPropertySetInfo()
{
    css::uno::Reference<css::beans::XPropertySetInfo> xPropertySetInfo;

    css::uno::Reference<css::beans::XPropertySet> xPropertySet(m_xRootBrowser, css::uno::UNO_QUERY);

    if (xPropertySet.is())
        xPropertySetInfo = xPropertySet->getPropertySetInfo();

    return xPropertySetInfo;
}

void SAL_CALL ScriptProvider::setPropertyValue(const OUString&, const css::uno::Any&) {}

css::uno::Any SAL_CALL ScriptProvider::getPropertyValue(const OUString& sPropertyName)
{
    css::uno::Any xRet;

    css::uno::Reference<css::beans::XPropertySet> xPropertySet(m_xRootBrowser, css::uno::UNO_QUERY);

    if (xPropertySet.is())
        xRet = xPropertySet->getPropertyValue(sPropertyName);

    return xRet;
}

void SAL_CALL ScriptProvider::addPropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ScriptProvider::removePropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ScriptProvider::addVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

void SAL_CALL ScriptProvider::removeVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

css::uno::Reference<css::beans::XIntrospectionAccess> SAL_CALL ScriptProvider::getIntrospection()
{
    return css::uno::Reference<css::beans::XIntrospectionAccess>();
}

css::uno::Any SAL_CALL ScriptProvider::invoke(const OUString& sFunctionName,
                                              const css::uno::Sequence<css::uno::Any>& aParams,
                                              css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                              css::uno::Sequence<css::uno::Any>& aOutParam)
{
    css::uno::Any xRet;

    css::uno::Reference<css::script::XInvocation> xInvocation(m_xRootBrowser, css::uno::UNO_QUERY);

    if (xInvocation.is())
        xRet = xInvocation->invoke(sFunctionName, aParams, aOutParamIndex, aOutParam);

    return xRet;
}

void SAL_CALL ScriptProvider::setValue(const OUString&, const css::uno::Any&) {}

css::uno::Any SAL_CALL ScriptProvider::getValue(const OUString&) { return css::uno::Any(); }

sal_Bool SAL_CALL ScriptProvider::hasMethod(const OUString& sName)
{
    css::uno::Reference<css::script::XInvocation> xInvocation(m_xRootBrowser, css::uno::UNO_QUERY);

    if (xInvocation.is())
        return xInvocation->hasMethod(sName);
    else
        return false;
}

sal_Bool SAL_CALL ScriptProvider::hasProperty(const OUString&) { return false; }

OUString SAL_CALL ScriptProvider::getImplementationName()
{
    return getImplementationNameStatic(
        m_pProviderContext->m_pSingleScriptFactory->getLanguageName());
}

sal_Bool SAL_CALL ScriptProvider::supportsService(OUString const& serviceName)
{
    const css::uno::Sequence<OUString> names = getSupportedServiceNames();
    const OUString* pNames = names.getConstArray();

    for (sal_Int32 i = 0, count = names.getLength(); i < count; i++)
    {
        if (pNames[i] == serviceName)
            return true;
    }

    return false;
}

css::uno::Sequence<OUString> SAL_CALL ScriptProvider::getSupportedServiceNames()
{
    return getSupportedServiceNamesStatic(
        m_pProviderContext->m_pSingleScriptFactory->getLanguageName());
}

SAL_DLLPUBLIC_EXPORT OUString
ScriptProvider::getImplementationNameStatic(std::u16string_view sLanguageName)
{
    return u"com.sun.star.singleprov.ScriptProviderFor"_ustr + sLanguageName;
}

SAL_DLLPUBLIC_EXPORT css::uno::Sequence<OUString>
ScriptProvider::getSupportedServiceNamesStatic(std::u16string_view sLanguageName)
{
    css::uno::Sequence<OUString> names(2);
    OUString* pNames = names.getArray();
    pNames[0] = u"com.sun.star.script.provider.ScriptProviderFor"_ustr + sLanguageName;
    pNames[1] = "com.sun.star.script.provider.LanguageScriptProvider";
    return names;
}

SAL_DLLPUBLIC_EXPORT css::uno::Reference<css::uno::XInterface>
ScriptProvider::create(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                       const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory)
{
    return static_cast<css::lang::XTypeProvider*>(
        new ScriptProvider(xContext, pSingleScriptFactory));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
