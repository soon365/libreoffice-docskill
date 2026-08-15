/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <jsuno/detail/dllapi.hxx>
#include <rtl/ustring.hxx>

#include <com/sun/star/uno/Reference.hxx>

#include <span>
#include <utility>

namespace com::sun::star::uno
{
class XInterface;
}

namespace jsuno
{
typedef std::span<std::pair<const char*, css::uno::Reference<css::uno::XInterface>>> VariableList;

// @return JSON-stringified result (empty string for a value that JSON.stringify drops: `undefined`,
// a function, or a symbol)
//
// @param aGlobalVariables  A list of variables to define in the global scope in the script.
//
// @throws css.script.provider.ScriptExceptionRaisedException
LO_DLLPUBLIC_JSUNO OUString execute(OUString const& script,
                                    VariableList aGlobalVariables = VariableList());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
