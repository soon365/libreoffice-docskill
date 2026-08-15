#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
touch sfx2/inc/pch/precompiled_sfx.hxx include/sfx2/templatedlg.hxx
"$MAKE" -f Makefile.gbuild UIConfig_sfx Library_sfx > /c/docskill.ai/_dsstock_build2.log 2>&1
echo "make_exit=$?" >> /c/docskill.ai/_dsstock_build2.log
tail -n 3 /c/docskill.ai/_dsstock_build2.log
