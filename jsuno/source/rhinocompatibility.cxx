/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include "rhinocompatibility.hxx"

#include <array>
#include <sal/macros.h>

#include "jsvalue.hxx"

namespace jsuno::rhino_compatibility
{
namespace
{
JSValue importClass(JSContext* pContext, JSValueConst, int, JSValueConst* argv)
{
    JSAtom pGetImportClassName = JS_NewAtom(pContext, "getImportClassName");

    jsuno::ValueRef sShortName(pContext,
                               JS_Invoke(pContext, argv[0], pGetImportClassName, 0, nullptr));
    JS_FreeAtom(pContext, pGetImportClassName);

    if (JS_IsException(sShortName))
        return JS_EXCEPTION;

    JSAtom pAtom = JS_ValueToAtom(pContext, sShortName);
    if (pAtom == JS_ATOM_NULL)
        return JS_EXCEPTION;

    const jsuno::ValueRef xGlobal(pContext, JS_GetGlobalObject(pContext));
    int nRet = JS_SetProperty(pContext, xGlobal, pAtom, JS_DupValue(pContext, argv[0]));
    JS_FreeAtom(pContext, pAtom);

    return nRet == -1 ? JS_EXCEPTION : JS_UNDEFINED;
}

JSValue unoRuntimeGetImportClassName(JSContext* pContext, JSValueConst, int, JSValueConst*)
{
    return JS_NewString(pContext, "UnoRuntime");
}

JSValue queryInterface(JSContext* pContext, JSValueConst, int, JSValueConst* argv)
{
    // Redirect UnoRuntime.queryInterface(a, b) to b.queryInterface(uno.type.interface(a))

    jsuno::ValueRef xUnoTypeInterface(pContext, JS_GetGlobalObject(pContext));

    for (const auto part : std::array<const char*, 3>{ "uno", "type", "interface" })
    {
        JSAtom pAtom = JS_NewAtom(pContext, part);
        xUnoTypeInterface = JS_GetProperty(pContext, xUnoTypeInterface, pAtom);
        JS_FreeAtom(pContext, pAtom);

        if (JS_IsException(xUnoTypeInterface))
            return JS_EXCEPTION;
    }

    jsuno::ValueRef xType(pContext, JS_Call(pContext, xUnoTypeInterface, JS_NULL, 1, argv));
    if (JS_IsException(xType))
        return JS_EXCEPTION;

    JSAtom pAtom = JS_NewAtom(pContext, "queryInterface");
    JSValueConst args[] = { xType };
    jsuno::ValueRef xRet(pContext, JS_Invoke(pContext, argv[1], pAtom, SAL_N_ELEMENTS(args), args));
    JS_FreeAtom(pContext, pAtom);

    return xRet.release();
}
}

void setUp(JSContext* pContext)
{
    // Make “Packages” an alias for “uno.idl”
    const jsuno::ValueRef xGlobal(pContext, JS_GetGlobalObject(pContext));
    const jsuno::ValueRef xUno(pContext, JS_GetPropertyStr(pContext, xGlobal, "uno"));
    assert(JS_IsObject(xUno));
    jsuno::ValueRef xIdl(pContext, JS_GetPropertyStr(pContext, xUno, "idl"));
    assert(JS_IsObject(xIdl));
    JS_SetPropertyStr(pContext, xGlobal, "Packages", xIdl.release());

    jsuno::ValueRef xImportClass(pContext,
                                 JS_NewCFunction(pContext, importClass, "importClass", 1));
    JS_SetPropertyStr(pContext, xGlobal, "importClass", xImportClass.release());
}

jsuno::ValueRef createUnoRuntime(JSContext* pContext)
{
    jsuno::ValueRef xObject(pContext, JS_NewObject(pContext));
    if (JS_IsException(xObject))
        return xObject;

    static const JSCFunctionListEntry functions[] = {
        JS_CFUNC_DEF("queryInterface", 2, queryInterface),
        JS_CFUNC_DEF("getImportClassName", 0, unoRuntimeGetImportClassName),
    };

    ValueRef proto(pContext, JS_NewObject(pContext));
    JS_SetPropertyFunctionList(pContext, xObject, functions, SAL_N_ELEMENTS(functions));

    return xObject;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
