/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vcl/weld/ScrolledWindow.hxx>

namespace weld
{
void ScrolledWindow::hadjustment_configure(int value, int upper, int step_increment,
                                           int page_increment, int page_size)
{
    hadjustment_set_value(value);
    hadjustment_set_upper(upper);
    hadjustment_set_step_increment(step_increment);
    hadjustment_set_page_increment(page_increment);
    hadjustment_set_page_size(page_size);
}

void ScrolledWindow::vadjustment_configure(int value, int upper, int step_increment,
                                           int page_increment, int page_size)
{
    vadjustment_set_value(value);
    vadjustment_set_upper(upper);
    vadjustment_set_step_increment(step_increment);
    vadjustment_set_page_increment(page_increment);
    vadjustment_set_page_size(page_size);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
