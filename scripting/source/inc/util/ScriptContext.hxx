/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/frame/theDesktop.hpp>
#include <com/sun/star/script/provider/XScriptContext.hpp>
#include <cppuhelper/implbase.hxx>

namespace sf_misc
{
class ScriptContext : public cppu::WeakImplHelper<css::script::provider::XScriptContext>
{
public:
    explicit ScriptContext(
        const css::uno::Reference<css::uno::XComponentContext>& xContext,
        const css::uno::Reference<css::frame::XModel>& xDocument
        = css::uno::Reference<css::frame::XModel>(),
        const css::uno::Reference<css::document::XScriptInvocationContext>& xInvocation
        = css::uno::Reference<css::document::XScriptInvocationContext>())
        : m_xContext(xContext)
        , m_xDocument(xDocument)
        , m_xInvocation(xInvocation)
    {
        assert(m_xContext.is());
    }

    // XScriptContext
    css::uno::Reference<css::frame::XModel> SAL_CALL getDocument() override
    {
        if (m_xDocument.is())
            return m_xDocument;
        else
        {
            css::uno::Reference<css::frame::XModel> xDocument(getDesktop()->getCurrentComponent(),
                                                              css::uno::UNO_QUERY);
            return xDocument;
        }
    }

    css::uno::Reference<css::document::XScriptInvocationContext>
        SAL_CALL getInvocationContext() override
    {
        return m_xInvocation;
    }

    css::uno::Reference<css::frame::XDesktop> SAL_CALL getDesktop() override
    {
        return css::frame::theDesktop::get(m_xContext);
    }

    css::uno::Reference<css::uno::XComponentContext> SAL_CALL getComponentContext() override
    {
        return m_xContext;
    }

private:
    css::uno::Reference<css::uno::XComponentContext> m_xContext;
    css::uno::Reference<css::frame::XModel> m_xDocument;
    css::uno::Reference<css::document::XScriptInvocationContext> m_xInvocation;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
