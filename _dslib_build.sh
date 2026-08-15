#!/usr/bin/env bash
set -o pipefail
cd /c/docskill.ai/LibreOffice || exit 3
# Touch sources so gbuild rebuilds even if mtime races
touch sfx2/inc/docskilltemplatetags.hxx
touch sfx2/source/dialog/backingwindow.cxx
touch sfx2/source/control/templatedefaultview.cxx
MAKE=/c/Users/chenjiang/bin/make.exe
echo "=== build start $(date) ===" > /c/docskill.ai/_dslib_build.log
"$MAKE" -f Makefile.gbuild Library_sfx >> /c/docskill.ai/_dslib_build.log 2>&1
rc=$?
echo "make_exit=$rc" >> /c/docskill.ai/_dslib_build.log
tail -n 25 /c/docskill.ai/_dslib_build.log
echo "RC=$rc"
