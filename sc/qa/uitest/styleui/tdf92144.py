# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
"""Tests for tdf#92144 - relative font sizes in Calc cell styles."""

from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict
from uitest.uihelper.common import select_pos
from uitest.uihelper.common import type_text
from libreoffice.uno.propertyvalue import mkPropertyValues


class tdf92144(UITestCase):

    def test_percentage_font_size_in_cell_style(self):
        """Test that percentage font size is preserved in cell style."""
        with self.ui_test.create_doc_in_start_center("calc"):

            # Create a new cell style
            with self.ui_test.execute_dialog_through_command(
                    ".uno:StyleNewByExample") as xDialog:
                xStyleName = xDialog.getChild("stylename")
                xStyleName.executeAction("CLEAR", tuple())
                type_text(xStyleName, "Test Percent Size")

            # Edit the style and set 80%
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test Percent Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Set percentage
                xSizeField = xDialog.getChild("cbWestSize")
                xSizeField.executeAction("CLEAR", tuple())
                type_text(xSizeField, "80%")

            # Reopen the style and verify it's still 80%
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test Percent Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Read the value
                xSizeField = xDialog.getChild("cbWestSize")
                displayed_value = get_state_as_dict(xSizeField)["Text"]

                # Verify it's still 80% (not converted to absolute value)
                self.assertEqual(displayed_value, "80%",
                    "Percentage value was not preserved")

    def test_point_relative_font_size_in_cell_style(self):
        """Test that +2pt font size is preserved in cell style."""
        with self.ui_test.create_doc_in_start_center("calc"):

            # Create a new cell style
            with self.ui_test.execute_dialog_through_command(
                    ".uno:StyleNewByExample") as xDialog:
                xStyleName = xDialog.getChild("stylename")
                xStyleName.executeAction("CLEAR", tuple())
                type_text(xStyleName, "Test PtRelative Size")

            # Edit the style and set +2pt
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test PtRelative Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Set +2pt
                xSizeField = xDialog.getChild("cbWestSize")
                xSizeField.executeAction("CLEAR", tuple())
                type_text(xSizeField, "+2pt")

            # Reopen and verify
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test PtRelative Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Read the value
                xSizeField = xDialog.getChild("cbWestSize")
                displayed_value = get_state_as_dict(xSizeField)["Text"]

                # Verify it's still +2pt (accept both "+2pt" and "+2 pt")
                self.assertIn(displayed_value.replace(" ", ""), ["+2pt"],
                    f"Expected '+2pt' or '+2 pt', got '{displayed_value}'")

    def test_negative_point_relative_font_size(self):
        """Test that -2pt font size is preserved in cell style."""
        with self.ui_test.create_doc_in_start_center("calc"):

            # Create a new cell style
            with self.ui_test.execute_dialog_through_command(
                    ".uno:StyleNewByExample") as xDialog:
                xStyleName = xDialog.getChild("stylename")
                xStyleName.executeAction("CLEAR", tuple())
                type_text(xStyleName, "Test NegRelative Size")

            # Edit the style and set -2pt
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test NegRelative Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Set -2pt
                xSizeField = xDialog.getChild("cbWestSize")
                xSizeField.executeAction("CLEAR", tuple())
                type_text(xSizeField, "-2pt")

            # Reopen and verify
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Test NegRelative Size",
                                            "Family": "CellStyles"}))) as xDialog:

                # Go to Font tab
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                # Read the value
                xSizeField = xDialog.getChild("cbWestSize")
                displayed_value = get_state_as_dict(xSizeField)["Text"]

                # Verify it's still -2pt (accept both "-2pt" and "-2 pt")
                self.assertIn(displayed_value.replace(" ", ""), ["-2pt"],
                    f"Expected '-2pt' or '-2 pt', got '{displayed_value}'")

    def test_edit_root_style_relative_font_size_no_crash(self):
        """Test that setting a relative font size on root Default style (no parent) does not crash."""
        with self.ui_test.create_doc_in_start_center("calc"):
            with self.ui_test.execute_blocking_action(
                    self.xUITest.executeCommandWithParameters,
                    args=(".uno:EditStyle",
                          mkPropertyValues({"Param": "Default",
                                            "Family": "CellStyles"}))) as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "2")
                xSizeField = xDialog.getChild("cbWestSize")
                xSizeField.executeAction("CLEAR", tuple())
                type_text(xSizeField, "80%")


# vim: set shiftwidth=4 softtabstop=4 expandtab:
