/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/uno/Reference.hxx>
#include <memory>

namespace com::sun::star::script::provider
{
class XScriptURIHelper;
}

namespace com::sun::star::uno
{
class XComponentContext;
}

namespace com::sun::star::ucb
{
class XSimpleFileAccess3;
}

namespace singleprovider
{
class SingleScriptFactory;

class ProviderContext
{
public:
    ProviderContext(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                    const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory);

    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    std::shared_ptr<SingleScriptFactory> m_pSingleScriptFactory;
    css::uno::Reference<css::script::provider::XScriptURIHelper> m_xUriHelper;
    css::uno::Reference<css::ucb::XSimpleFileAccess3> m_xFileAccess;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
