/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <QtInstance.hxx>
#include <QtInstanceWidget.hxx>
#include <QtSalInstanceBuilderWrapper.hxx>

#include <QtWidgets/QWidget>

QtSalInstanceBuilderWrapper::QtSalInstanceBuilderWrapper(weld::Widget* pParent,
                                                         std::u16string_view sUIRoot,
                                                         const OUString& rUIFile)
    : SalInstanceBuilder(GetVclWidget(pParent), sUIRoot, rUIFile)
    , m_pParent(pParent)
{
}

QtSalInstanceBuilderWrapper::~QtSalInstanceBuilderWrapper() {}

std::unique_ptr<weld::Dialog> QtSalInstanceBuilderWrapper::weld_dialog(const OUString& id)
{
    std::unique_ptr<weld::Dialog> pDialog = SalInstanceBuilder::weld_dialog(id);

    // if the parent is a QtInstanceWidget (i.e. a native Qt dialog created by QtInstanceBuilder),
    // set its QWidget as a parent here (e.g. for proper modality)
    // because SalInstanceBuilder only handles SalInstanceWidget parents itself
    QtInstanceWidget* pQtParent = dynamic_cast<QtInstanceWidget*>(m_pParent);
    if (!pQtParent)
        return pDialog;

    if (QWidget* pDialogWidget = QtInstance::GetQWidget(pDialog.get()))
    {
        QWidget* pDialogWindow = pDialogWidget->window();
        assert(pDialogWindow);
        pDialogWindow->setParent(pQtParent->getQWidget(), pDialogWindow->windowFlags());
    }

    return pDialog;
}

vcl::Window* QtSalInstanceBuilderWrapper::GetVclWidget(weld::Widget* pWidget)
{
    if (SalInstanceWidget* pSalInstanceParent = dynamic_cast<SalInstanceWidget*>(pWidget))
        return pSalInstanceParent->getWidget();

    return nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
