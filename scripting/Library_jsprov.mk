# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,jsprov))

$(eval $(call gb_Library_set_componentfile,jsprov,scripting/source/jsprov/jsprov,services))

$(eval $(call gb_Library_use_sdk_api,jsprov))

$(eval $(call gb_Library_set_include,jsprov,\
    $$(INCLUDE) \
    -I$(SRCDIR)/scripting/source/inc \
))

$(eval $(call gb_Library_use_libraries,jsprov,\
    comphelper \
    cppu \
    cppuhelper \
    jsuno \
    sal \
    singleprov \
))

$(eval $(call gb_Library_add_exception_objects,jsprov,\
    scripting/source/jsprov/loader \
))

# vim: set noet sw=4 ts=4:
