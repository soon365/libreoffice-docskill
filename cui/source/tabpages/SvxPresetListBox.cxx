/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <SvxPresetListBox.hxx>

#include <svx/xtable.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/event.hxx>
#include <vcl/image.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld/Builder.hxx>
#include <vcl/weld/IconView.hxx>
#include <vcl/weld/Menu.hxx>

SvxPresetListBox::SvxPresetListBox(weld::IconView& rIconView)
    : m_rIconView(rIconView)
    , m_aIconSize(60, 64)
{
    m_rIconView.connect_command(LINK(this, SvxPresetListBox, CommandHdl));
    m_rIconView.connect_key_press(LINK(this, SvxPresetListBox, KeyPressHdl));
}

IMPL_LINK(SvxPresetListBox, CommandHdl, const CommandEvent&, rEvent, bool)
{
    if (rEvent.GetCommand() != CommandEventId::ContextMenu)
        return false;

    Point aPos;
    std::unique_ptr<weld::TreeIter> pItem;
    if (rEvent.IsMouseEvent())
    {
        aPos = rEvent.GetMousePosPixel();
        pItem = m_rIconView.get_item_at_pos(aPos);
        if (!pItem)
            return false;
    }
    else
    {
        pItem = m_rIconView.get_selected();
        if (!pItem)
            return false;
        aPos = m_rIconView.get_rect(*pItem).Center();
    }

    const int nContextMenuItemIndex = m_rIconView.get_iter_index_in_parent(*pItem);
    std::unique_ptr<weld::Builder> xBuilder(
        Application::CreateBuilder(&m_rIconView, u"svx/ui/presetmenu.ui"_ustr));
    std::unique_ptr<weld::Menu> xMenu(xBuilder->weld_menu(u"menu"_ustr));
    const OUString sIdent = xMenu->popup_at_rect(&m_rIconView, tools::Rectangle(aPos, Size(1, 1)));
    if (sIdent == u"rename")
        maRenameHdl.Call(nContextMenuItemIndex);
    else if (sIdent == u"delete")
        maDeleteHdl.Call(nContextMenuItemIndex);

    return true;
}

IMPL_LINK(SvxPresetListBox, KeyPressHdl, const KeyEvent&, rKEvt, bool)
{
    switch (rKEvt.GetKeyCode().GetCode())
    {
        case KEY_DELETE:
        {
            maDeleteHdl.Call(m_rIconView.get_selected_index());
            return true;
        }
        default:
            return false;
    }
}

template <typename ListType> void SvxPresetListBox::FillPresetListBoxImpl(ListType& rList)
{
    const Size aSize(GetIconSize());
    for (tools::Long nIndex = 0; nIndex < rList.Count(); nIndex++)
    {
        Bitmap aBitmap = rList.CreateBitmap(nIndex, aSize);
        XPropertyEntry* pItem = rList.Get(nIndex);
        const OUString sName = pItem->GetName();
        m_rIconView.insert(nIndex, nullptr, nullptr, &aBitmap, nullptr);
        m_rIconView.set_item_accessible_name(nIndex, sName);
        m_rIconView.set_item_tooltip_text(nIndex, sName);
    }
}

void SvxPresetListBox::FillPresetListBox(XGradientList& rList)
{
    FillPresetListBoxImpl<XGradientList>(rList);
}

void SvxPresetListBox::FillPresetListBox(XHatchList& rList)
{
    FillPresetListBoxImpl<XHatchList>(rList);
}

void SvxPresetListBox::FillPresetListBox(XBitmapList& rList)
{
    FillPresetListBoxImpl<XBitmapList>(rList);
}

void SvxPresetListBox::FillPresetListBox(XPatternList& rList)
{
    FillPresetListBoxImpl<XPatternList>(rList);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
