/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "scriptfile.hxx"

#include <com/sun/star/script/browse/BrowseNodeTypes.hpp>
#include <com/sun/star/ucb/SimpleFileAccess.hpp>

#include <singleprov/singlescriptfactory.hxx>

#include "externaledit.hxx"
#include "provcontext.hxx"
#include "scriptmacro.hxx"

namespace singleprovider
{
ScriptFile::ScriptFile(const std::shared_ptr<ProviderContext>& pProviderContext,
                       const OUString& sName, const OUString& sBaseUri)
    : ScriptBrowser(pProviderContext, sName, sBaseUri)
{
}

css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>>
    SAL_CALL ScriptFile::getChildNodes()
{
    // Each file is treated as a container with a single macro inside it
    css::uno::Sequence<css::uno::Reference<css::script::browse::XBrowseNode>> aChild(1);

    aChild.getArray()[0].set(new ScriptMacro(m_pProviderContext, m_sName, m_sBaseUri));

    return aChild;
}

sal_Bool SAL_CALL ScriptFile::hasChildNodes() { return true; }

sal_Int16 SAL_CALL ScriptFile::getType() { return css::script::browse::BrowseNodeTypes::CONTAINER; }

css::uno::Any SAL_CALL ScriptFile::getPropertyValue(const OUString& sPropertyName)
{
    css::uno::Any xRet;

    if (sPropertyName == "Editable")
        xRet <<= isEditable(m_pProviderContext, m_sBaseUri);
    else
        xRet = ScriptBrowser::getPropertyValue(sPropertyName);

    return xRet;
}

css::uno::Any SAL_CALL ScriptFile::invoke(const OUString& sFunctionName,
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

sal_Bool SAL_CALL ScriptFile::hasMethod(const OUString& sFunctionName)
{
    if (sFunctionName == "Editable")
        return true;
    else
        return ScriptBrowser::hasMethod(sFunctionName);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
