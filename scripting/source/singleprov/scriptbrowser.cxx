/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "scriptbrowser.hxx"

#include <com/sun/star/script/browse/BrowseNodeTypes.hpp>

#include <singleprov/singlescriptfactory.hxx>

#include "externaledit.hxx"
#include "provcontext.hxx"

namespace singleprovider
{
ScriptBrowser::ScriptBrowser(const std::shared_ptr<ProviderContext>& pProviderContext,
                             const OUString& sName, const OUString& sBaseUri)
    : m_pProviderContext(pProviderContext)
    , m_sName(sName)
    , m_sBaseUri(sBaseUri)
{
}

OUString SAL_CALL ScriptBrowser::getName() { return m_sName; }

css::uno::Reference<css::beans::XPropertySetInfo> SAL_CALL ScriptBrowser::getPropertySetInfo()
{
    return this;
}

void SAL_CALL ScriptBrowser::setPropertyValue(const OUString&, const css::uno::Any&) {}

css::uno::Any SAL_CALL ScriptBrowser::getPropertyValue(const OUString& sPropertyName)
{
    css::uno::Any xRet;

    if (sPropertyName == "Editable" || sPropertyName == "Creatable")
        xRet <<= false;
    else
    {
        throw css::beans::UnknownPropertyException("Tried to get unknown property "
                                                   + sPropertyName);
    }

    return xRet;
}

void SAL_CALL ScriptBrowser::addPropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ScriptBrowser::removePropertyChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XPropertyChangeListener>&)
{
}

void SAL_CALL ScriptBrowser::addVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

void SAL_CALL ScriptBrowser::removeVetoableChangeListener(
    const OUString&, const css::uno::Reference<css::beans::XVetoableChangeListener>&)
{
}

css::uno::Sequence<css::beans::Property> SAL_CALL ScriptBrowser::getProperties()
{
    css::uno::Sequence<css::beans::Property> aProperties(2);
    aProperties.getArray()[0] = getEditableProperty();
    aProperties.getArray()[1] = getCreatableProperty();
    return aProperties;
}

css::beans::Property SAL_CALL ScriptBrowser::getPropertyByName(const OUString& sName)
{
    if (sName == "Editable")
        return getEditableProperty();
    else if (sName == "Creatable")
        return getCreatableProperty();
    else
        throw css::beans::UnknownPropertyException("Tried to retrieve unknown property " + sName);
}

sal_Bool SAL_CALL ScriptBrowser::hasPropertyByName(const OUString& sName)
{
    return sName == "Editable" || sName == "Creatable";
}

css::beans::Property ScriptBrowser::getEditableProperty()
{
    return css::beans::Property("Editable", 0, cppu::UnoType<sal_Bool>::get(), 0);
}

css::beans::Property ScriptBrowser::getCreatableProperty()
{
    return css::beans::Property("Creatable", 0, cppu::UnoType<sal_Bool>::get(), 0);
}

css::uno::Reference<css::beans::XIntrospectionAccess> SAL_CALL ScriptBrowser::getIntrospection()
{
    return css::uno::Reference<css::beans::XIntrospectionAccess>();
}

css::uno::Any SAL_CALL ScriptBrowser::invoke(const OUString& sMethodName,
                                             const css::uno::Sequence<css::uno::Any>&,
                                             css::uno::Sequence<sal_Int16>&,
                                             css::uno::Sequence<css::uno::Any>&)
{
    throw css::lang::IllegalArgumentException("Tried to invoke unknown method " + sMethodName,
                                              getXWeak(), 0);
}

void SAL_CALL ScriptBrowser::setValue(const OUString&, const css::uno::Any&) {}

css::uno::Any SAL_CALL ScriptBrowser::getValue(const OUString& sName)
{
    throw css::beans::UnknownPropertyException("Tried to get unknown property " + sName);
}

sal_Bool SAL_CALL ScriptBrowser::hasMethod(const OUString&) { return false; }

sal_Bool SAL_CALL ScriptBrowser::hasProperty(const OUString&) { return false; }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
