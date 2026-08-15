/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sal/config.h>
#include <config_vclplug.h>

#include <QtFont.hxx>
#include <QtFontFace.hxx>
#include <QtFrame.hxx>
#include <QtGraphics.hxx>
#include <QtGraphics_Controls.hxx>
#include <QtInstance.hxx>
#include <QtPainter.hxx>
#include <font/PhysicalFontCollection.hxx>
#if USE_HEADLESS_CODE
#include <unx/fontmanager.hxx>
#include <unx/geninst.h>
#include <unx/glyphcache.hxx>
#endif
#include <sallayout.hxx>

#include <vcl/fontcharmap.hxx>

#include <QtGui/QFontDatabase>
#include <QtGui/QGlyphRun>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QRawFont>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QtGraphics::QtGraphics( QtFrame *pFrame, QImage *pQImage )
    : m_pFrame( pFrame )
    , m_pTextStyle{ nullptr, }
    , m_aTextColor( 0x00, 0x00, 0x00 )
{
    m_pBackend = std::make_unique<QtGraphicsBackend>(m_pFrame, pQImage);

    if (!initWidgetDrawBackends(false))
    {
        if (!QtInstance::noNativeControls())
            m_pWidgetDraw.reset(new QtGraphics_Controls(*this));
    }
    if (m_pFrame)
        setDevicePixelRatioF(m_pFrame->devicePixelRatioF());
}

QtGraphics::~QtGraphics() { ReleaseFonts(); }

void QtGraphics::ChangeQImage(QImage* pQImage)
{
    m_pBackend->setQImage(pQImage);
    m_pBackend->ResetClipRegion();
}

SalGraphicsImpl* QtGraphics::GetImpl() const { return m_pBackend.get(); }

SystemGraphicsData QtGraphics::GetGraphicsData() const { return SystemGraphicsData(); }

void QtGraphics::handleDamage(const tools::Rectangle& rDamagedRegion)
{
    assert(m_pWidgetDraw);
    assert(dynamic_cast<QtGraphics_Controls*>(m_pWidgetDraw.get()));
    assert(!rDamagedRegion.IsEmpty());

    QImage* pImage = static_cast<QtGraphics_Controls*>(m_pWidgetDraw.get())->getImage();
    QImage blit(*pImage);
    blit.setDevicePixelRatio(1);
    QtPainter aPainter(*m_pBackend);
    aPainter.drawImage(QPoint(rDamagedRegion.Left(), rDamagedRegion.Top()), blit);
    aPainter.update(toQRect(rDamagedRegion));
}

void QtGraphics::GetResolution(sal_Int32& rDPIX, sal_Int32& rDPIY)
{
    QtGraphicsBase::ImplGetResolution(m_pFrame, rDPIX, rDPIY);
}

void QtGraphics::SetTextColor(Color nColor) { m_aTextColor = nColor; }

void QtGraphics::SetFont(LogicalFontInstance* pReqFont, int nFallbackLevel)
{
    // release the text styles
    for (int i = nFallbackLevel; i < MAX_FALLBACK; ++i)
    {
        if (!m_pTextStyle[i])
            break;
        m_pTextStyle[i].clear();
    }

    if (!pReqFont)
        return;

    m_pTextStyle[nFallbackLevel] = static_cast<QtFont*>(pReqFont);
}

void QtGraphics::GetFontMetric(FontMetricDataRef& rFMD, int nFallbackLevel)
{
    QRawFont aRawFont(QRawFont::fromFont(*m_pTextStyle[nFallbackLevel]));
    QtFontFace::fillAttributesFromQFont(*m_pTextStyle[nFallbackLevel], *rFMD);

    rFMD->ImplCalcLineSpacing(m_pTextStyle[nFallbackLevel].get());
    rFMD->ImplInitBaselines(m_pTextStyle[nFallbackLevel].get());

    rFMD->SetSlant(0);
    rFMD->SetWidth(aRawFont.averageCharWidth());

    rFMD->SetMinKashida(m_pTextStyle[nFallbackLevel]->GetKashidaWidth());
}

FontCharMapRef QtGraphics::GetFontCharMap() const
{
    if (!m_pTextStyle[0])
        return FontCharMapRef(new FontCharMap());
    return m_pTextStyle[0]->GetFontFace()->GetFontCharMap();
}

bool QtGraphics::GetFontCapabilities(vcl::FontCapabilities& rFontCapabilities) const
{
    if (!m_pTextStyle[0])
        return false;
    return m_pTextStyle[0]->GetFontFace()->GetFontCapabilities(rFontCapabilities);
}

