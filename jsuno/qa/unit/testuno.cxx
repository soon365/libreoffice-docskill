/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>
#include <o3tl/environment.hxx>

#include <jsuno/jsuno.hxx>
#include <rtl/ustring.hxx>

namespace
{
class TestUno : public CppUnit::TestFixture
{
    void appendFileContents(std::u16string_view sFilename, std::string& sOutput);
    void executeScript(std::u16string_view sScriptName);

public:
    void test() { executeScript(u"test.js"); }
    void testMissingParams() { executeScript(u"testmissingparams.js"); }
    void testRhinoCompatibility() { executeScript(u"testrhinocompatibility.js"); }

    CPPUNIT_TEST_SUITE(TestUno);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST(testMissingParams);
    CPPUNIT_TEST(testRhinoCompatibility);
    CPPUNIT_TEST_SUITE_END();
};

void TestUno::appendFileContents(std::u16string_view sFilename, std::string& sOutput)
{
    OUString sSrcPath = o3tl::getEnvironment(u"SRC_ROOT"_ustr);
    CPPUNIT_ASSERT_MESSAGE("SRC_ROOT env variable not set", !sSrcPath.isEmpty());

    // Remove any trailing slash
    sSrcPath.endsWith('/', &sSrcPath);

    OUString sPath = sSrcPath + "/jsuno/qa/extras/" + sFilename;
    OString sPathUtf8 = OUStringToOString(sPath, RTL_TEXTENCODING_UTF8);

    try
    {
        std::ifstream ifs(sPathUtf8.getStr());
        ifs.exceptions(std::ifstream::failbit);
        sOutput.append((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    }
    catch (const std::ios_base::failure& rFail)
    {
        std::cerr << sPathUtf8 << ": " << rFail.what() << std::endl;
        CPPUNIT_FAIL("error loading test script");
    }
}

void TestUno::executeScript(std::u16string_view sScriptName)
{
    std::string sSourceUtf8;

    appendFileContents(u"common.js", sSourceUtf8);
    appendFileContents(sScriptName, sSourceUtf8);

    OUString sSource = OStringToOUString(sSourceUtf8, RTL_TEXTENCODING_UTF8);
    jsuno::execute(sSource);
}

CPPUNIT_TEST_SUITE_REGISTRATION(TestUno);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
