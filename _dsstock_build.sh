#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
git apply --reverse /c/docskill.ai/patches/libreoffice-template-manager-docskill-tabs.patch || { echo "REVERSE_FAILED"; exit 4; }
echo "reverted; header present? $(test -f sfx2/inc/docskilltemplatetags.hxx && echo yes || echo no)"
touch sfx2/source/dialog/backingwindow.cxx sfx2/source/doc/templatedlg.cxx sfx2/source/control/templatedefaultview.cxx
"$MAKE" -f Makefile.gbuild UIConfig_sfx Library_sfx > /c/docskill.ai/_dsstock_build.log 2>&1
echo "make_exit=$?" >> /c/docskill.ai/_dsstock_build.log
tail -n 3 /c/docskill.ai/_dsstock_build.log
