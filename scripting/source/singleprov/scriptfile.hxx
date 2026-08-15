/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "scriptbrowser.hxx"

namespace singleprovider
{
class ScriptFile : public ScriptBrowser
{
public:
    ScriptFile(const std::shared_ptr<ProviderContext>& pProviderContext, const OUString& sName,
               const OUString& sBaseUri);

    // XBrowseNode
    css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
        SAL_CALL getChildNodes() override;
    sal_Bool SAL_CALL hasChildNodes() override;
    sal_Int16 SAL_CALL getType() override;

    // XPropertySet
    css::uno::Any SAL_CALL getPropertyValue(const OUString& sPropertyName) override;

    // XInvocation
    css::uno::Any SAL_CALL invoke(const OUString& sFunctionName,
                                  const css::uno::Sequence<css::uno::Any>& aParams,
                                  css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                  css::uno::Sequence<css::uno::Any>& aOutParam) override;
    sal_Bool SAL_CALL hasMethod(const OUString& sFunctionName) override;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
