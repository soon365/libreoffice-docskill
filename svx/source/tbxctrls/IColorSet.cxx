/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <osl/diagnose.hxx>
#include <svx/IColorSet.hxx>
#include <svx/xtable.hxx>

void IColorSet::addEntriesForXColorList(const XColorList& rXColorList)
{
    const sal_uInt32 nColorCount(rXColorList.Count());

    for (sal_uInt32 nIndex = 0; nIndex < nColorCount; nIndex++)
    {
        const XColorEntry* pEntry = rXColorList.GetColor(nIndex);

        if (pEntry)
        {
            insert(nIndex, pEntry->GetColor(), pEntry->GetName());
        }
        else
        {
            OSL_ENSURE(false, "OOps, XColorList with empty entries (!)");
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
