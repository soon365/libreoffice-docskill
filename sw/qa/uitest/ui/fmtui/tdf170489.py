# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
"""Tests for tdf#170489 - Inheritance when setting paragraph style font size
to 100% or +0pt."""

from uitest.framework import UITestCase
from uitest.uihelper.common import select_pos
from uitest.uihelper.common import type_text
from com.sun.star.beans.PropertyState import DIRECT_VALUE, DEFAULT_VALUE
from libreoffice.uno.propertyvalue import mkPropertyValues


class tdf170489(UITestCase):

    def _create_child_style_with_font_size(self, styleName, fontSize):
        """Create a new paragraph style with an explicit font size."""
        # Create a new style
        with self.ui_test.execute_dialog_through_command(
                ".uno:StyleNewByExample") as xDialog:
            xStyleName = xDialog.getChild("stylename")
            type_text(xStyleName, styleName)

        # Set parent and change font size
        with self.ui_test.execute_dialog_through_command(
                ".uno:EditStyle") as xDialog:
            xTabs = xDialog.getChild("tabcontrol")
            # Tab 0 = Organizer: set parent
            select_pos(xTabs, "0")
            xLinkedWith = xTabs.getChild("linkedwith")
            xLinkedWith.executeAction("SELECT",
                mkPropertyValues({"TEXT": "Default Paragraph Style"}))
            # Tab 1 = Character: change font size
            select_pos(xTabs, "1")
            xSizeWest = xTabs.getChild("cbWestSize")
            xSizeWest.executeAction("CLEAR", tuple())
            type_text(xSizeWest, fontSize)

    def test_font_size_inherit_percentage(self):
        """Setting font size to 100% should restore inheritance from parent."""
        with self.ui_test.create_doc_in_start_center("writer"):
            document = self.ui_test.get_component()
            xParaStyles = document.StyleFamilies.ParagraphStyles

            self._create_child_style_with_font_size("Test170489Pct", "20pt")

            xStyle = xParaStyles.getByName("Test170489Pct")

            # Verify font size was applied as a direct (explicit) value
            self.assertEqual(xStyle.getPropertyValue("CharHeight"), 20)
            self.assertEqual(xStyle.getPropertyState("CharHeight"),
                DIRECT_VALUE)

            # Open style dialog, set font size to 100% (= inherit from parent)
            with self.ui_test.execute_dialog_through_command(
                    ".uno:EditStyle") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "1")
                xSizeWest = xTabs.getChild("cbWestSize")
                xSizeWest.executeAction("CLEAR", tuple())
                type_text(xSizeWest, "100%")

            # Font size should now be truly inherited (not a direct value)
            self.assertEqual(xStyle.getPropertyState("CharHeight"),
                DEFAULT_VALUE)

    def test_font_size_inherit_pt_relative(self):
        """Setting font size to +0pt should restore inheritance from parent."""
        with self.ui_test.create_doc_in_start_center("writer"):
            document = self.ui_test.get_component()
            xParaStyles = document.StyleFamilies.ParagraphStyles

            self._create_child_style_with_font_size("Test170489Pt", "20pt")

            xStyle = xParaStyles.getByName("Test170489Pt")

            # Verify font size was applied as a direct (explicit) value
            self.assertEqual(xStyle.getPropertyValue("CharHeight"), 20)
            self.assertEqual(xStyle.getPropertyState("CharHeight"),
                DIRECT_VALUE)

            # Open style dialog, set font size to +0pt (= inherit from parent)
            with self.ui_test.execute_dialog_through_command(
                    ".uno:EditStyle") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "1")
                xSizeWest = xTabs.getChild("cbWestSize")
                xSizeWest.executeAction("CLEAR", tuple())
                type_text(xSizeWest, "+0pt")

            # Font size should now be truly inherited (not a direct value)
            self.assertEqual(xStyle.getPropertyState("CharHeight"),
                DEFAULT_VALUE)

# vim: set shiftwidth=4 softtabstop=4 expandtab:
