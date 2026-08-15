/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "provcontext.hxx"

#include <com/sun/star/ucb/SimpleFileAccess.hpp>

namespace singleprovider
{
ProviderContext::ProviderContext(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                                 const std::shared_ptr<SingleScriptFactory>& pSingleScriptFactory)
    : m_xContext(xContext)
    , m_pSingleScriptFactory(pSingleScriptFactory)
    , m_xFileAccess(css::ucb::SimpleFileAccess::create(xContext))
{
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
