/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/script/browse/XBrowseNode.hpp>
#include <com/sun/star/script/provider/XScriptProvider.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <cppuhelper/implbase.hxx>
#include <memory>

namespace singleprovider
{
class ProviderContext;
class ScriptBrowser;
class SingleScriptFactory;

class ScriptProvider
    : public cppu::WeakImplHelper<css::lang::XInitialization, css::script::browse::XBrowseNode,
                                  css::beans::XPropertySet, css::script::XInvocation,
                                  css::script::provider::XScriptProvider, css::lang::XServiceInfo>
{
public:
    // XInitialization
    void SAL_CALL initialize(const css::uno::Sequence<css::uno::Any>& aArguments) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    sal_Bool SAL_CALL supportsService(OUString const& serviceName) override;
    css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    // XBrowseNode
    OUString SAL_CALL getName() override;
    css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
        SAL_CALL getChildNodes() override;
    sal_Bool SAL_CALL hasChildNodes() override;
    sal_Int16 SAL_CALL getType() override;

    // XScriptProvider
    css::uno::Reference<css::script::provider::XScript>
        SAL_CALL getScript(const OUString& sScriptUri) override;

    // XPropertySet
    css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL getPropertySetInfo() override;
    void SAL_CALL setPropertyValue(const OUString& aPropertyName,
                                   const css::uno::Any& aValue) override;
    css::uno::Any SAL_CALL getPropertyValue(const OUString& sPropertyName) override;
    void SAL_CALL addPropertyChangeListener(
        const OUString& aPropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& xListener) override;
    void SAL_CALL removePropertyChangeListener(
        const OUString& aPropertyName,
        const css::uno::Reference<css::beans::XPropertyChangeListener>& aListener) override;
    void SAL_CALL addVetoableChangeListener(
        const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;
    void SAL_CALL removeVetoableChangeListener(
        const OUString& PropertyName,
        const css::uno::Reference<css::beans::XVetoableChangeListener>& aListener) override;

    // XInvocation
    css::uno::Reference<css::beans::XIntrospectionAccess> SAL_CALL getIntrospection() override;
    css::uno::Any SAL_CALL invoke(const OUString& sFunctionName,
                                  const css::uno::Sequence<css::uno::Any>& aParams,
                                  css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                  css::uno::Sequence<css::uno::Any>& aOutParam) override;
    void SAL_CALL setValue(const OUString& sPropertyName, const css::uno::Any& aValue) override;
    css::uno::Any SAL_CALL getValue(const OUString& sPropertyName) override;
    sal_Bool SAL_CALL hasMethod(const OUString& sName) override;
    sal_Bool SAL_CALL hasProperty(const OUString& sName) override;

    static SAL_DLLPUBLIC_EXPORT OUString
    getImplementationNameStatic(std::u16string_view sLanguageName);
    static SAL_DLLPUBLIC_EXPORT css::uno::Sequence<OUString>
    getSupportedServiceNamesStatic(std::u16string_view sLanguageName);
    static SAL_DLLPUBLIC_EXPORT css::uno::Reference<css::uno::XInterface>
    create(const css::uno::Reference<css::uno::XComponentContext>& xContext,
           const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory);

private:
    ScriptProvider(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                   const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory);

    void setLocation(const OUString& sLocation);

    std::shared_ptr<ProviderContext> m_pProviderContext;
    css::uno::Reference<css::script::browse::XBrowseNode> m_xRootBrowser;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
