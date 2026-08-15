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

#include <salvtables.hxx>

/**
 * SalInstanceBuilder subclass/wrapper to support native Qt dialogs/widgets
 * created by QtInstanceBuilder as parents for non-native dialogs created
 * by SalInstanceBuilder.
 *
 * This is e.g. required to support non-native modal dialogs with native parents.
 *
 * Once QtInstanceBuilder can handle all UI files itself, this class is no
 * longer needed.
 */
class QtSalInstanceBuilderWrapper : public SalInstanceBuilder
{
    weld::Widget* const m_pParent;

public:
    QtSalInstanceBuilderWrapper(weld::Widget* pParent, std::u16string_view sUIRoot,
                                const OUString& rUIFile);
    virtual ~QtSalInstanceBuilderWrapper() override;

    virtual std::unique_ptr<weld::Dialog> weld_dialog(const OUString& id) override;

private:
    static vcl::Window* GetVclWidget(weld::Widget* pWidget);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
