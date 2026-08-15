/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// ---------------------------------------------------------------------------
// DocSkill: two-dimension (用途 × 风格) reference-library browse support.
//
// The reference library ships one .otp per style family, all under a single
// category. To let users browse it along two orthogonal axes without inventing
// a multi-tag template model, we read a tiny UTF-8 TSV sidecar written by the
// extension ($(user)/docskill_template_tags.tsv) and filter the item list by
// the selected tabs. Untagged (built-in) templates are unaffected: they only
// show while both tab rows sit on index 0 (全部).
//
// This header is shared by both the standalone Template Manager dialog
// (sfx2/source/doc/templatedlg.cxx) and the Start Center's embedded template
// view (sfx2/source/dialog/backingwindow.cxx), so the same underline tab bar
// and filtering behaviour appear in both places.
// ---------------------------------------------------------------------------

#include <rtl/string.hxx>
#include <rtl/ustring.hxx>

#include <tools/color.hxx>
#include <tools/gen.hxx>
#include <tools/link.hxx>
#include <tools/long.hxx>
#include <tools/stream.hxx>
#include <tools/urlobj.hxx>

#include <vcl/event.hxx>
#include <vcl/font.hxx>
#include <vcl/outdev.hxx>
#include <vcl/ptrstyle.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/customweld.hxx>
#include <vcl/weld/DrawingArea.hxx>

#include <comphelper/processfactory.hxx>
#include <com/sun/star/util/PathSubstitution.hpp>
#include <com/sun/star/util/XStringSubstitution.hpp>

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

struct SfxTemplateTagData
{
    std::vector<OUString> aPurposes;                         // tab labels (row 1)
    std::vector<OUString> aStyles;                           // tab labels (row 2)
    // display-name -> (style label, [purpose labels]); mirrored by family id.
    std::map<OUString, std::pair<OUString, std::vector<OUString>>> aByName;
    std::map<OUString, std::pair<OUString, std::vector<OUString>>> aById;

    bool hasItems() const { return !aByName.empty(); }
};

// A horizontal, web-style underline tab bar rendered natively via VCL, so it
// looks the same on the Windows VCL backend as it does under gtk.
class TemplateTabBar final : public weld::CustomWidgetController
{
public:
    TemplateTabBar() : mnSel(0), mnHover(-1), mfScale(1.0f) {}

    void SetSelectHdl(const Link<TemplateTabBar&, void>& rLink) { maSelectHdl = rLink; }

    // Override the strip background so the tab row can blend seamlessly into the
    // surface below it (e.g. the Start Center template grid). COL_AUTO keeps the
    // default dialog colour used by the standalone Template Manager dialog.
    void SetBgColor(const Color& rCol)
    {
        maBg = rCol;
        // Only invalidate when already mapped and visible; Invalidate during
        // early weld/setup (before Hide/Show) has crashed on Windows VCL.
        if (GetDrawingArea() && IsReallyVisible())
            Invalidate();
    }

    void SetTabs(const std::vector<OUString>& rTabs)
    {
        maTabs = rTabs;
        if (mnSel >= static_cast<sal_Int32>(maTabs.size()))
            mnSel = 0;
        Layout();
        if (GetDrawingArea() && IsReallyVisible())
            Invalidate();
    }

    sal_Int32 GetSelected() const { return mnSel; }
    OUString GetSelectedText() const
    {
        return (mnSel >= 0 && mnSel < static_cast<sal_Int32>(maTabs.size()))
                   ? maTabs[mnSel] : OUString();
    }

    virtual void SetDrawingArea(weld::DrawingArea* pDrawingArea) override
    {
        CustomWidgetController::SetDrawingArea(pDrawingArea);
        OutputDevice& rDev = pDrawingArea->get_ref_device();
        mfScale = rDev.GetDPIScaleFactor();
        const tools::Long nRow = rDev.GetTextHeight() + static_cast<tools::Long>(14 * mfScale);
        pDrawingArea->set_size_request(-1, nRow);
        SetOutputSizePixel(Size(GetOutputSizePixel().Width(), nRow));
        Layout();
    }

    virtual void Resize() override
    {
        Layout();
        Invalidate();
    }

    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&) override
    {
        const tools::Long nW = GetOutputSizePixel().Width();
        const tools::Long nH = GetOutputSizePixel().Height();
        if (nW <= 0 || nH <= 0)
            return;

        const StyleSettings& rStyle = Application::GetSettings().GetStyleSettings();
        const Color aBg = (maBg == COL_AUTO) ? rStyle.GetDialogColor() : maBg;
        const Color aInactive = rStyle.GetDisableColor();
        const Color aActive = rStyle.GetWindowTextColor();
        const Color aHover = rStyle.GetLabelTextColor();
        const Color aAccent = rStyle.GetHighlightColor();

        rRenderContext.SetBackground(aBg);
        rRenderContext.Erase();

        const tools::Long nUnderline = std::max<tools::Long>(2, static_cast<tools::Long>(2 * mfScale));
        const tools::Long nTextH = rRenderContext.GetTextHeight();
        const tools::Long nTextY = std::max<tools::Long>(0, (nH - nTextH - nUnderline) / 2);

        const vcl::Font aBaseFont = rRenderContext.GetFont();
        for (size_t i = 0; i < maTabs.size() && i < maTabRects.size(); ++i)
        {
            const tools::Rectangle& rRect = maTabRects[i];
            const bool bSel = (static_cast<sal_Int32>(i) == mnSel);
            const bool bHov = (static_cast<sal_Int32>(i) == mnHover);

            vcl::Font aFont = aBaseFont;
            aFont.SetWeight(bSel ? WEIGHT_BOLD : WEIGHT_NORMAL);
            rRenderContext.SetFont(aFont);
            rRenderContext.SetTextColor(bSel ? aActive : (bHov ? aHover : aInactive));

            const tools::Long nTextW = rRenderContext.GetTextWidth(maTabs[i]);
            const tools::Long nTx = rRect.Left() + (rRect.GetWidth() - nTextW) / 2;
            rRenderContext.DrawText(Point(nTx, nTextY), maTabs[i]);

            if (bSel)
            {
                rRenderContext.SetLineColor();
                rRenderContext.SetFillColor(aAccent);
                rRenderContext.DrawRect(tools::Rectangle(
                    rRect.Left(), nH - nUnderline - 1, rRect.Right(), nH - 1));
            }
        }
        rRenderContext.SetFont(aBaseFont);
    }

    virtual bool MouseButtonDown(const MouseEvent& rMEvt) override
    {
        const sal_Int32 nHit = HitTest(rMEvt.GetPosPixel());
        if (nHit >= 0 && nHit != mnSel)
        {
            mnSel = nHit;
            Invalidate();
            maSelectHdl.Call(*this);
        }
        return true;
    }

    virtual bool MouseMove(const MouseEvent& rMEvt) override
    {
        const sal_Int32 nHit = HitTest(rMEvt.GetPosPixel());
        if (nHit != mnHover)
        {
            mnHover = nHit;
            SetPointer(nHit >= 0 ? PointerStyle::RefHand : PointerStyle::Arrow);
            Invalidate();
        }
        return true;
    }

