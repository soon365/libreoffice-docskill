/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <rtl/string.hxx>

namespace {

class Test: public CppUnit::TestFixture {
private:
    void testStartsWithIgnoreAsciiCase();
    void testCompareTo();
    void testUtf8StringLiterals();
    void testStartsWithChar();
    void testEndsWithChar();

    CPPUNIT_TEST_SUITE(Test);
    CPPUNIT_TEST(testStartsWithIgnoreAsciiCase);
    CPPUNIT_TEST(testCompareTo);
    CPPUNIT_TEST(testUtf8StringLiterals);
    CPPUNIT_TEST(testStartsWithChar);
    CPPUNIT_TEST(testEndsWithChar);
    CPPUNIT_TEST_SUITE_END();
};

void Test::testStartsWithIgnoreAsciiCase() {
    {
        OString r;
        CPPUNIT_ASSERT(OString().startsWithIgnoreAsciiCase(std::string_view(), &r));
        CPPUNIT_ASSERT(r.isEmpty());
    }
    {
        OString r;
        CPPUNIT_ASSERT("foo"_ostr.startsWithIgnoreAsciiCase(std::string_view(), &r));
        CPPUNIT_ASSERT_EQUAL("foo"_ostr, r);
    }
    {
        OString r;
        CPPUNIT_ASSERT(
            "foo"_ostr.startsWithIgnoreAsciiCase("F", &r));
        CPPUNIT_ASSERT_EQUAL("oo"_ostr, r);
    }
    {
        OString r("other"_ostr);
        CPPUNIT_ASSERT(
            !"foo"_ostr.startsWithIgnoreAsciiCase("bar", &r));
        CPPUNIT_ASSERT_EQUAL("other"_ostr, r);
    }
    {
        OString r("other"_ostr);
        CPPUNIT_ASSERT(
            !"foo"_ostr.startsWithIgnoreAsciiCase("foobar", &r));
        CPPUNIT_ASSERT_EQUAL("other"_ostr, r);
    }

    {
        OString r;
        CPPUNIT_ASSERT(OString().startsWithIgnoreAsciiCase("", &r));
        CPPUNIT_ASSERT(r.isEmpty());
    }
    {
        OString r;
        CPPUNIT_ASSERT("foo"_ostr.startsWithIgnoreAsciiCase("", &r));
        CPPUNIT_ASSERT_EQUAL("foo"_ostr, r);
    }
    {
        OString r;
        CPPUNIT_ASSERT(
            "foo"_ostr.startsWithIgnoreAsciiCase("F", &r));
        CPPUNIT_ASSERT_EQUAL("oo"_ostr, r);
    }
    {
        OString r("other"_ostr);
        CPPUNIT_ASSERT(
            !"foo"_ostr.startsWithIgnoreAsciiCase("bar", &r));
        CPPUNIT_ASSERT_EQUAL("other"_ostr, r);
    }
    {
        OString r("other"_ostr);
        CPPUNIT_ASSERT(
            !"foo"_ostr.startsWithIgnoreAsciiCase("foobar", &r));
        CPPUNIT_ASSERT_EQUAL("other"_ostr, r);
    }
}

void Test::testCompareTo()
{
    // test that embedded NUL does not stop the compare
    char str1[2] = { '\0', 'x' };
    char str2[2] = { '\0', 'y' };

    OString s1(str1, 2);
    OString s2(str2, 2);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), s1.compareTo(s1));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), s2.compareTo(s2));
    CPPUNIT_ASSERT(s1.compareTo(s2) < 0);
    CPPUNIT_ASSERT(s2.compareTo(s1) > 0);
    CPPUNIT_ASSERT(s1.compareTo(OString(s2 + "y")) < 0);
    CPPUNIT_ASSERT(s2.compareTo(OString(s1 + "x")) > 0);
    CPPUNIT_ASSERT(OString(s1 + "x").compareTo(s2) < 0);
    CPPUNIT_ASSERT(OString(s2 + "y").compareTo(s1) > 0);
}

void Test::testUtf8StringLiterals()
{
    OString sIn(u8"ßa"_ostr);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), sIn.getLength());
    CPPUNIT_ASSERT_EQUAL(195, int(static_cast<unsigned char>(sIn[0])));
    CPPUNIT_ASSERT_EQUAL(159, int(static_cast<unsigned char>(sIn[1])));
    CPPUNIT_ASSERT_EQUAL(97, int(static_cast<unsigned char>(sIn[2])));
}

void Test::testStartsWithChar()
{
    CPPUNIT_ASSERT(!""_ostr.startsWith('.'));
    CPPUNIT_ASSERT("."_ostr.startsWith('.'));
    CPPUNIT_ASSERT(".foo"_ostr.startsWith('.'));
    CPPUNIT_ASSERT(!"."_ostr.startsWith('?'));
    CPPUNIT_ASSERT(!".foo"_ostr.startsWith('?'));

    {
        OString rest = "not_changed"_ostr;
        CPPUNIT_ASSERT(!"foo"_ostr.startsWith('p', &rest));
        CPPUNIT_ASSERT_EQUAL("not_changed"_ostr, rest);
    }

    {
        OString rest = "must_change"_ostr;
        CPPUNIT_ASSERT("/removed_slash"_ostr.startsWith('/', &rest));
        CPPUNIT_ASSERT_EQUAL("removed_slash"_ostr, rest);
    }

    {
        std::string_view rest = "not_changed";
        CPPUNIT_ASSERT(!"foo"_ostr.startsWith('p', &rest));
        CPPUNIT_ASSERT_EQUAL("not_changed"_ostr, OString(rest));
    }

    {
        std::string_view rest = "must_change";
        CPPUNIT_ASSERT("/removed_slash"_ostr.startsWith('/', &rest));
        CPPUNIT_ASSERT_EQUAL("removed_slash"_ostr, OString(rest));
    }
}

void Test::testEndsWithChar()
{
    CPPUNIT_ASSERT(!""_ostr.endsWith('.'));
    CPPUNIT_ASSERT("."_ostr.endsWith('.'));
    CPPUNIT_ASSERT("foo."_ostr.endsWith('.'));
    CPPUNIT_ASSERT(!"."_ostr.endsWith('?'));
    CPPUNIT_ASSERT(!"foo."_ostr.endsWith('?'));

    {
        OString rest = "not_changed"_ostr;
        CPPUNIT_ASSERT(!"foo"_ostr.endsWith('p', &rest));
        CPPUNIT_ASSERT_EQUAL("not_changed"_ostr, rest);
    }

    {
        OString rest = "must_change"_ostr;
        CPPUNIT_ASSERT("removed_slash/"_ostr.endsWith('/', &rest));
        CPPUNIT_ASSERT_EQUAL("removed_slash"_ostr, rest);
    }

    {
        std::string_view rest = "not_changed";
        CPPUNIT_ASSERT(!"foo"_ostr.endsWith('p', &rest));
        CPPUNIT_ASSERT_EQUAL("not_changed"_ostr, OString(rest));
    }

    {
        std::string_view rest = "must_change";
        CPPUNIT_ASSERT("removed_slash/"_ostr.endsWith('/', &rest));
        CPPUNIT_ASSERT_EQUAL("removed_slash"_ostr, OString(rest));
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
