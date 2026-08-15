# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,singleprov))

$(eval $(call gb_Library_use_sdk_api,singleprov))

$(eval $(call gb_Library_set_include,singleprov,\
    $$(INCLUDE) \
    -I$(SRCDIR)/scripting/source/inc \
))

$(eval $(call gb_Library_use_libraries,singleprov,\
    comphelper \
    cppu \
    cppuhelper \
    sal \
))

$(eval $(call gb_Library_add_exception_objects,singleprov,\
    scripting/source/singleprov/externaledit \
    scripting/source/singleprov/provcontext \
    scripting/source/singleprov/scriptbrowser \
    scripting/source/singleprov/scriptdir \
    scripting/source/singleprov/scriptfile \
    scripting/source/singleprov/scriptmacro \
    scripting/source/singleprov/scriptprovider \
))


$(eval $(call gb_Library_use_system_win32_libs,singleprov,\
    shell32 \
    shlwapi \
))

# vim: set noet sw=4 ts=4:
