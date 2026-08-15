/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>
#include <config_fonts.h>
#include <cppunit/TestAssert.h>

#include <vcl/font/Feature.hxx>
#include <vcl/font/Variation.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>

class FontVariationTest : public test::BootstrapFixture
{
public:
    FontVariationTest()
        : BootstrapFixture(true, false)
    {
    }

    void testGetFontVariationAxesVariableFont();
    void testGetFontVariationAxesNonVariableFont();

    CPPUNIT_TEST_SUITE(FontVariationTest);
    CPPUNIT_TEST(testGetFontVariationAxesVariableFont);
    CPPUNIT_TEST(testGetFontVariationAxesNonVariableFont);
    CPPUNIT_TEST_SUITE_END();
};

void FontVariationTest::testGetFontVariationAxesVariableFont()
{
#if HAVE_MORE_FONTS
    ScopedVclPtrInstance<VirtualDevice> aVDev(*Application::GetDefaultDevice(),
                                              DeviceFormat::WITH_ALPHA);
    aVDev->SetOutputSizePixel(Size(10, 10));

    // Reem Kufi is bundled and exposes a single wght axis.
    OUString aFontName(u"Reem Kufi"_ustr);
    CPPUNIT_ASSERT(aVDev->IsFontAvailable(aFontName));

    vcl::Font aFont = aVDev->GetFont();
    aFont.SetFamilyName(aFontName);
    aVDev->SetFont(aFont);

    std::vector<vcl::font::VariationAxis> aAxes;
    CPPUNIT_ASSERT(aVDev->GetFontVariationAxes(aAxes));

    CPPUNIT_ASSERT_EQUAL(size_t(1), aAxes.size());

    // wght: Weight, 400..700, default 400
    CPPUNIT_ASSERT_EQUAL(vcl::font::featureCode("wght"), aAxes[0].nTag);
    CPPUNIT_ASSERT_EQUAL(u"Weight"_ustr, aAxes[0].aName);
    CPPUNIT_ASSERT_EQUAL(400.0f, aAxes[0].fMinValue);
    CPPUNIT_ASSERT_EQUAL(400.0f, aAxes[0].fDefaultValue);
    CPPUNIT_ASSERT_EQUAL(700.0f, aAxes[0].fMaxValue);

    aVDev.disposeAndClear();
#endif // HAVE_MORE_FONTS
}

void FontVariationTest::testGetFontVariationAxesNonVariableFont()
{
#if HAVE_MORE_FONTS
    ScopedVclPtrInstance<VirtualDevice> aVDev(*Application::GetDefaultDevice(),
                                              DeviceFormat::WITH_ALPHA);
    aVDev->SetOutputSizePixel(Size(10, 10));

    // Amiri is bundled but has no variation axes.
    OUString aFontName(u"Amiri"_ustr);
    CPPUNIT_ASSERT(aVDev->IsFontAvailable(aFontName));

    vcl::Font aFont = aVDev->GetFont();
    aFont.SetFamilyName(aFontName);
    aVDev->SetFont(aFont);

    std::vector<vcl::font::VariationAxis> aAxes;
    CPPUNIT_ASSERT(!aVDev->GetFontVariationAxes(aAxes));
    CPPUNIT_ASSERT(aAxes.empty());

    aVDev.disposeAndClear();
#endif // HAVE_MORE_FONTS
}

CPPUNIT_TEST_SUITE_REGISTRATION(FontVariationTest);

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
