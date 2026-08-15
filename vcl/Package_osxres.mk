# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
res_WRKDIR := $(gb_CustomTarget_workdir)/vcl/osx/res

$(res_WRKDIR)/.dir:
	mkdir -p $(@D) && touch $@

$(res_WRKDIR)/MainMenu.nib: $(SRCDIR)/vcl/osx/res/MainMenu.xib | $(res_WRKDIR)/.dir
	$(call gb_Output_announce,$(@F),$(true),NIB,2)
	ibtool --compile "$@" "$<"

$(res_WRKDIR)/MenuTranslations.plist: $(SRCDIR)/vcl/osx/res/MenuTranslations.plist | $(res_WRKDIR)/.dir
	cp "$<" "$@"

$(eval $(call gb_Package_Package,vcl_osxres,$(res_WRKDIR)))
$(call gb_Package_get_clean_target,vcl_osxres): $(call gb_CustomTarget_get_clean_target,vcl/osx/res)

$(eval $(call gb_Package_add_files_with_dir,vcl_osxres,Resources,\
	MainMenu.nib \
	MenuTranslations.plist \
))

# vim:set noet sw=4 ts=4:
