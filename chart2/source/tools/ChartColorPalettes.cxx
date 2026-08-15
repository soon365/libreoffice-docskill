/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ChartColorPalettes.hxx>
#include <ChartColorPaletteHelper.hxx>
#include <vcl/event.hxx>
#include <vcl/virdev.hxx>

namespace chart
{
constexpr tools::Long BORDER = ChartColorPaletteLayout::ItemBorder;
constexpr tools::Long SIZE = ChartColorPaletteLayout::ItemSize;

ChartColorPalettes::ChartColorPalettes(weld::Builder& rBuilder, const OUString& id,
                                       const OUString& winId)
    : mxIconView(rBuilder.weld_icon_view(id))
    , mxWindow(rBuilder.weld_scrolled_window(winId))
{
    mxIconView->connect_mouse_move(LINK(this, ChartColorPalettes, OnMouseMove));
}

void ChartColorPalettes::SetSelectHdl(const Link<const weld::TreeIter&, bool>& rLink)
{
    mxIconView->connect_item_activated(rLink);
}

sal_uInt16 ChartColorPalettes::GetSelectedItemId()
{
    OUString sId = mxIconView->get_selected_id();
    if (sId.isEmpty())
        return 0;
    return sId.toUInt32();
}

void ChartColorPalettes::SelectItem(sal_uInt16 nItemId) { mxIconView->select(nItemId - 1); }

void ChartColorPalettes::SetNoSelection() { mxIconView->unselect_all(); }

bool ChartColorPalettes::IsNoSelection() { return mxIconView->get_selected_id().isEmpty(); }

void ChartColorPalettes::GrabFocus() { mxIconView->grab_focus(); }

void ChartColorPalettes::insert(ChartColorPalette const& rColorSet)
{
    maColorSets.push_back(rColorSet);
}

const ChartColorPalette* ChartColorPalettes::getPalette(const sal_uInt32 nItem) const
{
    if (maColorSets.size() > nItem)
    {
        return &maColorSets[nItem];
    }
    return nullptr;
}

void ChartColorPalettes::setMouseMoveHdl(const Link<const OUString&, bool>& rLink)
{
    maMouseMoveHdl = rLink;
}

void ChartColorPalettes::Fill()
{
    const Size aSize(BORDER * 6 + SIZE * ChartColorPaletteSize / 2, BORDER * 5 + SIZE * 2);

    mxIconView->clear();

    mxIconView->freeze();

    ScopedVclPtrInstance<VirtualDevice> pVDev;
    pVDev->SetOutputSizePixel(aSize);

    tools::Rectangle aDrawArea(0, 0, aSize.getWidth(), aSize.getHeight());
    for (size_t i = 0; i < maColorSets.size(); ++i)
    {
        const ChartColorPalette& rColorSet = maColorSets[i];
        pVDev->Erase();
        ChartColorPaletteHelper::renderColorPalette(*pVDev, aDrawArea, rColorSet, false);
        OUString sId = OUString::number(i + 1);
        OUString sName = "Palette " + OUString::number(i + 1);

        Bitmap aBitmap(pVDev->GetBitmap(Point(0, 0), pVDev->GetOutputSizePixel()));
        mxIconView->insert(-1, nullptr, &sId, &aBitmap, nullptr);
        mxIconView->set_item_accessible_name(i, sName);
    }

    mxIconView->thaw();
}

IMPL_LINK(ChartColorPalettes, OnMouseMove, const MouseEvent&, rMouseEvent, bool)
{
    OUString sHighlightedId;
    if (!rMouseEvent.IsLeaveWindow())
    {
        if (std::unique_ptr<weld::TreeIter> pItem
            = mxIconView->get_item_at_pos(rMouseEvent.GetPosPixel()))
        {
            sHighlightedId = mxIconView->get_id(*pItem);
        }
    }

    if (maMouseMoveHdl.IsSet())
        return maMouseMoveHdl.Call(sHighlightedId);
    return false;
}

} // end namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