void QtGraphics::GetDevFontList(vcl::font::PhysicalFontCollection* pPFC)
{
    if (pPFC->Count())
        return;

#if USE_HEADLESS_CODE
    FreetypeManager& rFontManager = FreetypeManager::get();
    psp::PrintFontManager& rMgr = psp::PrintFontManager::get();
    std::vector<psp::fontID> aList = rMgr.getFontList();
    for (auto const& nFontId : aList)
    {
        auto const* pFont = rMgr.getFont(nFontId);
        if (!pFont)
            continue;

        // normalize face number to the FreetypeManager
        int nFaceNum = rMgr.getFontFaceNumber(nFontId);
        int nVariantNum = rMgr.getFontFaceVariation(nFontId);

        // inform FreetypeManager about this font provided by the PsPrint subsystem
        FontAttributes aFA = pFont->m_aFontAttributes;
        aFA.IncreaseQualityBy(4096);
        const OString aFileName = rMgr.getFontFileSysPath(nFontId);
        rFontManager.AddFontFile(aFileName, nFaceNum, nVariantNum, nFontId, aFA);
    }

    static const bool bUseFontconfig = (nullptr == getenv("SAL_VCL_QT_NO_FONTCONFIG"));
    if (bUseFontconfig)
        SalGenericInstance::RegisterFontSubstitutors(pPFC);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (auto& family : QFontDatabase::families())
        for (auto& style : QFontDatabase::styles(family))
            pPFC->Add(QtFontFace::fromQFontDatabase(family, style));
#else
    QFontDatabase aFDB;
    for (auto& family : aFDB.families())
        for (auto& style : aFDB.styles(family))
            pPFC->Add(QtFontFace::fromQFontDatabase(family, style));
#endif
}

void QtGraphics::ClearDevFontCache() {}

bool QtGraphics::AddTempDevFont(vcl::font::PhysicalFontCollection*, const OUString& /*rFileURL*/,
                                const OUString& /*rFontName*/)
{
    return false;
}

bool QtGraphics::RemoveTempDevFont(const OUString& /*rFileURL*/, const OUString& /*rFontName*/)
{
    return true; // No fonts registered - no problem to remove
}

namespace
{
class QtCommonSalLayout : public GenericSalLayout
{
public:
    QtCommonSalLayout(LogicalFontInstance& rLFI)
        : GenericSalLayout(rLFI)
    {
    }

    void SetOrientation(Degree10 nOrientation) { mnOrientation = nOrientation; }
};
}

std::unique_ptr<GenericSalLayout> QtGraphics::GetTextLayout(int nFallbackLevel)
{
    assert(m_pTextStyle[nFallbackLevel]);
    if (!m_pTextStyle[nFallbackLevel])
        return nullptr;
    return std::make_unique<QtCommonSalLayout>(*m_pTextStyle[nFallbackLevel]);
}

static QRawFont GetRawFont(const QFont& rFont, bool bWithoutHintingInTextDirection)
{
    QFont::HintingPreference eHinting = rFont.hintingPreference();
    static bool bAllowDefaultHinting = getenv("SAL_ALLOW_DEFAULT_HINTING") != nullptr;
    bool bAllowedHintStyle
        = !bWithoutHintingInTextDirection || bAllowDefaultHinting
          || (eHinting == QFont::PreferNoHinting || eHinting == QFont::PreferVerticalHinting);
    if (bWithoutHintingInTextDirection && !bAllowedHintStyle)
    {
        QFont aFont(rFont);
        aFont.setHintingPreference(QFont::PreferVerticalHinting);
        return QRawFont::fromFont(aFont);
    }
    return QRawFont::fromFont(rFont);
}

void QtGraphics::DrawTextLayout(const GenericSalLayout& rLayout)
{
    const QtFont* pFont = static_cast<const QtFont*>(&rLayout.GetFont());
    assert(pFont);
    QRawFont aRawFont(GetRawFont(*pFont, rLayout.GetSubpixelPositioning()));

    QVector<quint32> glyphIndexes;
    QVector<QPointF> positions;

    // prevent glyph rotation inside the SalLayout
    // probably better to add a parameter to GetNextGlyphs?
    QtCommonSalLayout* pQtLayout
        = static_cast<QtCommonSalLayout*>(const_cast<GenericSalLayout*>(&rLayout));
    Degree10 nOrientation = rLayout.GetOrientation();
    if (nOrientation)
        pQtLayout->SetOrientation(0_deg10);

    basegfx::B2DPoint aPos;
    const GlyphItem* pGlyph;
    int nStart = 0;
    while (rLayout.GetNextGlyph(&pGlyph, aPos, nStart))
    {
        glyphIndexes.push_back(pGlyph->glyphId());
        positions.push_back(QPointF(aPos.getX(), aPos.getY()));
    }

    // seems to be common to try to layout an empty string...
    if (positions.empty())
        return;

    if (nOrientation)
        pQtLayout->SetOrientation(nOrientation);

    QGlyphRun aGlyphRun;
    aGlyphRun.setPositions(positions);
    aGlyphRun.setGlyphIndexes(glyphIndexes);
    aGlyphRun.setRawFont(aRawFont);

    QtPainter aPainter(*m_pBackend);
    QColor aColor = toQColor(m_aTextColor);
    aPainter.setPen(aColor);

    if (nOrientation)
    {
        // make text position the center of the rotation
        // then rotate and move back
        QRect window = aPainter.window();
        window.moveTo(-positions[0].x(), -positions[0].y());
        aPainter.setWindow(window);

        QTransform p;
        p.rotate(-static_cast<qreal>(nOrientation.get()) / 10.0);
        p.translate(-positions[0].x(), -positions[0].y());
        aPainter.setTransform(p);
    }

    aPainter.drawGlyphRun(QPointF(), aGlyphRun);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
