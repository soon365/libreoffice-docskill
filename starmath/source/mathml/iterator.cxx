/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <mathml/iterator.hxx>

/** The purpose of this iterator is to be able to iterate threw an infinite element tree
  * infinite -> as much as your memory can hold
  * No call-backs that will end up in out of stack
  */

namespace mathml
{
void SmMlIteratorFree(SmMlElement* pMlElementTree)
{
    if (pMlElementTree == nullptr)
        return;
    for (size_t i = 0; i < pMlElementTree->getSubElementsCount(); ++i)
    {
        SmMlIteratorFree(pMlElementTree->getSubElement(i));
    }
    delete pMlElementTree;
}

} // end namespace mathml

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
