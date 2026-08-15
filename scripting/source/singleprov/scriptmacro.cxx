/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "scriptmacro.hxx"

#include <com/sun/star/script/browse/BrowseNodeTypes.hpp>
#include <com/sun/star/script/provider/XScriptURIHelper.hpp>
#include <singleprov/singlescriptfactory.hxx>

#include "externaledit.hxx"
#include "provcontext.hxx"

namespace singleprovider
{
ScriptMacro::ScriptMacro(const std::shared_ptr<ProviderContext>& pProviderContext,
                         const OUString& sName, const OUString& sUri)
    : ScriptBrowser(pProviderContext, sName, sUri)
{
}

css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
    SAL_CALL ScriptMacro::getChildNodes()
{
    return css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>();
}

sal_Bool SAL_CALL ScriptMacro::hasChildNodes() { return false; }

sal_Int16 SAL_CALL ScriptMacro::getType() { return css::script::browse::BrowseNodeTypes::SCRIPT; }

css::uno::Any SAL_CALL ScriptMacro::getPropertyValue(const OUString& sPropertyName)
{
    css::uno::Any xRet;

    if (sPropertyName == "URI")
        xRet <<= m_pProviderContext->m_xUriHelper->getScriptURI(m_sBaseUri);
    else if (sPropertyName == "Editable")
        xRet <<= isEditable(m_pProviderContext, m_sBaseUri);
    else
        xRet = ScriptBrowser::getPropertyValue(sPropertyName);

    return xRet;
}

css::uno::Sequence<css::beans::Property> SAL_CALL ScriptMacro::getProperties()
{
    const css::uno::Sequence<css::beans::Property> aParentProperties
        = ScriptBrowser::getProperties();

    css::uno::Sequence<css::beans::Property> aProperties(aParentProperties.getLength() + 1);
    aProperties.getArray()[0] = getUriProperty();

    std::copy(aParentProperties.begin(), aParentProperties.end(), aProperties.getArray() + 1);

    return aProperties;
}

css::beans::Property SAL_CALL ScriptMacro::getPropertyByName(const OUString& sName)
{
    if (sName == "URI")
        return getUriProperty();
    else
        return ScriptBrowser::getPropertyByName(sName);
}

sal_Bool SAL_CALL ScriptMacro::hasPropertyByName(const OUString& sName)
{
    if (sName == "URI")
        return true;
    else
        return ScriptBrowser::hasPropertyByName(sName);
}

css::beans::Property ScriptMacro::getUriProperty()
{
    return css::beans::Property("URI", 0, cppu::UnoType<OUString>::get(), 0);
}

css::uno::Any SAL_CALL ScriptMacro::invoke(const OUString& sFunctionName,
                                           const css::uno::Sequence<css::uno::Any>& aParams,
                                           css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                           css::uno::Sequence<css::uno::Any>& aOutParam)
{
    if (sFunctionName == "Editable")
    {
        externalEdit(m_pProviderContext, m_sBaseUri);
        return css::uno::Any();
    }
    else
        return ScriptBrowser::invoke(sFunctionName, aParams, aOutParamIndex, aOutParam);
}

sal_Bool SAL_CALL ScriptMacro::hasMethod(const OUString& sFunctionName)
{
    if (sFunctionName == "Editable")
        return true;
    else
        return ScriptBrowser::hasMethod(sFunctionName);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