private:
    std::vector<OUString> maTabs;
    std::vector<tools::Rectangle> maTabRects;
    sal_Int32 mnSel;
    sal_Int32 mnHover;
    float mfScale;
    Color maBg = COL_AUTO;
    Link<TemplateTabBar&, void> maSelectHdl;

    void Layout()
    {
        maTabRects.clear();
        weld::DrawingArea* pArea = GetDrawingArea();
        if (!pArea)
            return;
        OutputDevice& rDev = pArea->get_ref_device();
        const tools::Long nH = GetOutputSizePixel().Height();
        const tools::Long nPadH = static_cast<tools::Long>(14 * mfScale);
        const tools::Long nGap = static_cast<tools::Long>(4 * mfScale);
        tools::Long nX = static_cast<tools::Long>(2 * mfScale);
        for (const OUString& rTab : maTabs)
        {
            const tools::Long nW = rDev.GetTextWidth(rTab) + 2 * nPadH;
            maTabRects.emplace_back(nX, tools::Long(0), nX + nW, nH);
            nX += nW + nGap;
        }
    }

    sal_Int32 HitTest(const Point& rPos) const
    {
        for (size_t i = 0; i < maTabRects.size(); ++i)
            if (maTabRects[i].Contains(rPos))
                return static_cast<sal_Int32>(i);
        return -1;
    }
};

// Read $(user)/docskill_template_tags.tsv into rData. Any prior content in
// rData is left as-is on failure (callers pass a fresh instance).
inline void LoadDocSkillTemplateTags(SfxTemplateTagData& rData)
{
    OUString sUrl;
    try
    {
        css::uno::Reference<css::util::XStringSubstitution> xSubst =
            css::util::PathSubstitution::create(comphelper::getProcessComponentContext());
        sUrl = xSubst->substituteVariables(
            u"$(user)/docskill_template_tags.tsv"_ustr, true);
    }
    catch (const css::uno::Exception&)
    {
        return;
    }
    if (sUrl.isEmpty())
        return;

    SvFileStream aStream(sUrl, StreamMode::READ);
    if (!aStream.IsOpen())
        return;

    OString aLine;
    while (aStream.ReadLine(aLine))
    {
        if (aLine.isEmpty())
            continue;
        const OUString s = OStringToOUString(aLine, RTL_TEXTENCODING_UTF8);
        std::vector<OUString> aCols;
        sal_Int32 nIdx = 0;
        do
        {
            aCols.push_back(s.getToken(0, '\t', nIdx));
        } while (nIdx >= 0);
        if (aCols.empty())
            continue;

        if (aCols[0] == "purposes")
            rData.aPurposes.assign(aCols.begin() + 1, aCols.end());
        else if (aCols[0] == "styles")
            rData.aStyles.assign(aCols.begin() + 1, aCols.end());
        else if (aCols[0] == "item" && aCols.size() >= 4)
        {
            std::vector<OUString> aP;
            if (aCols.size() >= 5 && !aCols[4].isEmpty())
            {
                sal_Int32 nPi = 0;
                do
                {
                    const OUString sP = aCols[4].getToken(0, ',', nPi);
                    if (!sP.isEmpty())
                        aP.push_back(sP);
                } while (nPi >= 0);
            }
            auto aVal = std::make_pair(aCols[3], aP);
            if (!aCols[1].isEmpty())
                rData.aByName[aCols[1]] = aVal;
            if (!aCols[2].isEmpty())
                rData.aById[aCols[2]] = aVal;
        }
    }
}

// Whether an item passes the active (purpose, style) tab constraints. Empty
// selection strings mean "全部" (no constraint on that axis). rName is the
// template display name, rPath its file URL (used to recover the family id).
inline bool DocSkillTagMatch(const SfxTemplateTagData& rData,
                             const OUString& rSelPurpose, const OUString& rSelStyle,
                             const OUString& rName, const OUString& rPath)
{
    if (rSelPurpose.isEmpty() && rSelStyle.isEmpty())
        return true;

    auto it = rData.aByName.find(rName);
    if (it == rData.aByName.end())
    {
        const OUString sBase = INetURLObject(rPath).getBase();
        it = rData.aById.find(sBase);
        if (it == rData.aById.end())
            return false;
    }
    if (!rSelStyle.isEmpty() && it->second.first != rSelStyle)
        return false;
    if (!rSelPurpose.isEmpty())
    {
        const std::vector<OUString>& rP = it->second.second;
        if (std::find(rP.begin(), rP.end(), rSelPurpose) == rP.end())
            return false;
    }
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
