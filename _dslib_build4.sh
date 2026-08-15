#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
rm -f workdir/CxxObject/sfx2/source/dialog/backingwindow.o
echo "=== build $(date) ===" > /c/docskill.ai/_dslib_build4.log
"$MAKE" -f Makefile.gbuild Library_sfx UIConfig_sfx >> /c/docskill.ai/_dslib_build4.log 2>&1
rc=$?
echo "make_exit=$rc" >> /c/docskill.ai/_dslib_build4.log
grep -aE 'backingwindow|startcenter|error C|make_exit|\[build LNK\]|\[build UIC\]' /c/docskill.ai/_dslib_build4.log | tail -n 20
echo "RC=$rc"
