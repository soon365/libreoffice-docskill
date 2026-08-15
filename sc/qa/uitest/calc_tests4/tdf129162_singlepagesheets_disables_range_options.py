# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#from uitest.framework import UITestCase
#from uitest.uihelper.common import get_state_as_dict
#from libreoffice.uno.propertyvalue import mkPropertyValues

import os
from tempfile import TemporaryDirectory

from uitest.framework import UITestCase
from uitest.uihelper.calc import enter_text_to_cell
from uitest.uihelper.common import get_state_as_dict
from libreoffice.uno.propertyvalue import mkPropertyValues
from org.libreoffice.unotest import systemPathToFileUrl

class tdf129162(UITestCase):
    def test_singlePageSheetsDisablesRangeOptions(self):
        with self.ui_test.create_doc_in_start_center("calc"):
            with self.ui_test.execute_dialog_through_command('.uno:ExportToPDF', close_button="cancel") as xDialog:
                rangeChildren = ['all', 'pagerange', 'sheetrange', 'selection']

                self.assertEqual("false", get_state_as_dict(xDialog.getChild('singlepagesheets'))['Selected'])
                for child in rangeChildren:
                    self.assertEqual("true", get_state_as_dict(xDialog.getChild(child))['Enabled'])

                xDialog.getChild('singlepagesheets').executeAction('CLICK', mkPropertyValues({}))
                self.assertEqual("true", get_state_as_dict(xDialog.getChild('singlepagesheets'))['Selected'])
                for child in rangeChildren:
                    self.assertEqual("false", get_state_as_dict(xDialog.getChild(child))['Enabled'])

                xDialog.getChild('singlepagesheets').executeAction('CLICK', mkPropertyValues({}))
                self.assertEqual("false", get_state_as_dict(xDialog.getChild('singlepagesheets'))['Selected'])
                for child in rangeChildren:
                    self.assertEqual("true", get_state_as_dict(xDialog.getChild(child))['Enabled'])

# vim: set shiftwidth=4 softtabstop=4 expandtab:
