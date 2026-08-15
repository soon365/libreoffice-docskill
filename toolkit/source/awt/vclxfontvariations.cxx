/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <com/sun/star/awt/FontVariationAxis.hpp>
#include <com/sun/star/awt/XFontVariations.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <vcl/font/Feature.hxx>
#include <vcl/font/Variation.hxx>
#include <vcl/svapp.hxx>
#include <vcl/virdev.hxx>

namespace
{
class VCLXFontVariations
    : public cppu::WeakImplHelper<css::awt::XFontVariations, css::lang::XServiceInfo>
{
public:
    // XFontVariations
    virtual css::uno::Sequence<css::awt::FontVariationAxis>
        SAL_CALL getFontVariationAxes(const OUString& FontName) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;
};

css::uno::Sequence<css::awt::FontVariationAxis>
    SAL_CALL VCLXFontVariations::getFontVariationAxes(const OUString& FontName)
{
    SolarMutexGuard aGuard;

    ScopedVclPtrInstance<VirtualDevice> pDev;
    vcl::Font aFont = pDev->GetFont();
    aFont.SetFamilyName(FontName);
    pDev->SetFont(aFont);

    std::vector<vcl::font::VariationAxis> aAxes;
    if (!pDev->GetFontVariationAxes(aAxes))
        return {};

    css::uno::Sequence<css::awt::FontVariationAxis> aRet(aAxes.size());
    css::awt::FontVariationAxis* pRet = aRet.getArray();
    for (size_t i = 0; i < aAxes.size(); ++i)
    {
        pRet[i].Tag = vcl::font::featureCodeAsString(aAxes[i].nTag);
        pRet[i].Name = aAxes[i].aName;
        pRet[i].MinValue = aAxes[i].fMinValue;
        pRet[i].DefaultValue = aAxes[i].fDefaultValue;
        pRet[i].MaxValue = aAxes[i].fMaxValue;
    }
    return aRet;
}

OUString SAL_CALL VCLXFontVariations::getImplementationName()
{
    return u"stardiv.Toolkit.VCLXFontVariations"_ustr;
}

sal_Bool SAL_CALL VCLXFontVariations::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence<OUString> SAL_CALL VCLXFontVariations::getSupportedServiceNames()
{
    return { u"com.sun.star.awt.FontVariations"_ustr };
}
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
stardiv_Toolkit_VCLXFontVariations_get_implementation(css::uno::XComponentContext*,
                                                      css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new VCLXFontVariations);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
