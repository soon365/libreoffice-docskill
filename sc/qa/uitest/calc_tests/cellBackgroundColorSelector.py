# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict
from uitest.uihelper.common import select_pos
from uitest.uihelper.common import select_by_text
from libreoffice.uno.propertyvalue import mkPropertyValues

class CalcCellBackgroundColorSelector(UITestCase):

    def test_color_selector(self):

        #This is to test color selection
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #select cell A1
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A1"}))
            #format - cell
            with self.ui_test.execute_dialog_through_command(".uno:FormatCellDialog") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "6")  #tab Numbers
                # click on color btn
                xbtncolor = xDialog.getChild("btncolor")
                xbtncolor.executeAction("CLICK",tuple())
                xpaletteselector = xDialog.getChild("paletteselector")

                # Now we have the ColorPage that we can get the color selector from it
                xColorpage = xDialog.getChild("ColorPage")
                color_selector = xColorpage.getChild("coloriconview")

                # For chart-palettes colors
                select_by_text(xpaletteselector, "Chart Palettes")
                # Select Color at index 1
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "1"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "1")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "12")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Chart 2")
                # ID is the RGB color value in hex
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemId"], "FF420E")

                # Select Color at index 4
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "4"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "4")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Chart 5")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemId"], "7E0021")

                # For libreoffice colors
                select_by_text(xpaletteselector, "LibreOffice")
                # Select Color at index 5
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "5"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "5")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "32")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Green Accent")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemId"], "2CEE0E")

                # Select Color at index 29
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "29"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "29")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Yellow Accent")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemId"], "FFD74C")

                # For html colors
                select_by_text(xpaletteselector, "HTML")
                # Select Color at index 0
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "0"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "0")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "139")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "White")
                # Select Color at index 119
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "119"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "119")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Navy")

                # For freecolour-hlc colors
                select_by_text(xpaletteselector, "Freecolour HLC")
                # Select Color at index 987
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "987"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "987")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "1032")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "HLC 350 60 10")
                # Select Color at index 574
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "574"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "574")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "HLC 190 50 20")

                # For tonal colors
                select_by_text(xpaletteselector, "Tonal")
                # Select Color at index 16
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "16"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "16")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "120")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Cyan 82%")
                # Select Color at index 12
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "12"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "12")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Magenta 82%")

                # For material colors
                select_by_text(xpaletteselector, "Material")
                # Select Color at index 8
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "8"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "8")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "228")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Gray 800")

                # For standard colors
                select_by_text(xpaletteselector, "Standard")
                # Select Color at index 2
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "2"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "2")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "120")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Dark Gray 3")



    def test_recent_color_selector(self):

        #This is to test recent color selection
        with self.ui_test.create_doc_in_start_center("calc"):
            xCalcDoc = self.xUITest.getTopFocusWindow()
            gridwin = xCalcDoc.getChild("grid_window")
            #select cell A5
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "A5"}))
            #format - cell
            with self.ui_test.execute_dialog_through_command(".uno:FormatCellDialog") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "6")  #tab Numbers
                # click on color btn
                xbtncolor = xDialog.getChild("btncolor")
                xbtncolor.executeAction("CLICK",tuple())

                # we will select color for cell A5 to be able to predict the latest color in
                # recent color selector
                xpaletteselector = xDialog.getChild("paletteselector")
                xColorpage = xDialog.getChild("ColorPage")
                color_selector = xColorpage.getChild("coloriconview")

                # For chart-palettes colors
                select_by_text(xpaletteselector, "Chart Palettes")
                # Select Color at index 1 and press Enter to activate
                color_selector.executeAction("SELECT", mkPropertyValues({"POS": "1"}))
                color_selector.executeAction("TYPE", mkPropertyValues({"KEYCODE": "RETURN"}))
                self.assertEqual(get_state_as_dict(color_selector)["SelectedItemPos"], "1")
                self.assertEqual(get_state_as_dict(color_selector)["Children"], "12")
                self.assertEqual(get_state_as_dict(color_selector)["SelectedEntryTooltip"], "Chart 2")
                xrgb = get_state_as_dict(color_selector)["SelectedItemId"]

                # close the dialog after selection of the color

            #select cell D3
            gridwin.executeAction("SELECT", mkPropertyValues({"CELL": "D3"}))
            #format - cell
            with self.ui_test.execute_dialog_through_command(".uno:FormatCellDialog") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "6")  #tab Numbers
                # click on color btn
                xbtncolor = xDialog.getChild("btncolor")
                xbtncolor.executeAction("CLICK",tuple())

                xColorpage = xDialog.getChild("ColorPage")
                recent_color_selector = xColorpage.getChild("recentcoloriconview")

                # Select Color with id 1
                recent_color_selector.executeAction("SELECT", mkPropertyValues({"POS": "0"}))
                self.assertEqual(get_state_as_dict(recent_color_selector)["SelectedItemPos"], "0")
                self.assertEqual(get_state_as_dict(recent_color_selector)["SelectedEntryTooltip"], "Chart 2")
                self.assertEqual(get_state_as_dict(recent_color_selector)["SelectedItemId"], xrgb)



# vim: set shiftwidth=4 softtabstop=4 expandtab:
