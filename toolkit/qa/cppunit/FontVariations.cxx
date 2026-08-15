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

#include <com/sun/star/awt/FontVariationAxis.hpp>
#include <com/sun/star/awt/XFontVariations.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/uno/Reference.hxx>

using namespace css;
using namespace css::awt;
using namespace css::lang;
using namespace css::uno;

namespace
{
class FontVariationsTest : public test::BootstrapFixture
{
};

CPPUNIT_TEST_FIXTURE(FontVariationsTest, testGetFontVariationAxesVariableFont)
{
#if HAVE_MORE_FONTS
    Reference<XMultiComponentFactory> xFactory(m_xContext->getServiceManager(), UNO_SET_THROW);
    Reference<XFontVariations> xProvider(
        xFactory->createInstanceWithContext(u"com.sun.star.awt.FontVariations"_ustr, m_xContext),
        UNO_QUERY_THROW);

    // Reem Kufi is bundled and exposes a single wght axis.
    Sequence<FontVariationAxis> aAxes = xProvider->getFontVariationAxes(u"Reem Kufi"_ustr);

    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), aAxes.getLength());
    CPPUNIT_ASSERT_EQUAL(u"wght"_ustr, aAxes[0].Tag);
    CPPUNIT_ASSERT_EQUAL(u"Weight"_ustr, aAxes[0].Name);
    CPPUNIT_ASSERT_EQUAL(400.0, aAxes[0].MinValue);
    CPPUNIT_ASSERT_EQUAL(400.0, aAxes[0].DefaultValue);
    CPPUNIT_ASSERT_EQUAL(700.0, aAxes[0].MaxValue);
#endif
}

CPPUNIT_TEST_FIXTURE(FontVariationsTest, testGetFontVariationAxesNonVariableFont)
{
#if HAVE_MORE_FONTS
    Reference<XMultiComponentFactory> xFactory(m_xContext->getServiceManager(), UNO_SET_THROW);
    Reference<XFontVariations> xProvider(
        xFactory->createInstanceWithContext(u"com.sun.star.awt.FontVariations"_ustr, m_xContext),
        UNO_QUERY_THROW);

    // Amiri is bundled but has no variation axes.
    Sequence<FontVariationAxis> aAxes = xProvider->getFontVariationAxes(u"Amiri"_ustr);

    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), aAxes.getLength());
#endif
}

CPPUNIT_TEST_FIXTURE(FontVariationsTest, testGetFontVariationAxesUnknownFont)
{
    Reference<XMultiComponentFactory> xFactory(m_xContext->getServiceManager(), UNO_SET_THROW);
    Reference<XFontVariations> xProvider(
        xFactory->createInstanceWithContext(u"com.sun.star.awt.FontVariations"_ustr, m_xContext),
        UNO_QUERY_THROW);

    Sequence<FontVariationAxis> aAxes
        = xProvider->getFontVariationAxes(u"This Font Does Not Exist"_ustr);

    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), aAxes.getLength());
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
