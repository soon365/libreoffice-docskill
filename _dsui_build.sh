#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
"$MAKE" -f Makefile.gbuild UIConfig_sfx > /c/docskill.ai/_dsui_build.log 2>&1
echo "make_exit=$?" >> /c/docskill.ai/_dsui_build.log
tail -n 4 /c/docskill.ai/_dsui_build.log
