/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/script/provider/XScript.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <cppuhelper/implbase.hxx>

#include <jsuno/jsuno.hxx>

#include <singleprov/scriptprovider.hxx>
#include <singleprov/singlescriptfactory.hxx>
#include <util/ScriptContext.hxx>

namespace
{
class ScriptWrapper : public cppu::WeakImplHelper<css::script::provider::XScript>
{
public:
    ScriptWrapper(const css::uno::Reference<css::uno::XComponentContext>& xContext,
                  const OUString& sScript)
        : m_xScriptContext(new sf_misc::ScriptContext(xContext))
        , m_sScript(sScript)
    {
    }

    // XScript
    css::uno::Any SAL_CALL invoke(const css::uno::Sequence<css::uno::Any>& aParams,
                                  css::uno::Sequence<sal_Int16>& aOutParamIndex,
                                  css::uno::Sequence<css::uno::Any>& aOutParam) override;

private:
    css::uno::Reference<css::script::provider::XScriptContext> m_xScriptContext;
    OUString m_sScript;
};

class JsProvScriptFactory : public singleprovider::SingleScriptFactory
{
    OUString getLanguageName() const override;
    OUString getExtension() const override;
    css::uno::Reference<css::script::provider::XScript>
    getScript(css::uno::Reference<css::uno::XComponentContext> xContext, const OUString& sName,
              std::string_view sSource) const override;
    bool appMightExecute(const OUString& sAppName) const override;
    OUString getExampleMacro() const override;
};

OUString JsProvScriptFactory::getLanguageName() const { return "JavaScript"; }

OUString JsProvScriptFactory::getExtension() const { return ".js"; }

css::uno::Reference<css::script::provider::XScript>
JsProvScriptFactory::getScript(css::uno::Reference<css::uno::XComponentContext> xContext,
                               const OUString&, std::string_view sSource) const
{
    return new ScriptWrapper(xContext, OStringToOUString(sSource, RTL_TEXTENCODING_UTF8));
}

bool JsProvScriptFactory::appMightExecute(const OUString& sAppName) const
{
    return sAppName.startsWithIgnoreAsciiCase("js.") || sAppName.startsWithIgnoreAsciiCase("node.");
}

OUString JsProvScriptFactory::getExampleMacro() const
{
    return u"let text = XSCRIPTCONTEXT.getDocument().getText();\n"
           "\n"
           "text.getEnd().setString(\"Hello, world!\");\n"_ustr;
}

css::uno::Any SAL_CALL ScriptWrapper::invoke(const css::uno::Sequence<css::uno::Any>&,
                                             css::uno::Sequence<sal_Int16>&,
                                             css::uno::Sequence<css::uno::Any>&)
{
    std::pair<const char*, css::uno::Reference<css::uno::XInterface>> aGlobalVariables[] = {
        { "XSCRIPTCONTEXT", m_xScriptContext },
    };

    jsuno::execute(m_sScript, aGlobalVariables);

    return css::uno::Any();
}
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
scripting_JavaScriptProviderImpl_get_implementation(css::uno::XComponentContext* pContext,
                                                    css::uno::Sequence<css::uno::Any> const&)
{
    css::uno::Reference<css::uno::XInterface> xProvider
        = singleprovider::ScriptProvider::create(pContext, std::make_shared<JsProvScriptFactory>());
    xProvider->acquire();
    return xProvider.get();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
