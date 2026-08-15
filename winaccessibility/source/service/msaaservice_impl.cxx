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

#include <rtl/ref.hxx>
#include <sal/log.hxx>
#include <comphelper/OAccessible.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implementationentry.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/accessibility/XMSAAService.hpp>
#include <com/sun/star/accessibility/AccessibleRole.hpp>

#include <com/sun/star/awt/XExtendedToolkit.hpp>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>

#include <prewin.h>
#include <postwin.h>

#include <AccTopWindowListener.hxx>

using namespace ::com::sun::star; // for odk interfaces
using namespace ::com::sun::star::uno; // for basic types
using namespace ::com::sun::star::accessibility;

using namespace ::com::sun::star::awt;

namespace my_sc_impl
{

namespace {

class MSAAServiceImpl : public ::cppu::WeakImplHelper<
            XMSAAService, lang::XServiceInfo >
{
private:
    rtl::Reference<AccTopWindowListener> m_pTopWindowListener;

public:
    MSAAServiceImpl ();

    // XComponent - as used by VCL to lifecycle manage this bridge.
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener( const css::uno::Reference< css::lang::XEventListener >& ) override
    { /* dummy */ }
    virtual void SAL_CALL removeEventListener( const css::uno::Reference< css::lang::XEventListener >& ) override
    { /* dummy */ }

    // XMSAAService
    virtual sal_Int64 SAL_CALL getAccObjectPtr(
            sal_Int64 hWnd, sal_Int64 lParam, sal_Int64 wParam) override;

    void handleWindowOpened(vcl::Window* pWindow);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( OUString const & serviceName ) override;
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};

}

/**
   * Implementation of getAccObjectPtr.
   * @param
   * @return Com interface.
   */
sal_Int64 MSAAServiceImpl::getAccObjectPtr(
        sal_Int64 hWnd, sal_Int64 lParam, sal_Int64 wParam)
{
    // tdf#155794: this must complete without taking SolarMutex

    if (!m_pTopWindowListener.is())
    {
        return 0;
    }
    return m_pTopWindowListener->GetMSComPtr(hWnd, lParam, wParam);
}

/** The method will be invoked when a top window is opened and AT starts up. */
void MSAAServiceImpl::handleWindowOpened(vcl::Window* pWindow)
{
    SolarMutexGuard g;

    SAL_INFO("iacc2", "Window opened " << pWindow);

    if (m_pTopWindowListener.is() && pWindow)
        m_pTopWindowListener->HandleWindowOpened(pWindow);
}

OUString MSAAServiceImpl::getImplementationName()
{
    return "com.sun.star.accessibility.my_sc_implementation.MSAAService";
}

/**
   * Implementation of XServiceInfo, return support service name.
   * @param Service name.
   * @return If the service name is supported.
   */
sal_Bool MSAAServiceImpl::supportsService( OUString const & serviceName )
{
    return cppu::supportsService(this, serviceName);
}

/**
   * Implementation of XServiceInfo, return all service names.
   * @param.
   * @return service name sequence.
   */
Sequence< OUString > MSAAServiceImpl::getSupportedServiceNames()
{
   return { "com.sun.star.accessibility.MSAAService" };
}

/*
 * Setup and notify the OS of Accessible peers for all existing windows.
 */
static void AccessBridgeUpdateOldTopWindows(const rtl::Reference<MSAAServiceImpl>& rpAccMgr)
{
    tools::Long nTopWindowCount = Application::GetTopWindowCount();
    for (tools::Long i = 0; i < nTopWindowCount; i++)
    {
        vcl::Window* pTopWindow = Application::GetTopWindow( i );
        rtl::Reference<comphelper::OAccessible> pAccessible = pTopWindow->GetAccessible();
        if (pAccessible.is() && !pAccessible->getAccessibleName().isEmpty())
            rpAccMgr->handleWindowOpened(pTopWindow);
    }
}

MSAAServiceImpl::MSAAServiceImpl()
{
    Reference< XExtendedToolkit > xToolkit(Application::GetVCLToolkit(), UNO_QUERY);

    if( xToolkit.is() )
    {
        m_pTopWindowListener.set(new AccTopWindowListener());
        Reference<XTopWindowListener> const xRef(m_pTopWindowListener);
        xToolkit->addTopWindowListener( xRef );
        SAL_INFO( "iacc2", "successfully connected to the toolkit event hose" );
    }
    else
        SAL_WARN( "iacc2", "No VCL toolkit interface to listen to for events");
}

void MSAAServiceImpl::dispose()
{
    SolarMutexGuard g;

    // As all folders and streams contain references to their parents,
    // we must remove these references so that they will be deleted when
    // the hash_map of the root folder is cleared, releasing all subfolders
    // and substreams which in turn release theirs, etc. When xRootFolder is
    // released when this destructor completes, the folder tree should be
    // deleted fully (and automagically).
    m_pTopWindowListener.clear();
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
winaccessibility_MSAAServiceImpl_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    rtl::Reference<MSAAServiceImpl> pAccMgr(new MSAAServiceImpl());

    AccessBridgeUpdateOldTopWindows(pAccMgr);

    SAL_INFO("iacc2", "Created new IAccessible2 service impl.");

    pAccMgr->acquire();
    return uno::Reference<XMSAAService>(pAccMgr).get();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
