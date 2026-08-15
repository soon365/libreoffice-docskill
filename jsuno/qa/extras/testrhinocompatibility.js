/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Check that we can access UNO entities with the Packages alias
console.assert(Packages.org.libreoffice.embindtest.Constants.Short === -1234);

assertError(function() { importClass() },
            "TypeError: cannot read property 'getImportClassName' of undefined");

console.assert(Packages.org.libreoffice.embindtest.XTest.getImportClassName() == "XTest");
console.assert(Packages.org.libreoffice.embindtest.Enum.getImportClassName() == "Enum");
console.assert(Packages.org.getImportClassName() == "org");
console.assert(Packages.org.libreoffice.embindtest.getImportClassName() == "embindtest");

importClass(Packages.org.libreoffice.embindtest.XTest);
console.assert(XTest == Packages.org.libreoffice.embindtest.XTest);
importClass(Packages.org.libreoffice.embindtest.Enum);
console.assert(Enum == Packages.org.libreoffice.embindtest.Enum);
importClass(Packages.org.libreoffice.embindtest);
console.assert(embindtest == Packages.org.libreoffice.embindtest);

importClass(Packages.com.sun.star.uno.UnoRuntime);

let test = uno.idl.org.libreoffice.embindtest.Test.create(uno.componentContext);
let xTest = UnoRuntime.queryInterface(XTest, test);
console.assert(uno.sameUnoObject(xTest, test));

console.assert(UnoRuntime.queryInterface(XTest, uno.componentContext) === undefined);
