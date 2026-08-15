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
#include <rtl/ustring.hxx>
#include <string_view>

namespace com::sun::star::script::provider
{
class XScript;
}

namespace com::sun::star::uno
{
class XComponentContext;
}

namespace singleprovider
{
// Abstract class to provide callbacks into the real script provider. The provider should be able to
// just provide an implementation of the class and pass it to singleprov::ScriptBrowser.
class SingleScriptFactory
{
public:
    virtual OUString getLanguageName() const = 0;
    // Gets the extensions (including the ‘.’) for files that this factory is interested in
    virtual OUString getExtension() const = 0;
    // Given the source code, returns the XScript for the macro contained within
    virtual css::uno::Reference<css::script::provider::XScript>
    getScript(css::uno::Reference<css::uno::XComponentContext> xContext, const OUString& sName,
              std::string_view sSource) const = 0;
    // Given the basename of an executable, returns whether it looks like it might be an interpreter
    // for the given language. This is used on Windows to try to avoid invoking a default app that
    // is actually just going to run the script instead of editing it.
    virtual bool appMightExecute(const OUString& sAppName) const;
    // Get a string that will be inserted into a new source file as an example when the create
    // button is pressed.
    virtual OUString getExampleMacro() const { return OUString(); }

    virtual ~SingleScriptFactory() = default;
};

inline bool SingleScriptFactory::appMightExecute(const OUString&) const { return false; }
}
