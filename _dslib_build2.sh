#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
MAKE=/c/Users/chenjiang/bin/make.exe
# Force recompile of backingwindow by removing its object
rm -f workdir/CxxObject/sfx2/source/dialog/backingwindow.o
rm -f workdir/Dep/CxxObject/sfx2/source/dialog/backingwindow.d
echo "=== forced rebuild start $(date) ===" > /c/docskill.ai/_dslib_build2.log
"$MAKE" -f Makefile.gbuild Library_sfx >> /c/docskill.ai/_dslib_build2.log 2>&1
rc=$?
echo "make_exit=$rc" >> /c/docskill.ai/_dslib_build2.log
grep -E 'backingwindow|error |make_exit|LNK' /c/docskill.ai/_dslib_build2.log | tail -n 30
echo "RC=$rc"
