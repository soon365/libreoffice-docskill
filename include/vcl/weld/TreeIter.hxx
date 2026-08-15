/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <vcl/dllapi.h>

namespace weld
{
class ItemView;

class VCL_DLLPUBLIC TreeIter
{
private:
    const ItemView& m_rItemView;

    TreeIter(const TreeIter&) = delete;
    TreeIter& operator=(const TreeIter&) = delete;

protected:
    explicit TreeIter(const ItemView& rItemView)
        : m_rItemView(rItemView)
    {
    }

public:
    virtual ~TreeIter() {}

    virtual bool equal(const TreeIter& rOther) const = 0;
    const weld::ItemView& getItemView() const { return m_rItemView; }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
