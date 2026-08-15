/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <rtl/character.hxx>
#include <vcl/font/Variation.hxx>
#include <vcl/font/Feature.hxx>
#include <rtl/ustrbuf.hxx>
#include <o3tl/string_view.hxx>

namespace vcl::font
{
std::vector<Variation> VariationsFromString(std::u16string_view rString)
{
    std::vector<Variation> aVariations;

    // Tokenize on commas: "wght" 700, "wdth" 75
    std::size_t nTokenPos = 0;
    do
    {
        std::u16string_view aToken = o3tl::trim(o3tl::getToken(rString, u',', nTokenPos));
        if (aToken.empty())
            continue;

        // Extract quoted 4-char tag: "wght" or 'wght'
        if (aToken.size() < 7) // minimum: "xxxx" + space + digit
            continue;
        sal_Unicode cQuote = aToken[0];
        if (cQuote != '"' && cQuote != '\'')
            continue;
        if (aToken.size() < 6 || aToken[5] != cQuote)
            continue;

        // Axis tags must use only ASCII letters, digits, or space per the OpenType spec
        bool bValidTag = true;
        for (int j = 1; j <= 4; ++j)
        {
            if (!rtl::isAsciiAlphanumeric(aToken[j]) && aToken[j] != ' ')
            {
                bValidTag = false;
                break;
            }
        }
        if (!bValidTag)
            continue;

        const char aTag[] = { static_cast<char>(aToken[1]), static_cast<char>(aToken[2]),
                              static_cast<char>(aToken[3]), static_cast<char>(aToken[4]) };
        uint32_t nTag = featureCode(aTag);

        // Parse the value after the closing quote
        std::u16string_view aValue = o3tl::trim(aToken.substr(6));
        if (!aValue.empty())
        {
            float fValue = static_cast<float>(o3tl::toDouble(aValue));
            aVariations.push_back({ nTag, fValue });
        }
    } while (nTokenPos != std::u16string_view::npos);

    return aVariations;
}

OUString VariationsToString(const std::vector<Variation>& rVariations)
{
    OUStringBuffer aBuf;
    for (size_t i = 0; i < rVariations.size(); ++i)
    {
        if (i > 0)
            aBuf.append(", ");

        const Variation& rVar = rVariations[i];
        aBuf.append("\"" + featureCodeAsString(rVar.nTag) + "\" ");
        aBuf.append(rVar.fValue);
    }
    return aBuf.makeStringAndClear();
}

} // namespace vcl::font

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
