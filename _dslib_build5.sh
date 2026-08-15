#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
rm -f workdir/CxxObject/sfx2/source/dialog/backingwindow.o
echo "=== build $(date) ===" > /c/docskill.ai/_dslib_build5.log
"$MAKE" -f Makefile.gbuild Library_sfx UIConfig_sfx >> /c/docskill.ai/_dslib_build5.log 2>&1
rc=$?
echo "make_exit=$rc" >> /c/docskill.ai/_dslib_build5.log
grep -aE 'backingwindow|startcenter|error C|error :|make_exit|\[build LNK\]|\[build UIC\]|FAILED' /c/docskill.ai/_dslib_build5.log | tail -n 25
echo "RC=$rc"
