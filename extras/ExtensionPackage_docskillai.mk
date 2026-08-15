# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# DocSkill AI sidebar agent extension, bundled into the installation
# as a share/extensions bundled extension (auto-registered on first
# program start).
#
# The prebuilt .oxt is produced by C:\docskill.ai\docskill-ai-ext\build.bat
# (pack_oxt.ps1) and lives outside the LibreOffice source tree.

$(eval $(call gb_ExtensionPackage_ExtensionPackage_internal,docskillai,$(SRCDIR)/../docskill-ai-ext/DocSkillAI.oxt))

$(eval $(call gb_Module_register_target,$(call gb_ExtensionPackage_get_target,docskillai),$(call gb_ExtensionPackage_get_clean_target,docskillai)))

$(eval $(call gb_Helper_make_userfriendly_targets,docskillai,ExtensionPackage))

# vim: set noet sw=4 ts=4:
