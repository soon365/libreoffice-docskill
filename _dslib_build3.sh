#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
rm -f workdir/CxxObject/sfx2/source/dialog/backingwindow.o
"$MAKE" -f Makefile.gbuild Library_sfx > /c/docskill.ai/_dslib_build3.log 2>&1
rc=$?
grep -a 'backingwindow\|make_exit\|error C\|\[build LNK\]' /c/docskill.ai/_dslib_build3.log | tail -n 15
echo "make_exit=$rc" >> /c/docskill.ai/_dslib_build3.log
echo "RC=$rc"
