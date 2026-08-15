# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import select_pos, get_state_as_dict, get_url_for_data_file

class tdf116833(UITestCase):
    def test_tdf116833_reset_document_properties(self):
        with self.ui_test.load_file(get_url_for_data_file("tdf116833.odt")) as document:
            # Verify initial document metadata
            xDocProps = document.getDocumentProperties()
            self.assertEqual(xDocProps.ModifiedBy, "Foo Bar")

            # Open document properties and reset metadata
            with self.ui_test.execute_dialog_through_command(".uno:SetDocumentProperties") as xDialog:
                xTabs = xDialog.getChild("tabcontrol")
                select_pos(xTabs, "0")

                # Uncheck save preview image
                xThumbnailSaveCheckbox = xDialog.getChild("thumbnailsavecb")
                if get_state_as_dict(xThumbnailSaveCheckbox)["Selected"] == "true":
                    xThumbnailSaveCheckbox.executeAction("CLICK", tuple())
                self.assertEqual(get_state_as_dict(xThumbnailSaveCheckbox)["Selected"], "false")

                xResetBtn = xDialog.getChild("reset")
                xResetBtn.executeAction("CLICK", tuple())

            # Save and reload document
            self.xUITest.executeCommand(".uno:Save")
            self.xUITest.executeCommand(".uno:Reload")
            document = self.ui_test.get_component()

            # Without the fix in place, this test would have failed with
            # AssertionError: "Foo Bar" != ""
            # i.e. the metadata of the document was not deleted
            xDocProps = document.getDocumentProperties()
            self.assertEqual(xDocProps.ModifiedBy, "")

# vim: set shiftwidth=4 softtabstop=4 expandtab:
