# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from libreoffice.uno.propertyvalue import mkPropertyValues
from uitest.uihelper.calc import enter_text_to_cell
from libreoffice.calc.document import get_cell_by_position

class tdf171481(UITestCase):

    def testUnformatted(self):
        with self.ui_test.load_empty_file("calc") as document:
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")

            enter_text_to_cell(gridwin, "A1", "Open_new_Calc.ods")
            enter_text_to_cell(gridwin, "A2", "Paste_the_following_two_lines.ods")
            gridwin.executeAction("SELECT", mkPropertyValues({"RANGE": "A1:A2"}))
            self.xUITest.executeCommand(".uno:Copy")
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "C1"}))
            with self.ui_test.execute_dialog_through_command(".uno:PasteUnformatted", close_button="ok") as xDialog:
                pass

            document = self.ui_test.get_component()
            self.assertEqual("Open_new_Calc.ods", get_cell_by_position(document, 0, 2, 0).getString())
            self.assertEqual("Paste_the_following_two_lines.ods", get_cell_by_position(document, 0, 2, 1).getString())

    def testMarkdown(self):
        with self.ui_test.load_empty_file("writer") as writer_doc:
            xText = writer_doc.getText()
            xCursor = xText.createTextCursor()
            xText.insertString(xCursor, "| 1 | 2 |\n| --- | --- |\n| 3 | 4 |", False)

            self.xUITest.executeCommand(".uno:SelectAll")
            self.xUITest.executeCommand(".uno:Copy")

            with self.ui_test.load_empty_file("calc"):
                frames = self.ui_test.get_frames()
                frames[1].activate()

                formatProperty = mkPropertyValues({"SelectedFormat": 149})
                self.xUITest.executeCommandWithParameters(".uno:ClipboardFormatItems", formatProperty)

                document = self.ui_test.get_component()
                self.assertEqual("1", get_cell_by_position(document, 0, 0, 0).getString())
                self.assertEqual("2", get_cell_by_position(document, 0, 1, 0).getString())
                self.assertEqual("3", get_cell_by_position(document, 0, 0, 1).getString())
                self.assertEqual("4", get_cell_by_position(document, 0, 1, 1).getString())

# vim: set shiftwidth=4 softtabstop=4 expandtab:
