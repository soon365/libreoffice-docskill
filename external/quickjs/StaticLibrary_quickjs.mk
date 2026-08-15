# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_StaticLibrary_StaticLibrary,quickjs))

$(eval $(call gb_StaticLibrary_use_unpacked,quickjs,quickjs))

$(eval $(call gb_StaticLibrary_set_warnings_disabled,quickjs))

$(eval $(call gb_StaticLibrary_add_generated_cobjects,quickjs, \
    UnpackedTarball/quickjs/quickjs-amalgam \
))

ifeq ($(OS),WNT)
$(eval $(call gb_StaticLibrary_add_cflags,quickjs,\
    /std:c17 /experimental:c11atomics \
))
endif

# vim: set noet sw=4 ts=4:
