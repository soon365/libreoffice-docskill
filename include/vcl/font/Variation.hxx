/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <rtl/ustring.hxx>
#include <vcl/dllapi.h>
#include <vector>

namespace vcl::font
{
struct VCL_DLLPUBLIC Variation
{
    uint32_t nTag = 0;
    float fValue = 0;

    bool operator==(const Variation& rOther) const
    {
        return nTag == rOther.nTag && fValue == rOther.fValue;
    }
};

struct VCL_DLLPUBLIC VariationAxis
{
    uint32_t nTag = 0;
    OUString aName;
    float fMinValue = 0;
    float fDefaultValue = 0;
    float fMaxValue = 0;
};

VCL_DLLPUBLIC std::vector<Variation> VariationsFromString(std::u16string_view rString);
VCL_DLLPUBLIC OUString VariationsToString(const std::vector<Variation>& rVariations);

} // namespace vcl::font

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
