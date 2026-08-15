/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

assertError(function() { console.assert() },
            "Error: missing argument to console.assert");
assertError(function() { uno.type.sequence() },
            "Error: missing argument to uno.type.sequence");
assertError(function() { uno.type.enum() },
            "Error: missing argument to uno.type.enum");
assertError(function() { uno.type.struct() },
            "Error: missing argument to uno.type.struct");
assertError(function() { uno.type.exception() },
            "Error: missing argument to uno.type.exception");
assertError(function() { uno.type.interface() },
            "Error: missing argument to uno.type.interface");
assertError(function() { uno.idl.org.libreoffice.embindtest.theSingleton() },
            "Error: missing argument to getSingleton");
assertError(function() { new uno.Any() },
            "Error: missing argument to uno.Any constructor");
assertError(function() { uno.sameUnoObject() },
            "Error: missing argument to uno.sameUnoObject");
assertError(function() { uno.sameUnoObject(1) },
            "Error: missing argument to uno.sameUnoObject");
