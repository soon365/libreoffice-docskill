# -*- tab-width: 4; indent-tabs-mode: nil; py-indent-offset: 4 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
from uitest.framework import UITestCase
from uitest.uihelper.common import get_state_as_dict, type_text
from libreoffice.uno.propertyvalue import mkPropertyValues

class EditDictionaryFilterTest(UITestCase):
    def test_dictionary_live_filter(self):
        with self.ui_test.create_doc_in_start_center("writer"):
            # 1. Open Tools -> Options -> Language Settings -> Writing Aids
            with self.ui_test.execute_dialog_through_command(".uno:OptionsTreeDialog", close_button="cancel") as xOptionsDlg:
                xPages = xOptionsDlg.getChild("pages")
                xLangSettings = xPages.getChild('2') # Language Settings
                xLangSettings.executeAction("EXPAND", tuple())
                xWritingAids = xLangSettings.getChild('1') # Writing Aids
                xWritingAids.executeAction("SELECT", tuple())

                dictionaries = xOptionsDlg.getChild("lingudicts")
                hasEditableDictionary = False
                for i in dictionaries.getChildren():
                    entry = dictionaries.getChild(i)
                    entry_label = get_state_as_dict(entry)["Text"]
                    if entry_label == "List of Ignored Words [All]":
                        hasEditableDictionary = True
                        entry.executeAction("SELECT", tuple())
                        break

                self.assertTrue(hasEditableDictionary, "No editable dictionary found")

                # 2. Click "Edit..." button under User-defined dictionaries
                xEditDictBtn = xOptionsDlg.getChild("lingudictsedit")

                with self.ui_test.execute_blocking_action(xEditDictBtn.executeAction, args=("CLICK", ()), close_button="close") as xDictDlg:
                    xWordInput = xDictDlg.getChild("word")
                    xWordsList = xDictDlg.getChild("words")

                    # 3. Add a new test word to ensure there is something to filter
                    xWordInput.executeAction("TYPE", mkPropertyValues({"TEXT": "uitestword"}))
                    xNewBtn = xDictDlg.getChild("newreplace")
                    xNewBtn.executeAction("CLICK", tuple())

                    # 4. Prove the filter HIDES words (type gibberish)
                    xWordInput.executeAction("TYPE", mkPropertyValues({"KEYCODE": "CTRL+A"}))
                    xWordInput.executeAction("TYPE", mkPropertyValues({"TEXT": "zzxyy"}))

                    children_count = int(get_state_as_dict(xWordsList)["Children"])
                    self.assertEqual(children_count, 0, "Filter should hide words and return 0 matches for gibberish")

                    # 5. Prove the filter FINDS the correct word
                    xWordInput.executeAction("TYPE", mkPropertyValues({"KEYCODE": "CTRL+A"}))
                    xWordInput.executeAction("TYPE", mkPropertyValues({"TEXT": "uitest"}))

                    children_count = int(get_state_as_dict(xWordsList)["Children"])
                    self.assertTrue(children_count >= 1, "Filter should return at least 1 match for prefix")

                    # 6. Safe Clean up: Filter for the exact word first so it's guaranteed at POS: 0
                    xWordInput.executeAction("TYPE", mkPropertyValues({"KEYCODE": "CTRL+A"}))
                    xWordInput.executeAction("TYPE", mkPropertyValues({"TEXT": "uitestword"}))

                    xDelBtn = xDictDlg.getChild("delete")
                    xDelBtn.executeAction("CLICK", tuple())

# vim: set shiftwidth=4 softtabstop=4 expandtab:
