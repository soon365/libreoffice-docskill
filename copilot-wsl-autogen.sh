#!/bin/bash
set -e

export WSL_ONLY_AS_HELPER=TRUE
export PROGRAMFILESX86=/mnt/c/PROGRA~2
export PATH="/mnt/c/PROGRA~1/Git/mingw64/bin:/mnt/c/PROGRA~1/Git/usr/bin:/mnt/c/Users/chenjiang/bin:/mnt/c/WINDOWS/system32:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

cd /mnt/c/docskill.ai/LibreOffice
./autogen.sh