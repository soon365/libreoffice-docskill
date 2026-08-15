#!/bin/sh

set -eu

vs_version="$1"
target_file="$2"

sed -i -e "s/-latest/${vs_version}/" "$target_file"
sed -i -e "s/encoding=locale.getpreferredencoding(False))/encoding=locale.getpreferredencoding(False), errors='replace')/" "$target_file"
sed -i -e "s/subprocess.Popen(args, universal_newlines=True, encoding=encoding, close_fds=False,/subprocess.Popen(args, universal_newlines=True, encoding=encoding, errors='replace', close_fds=False,/" mesonbuild/utils/universal.py