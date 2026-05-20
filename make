#!/usr/bin/env bash
# Wrapper de build pour MSYS2/MINGW64.
# Utilise le make de MSYS si disponible, sinon fallback vers mingw32-make.

PATH="/c/msys64/ucrt64/bin:/c/msys64/mingw64/bin:/usr/bin:/bin:$PATH"
export PATH

TMP=${TMP:-/c/msys64/tmp}
TEMP=${TEMP:-/c/msys64/tmp}
TMPDIR=${TMPDIR:-/c/msys64/tmp}
export TMP TEMP TMPDIR

if [ -x "/usr/bin/make" ]; then
  exec "/usr/bin/make" TMP=/c/msys64/tmp TEMP=/c/msys64/tmp TMPDIR=/c/msys64/tmp "$@"
elif [ -x "/c/msys64/usr/bin/make.exe" ]; then
  exec "/c/msys64/usr/bin/make.exe" TMP=/c/msys64/tmp TEMP=/c/msys64/tmp TMPDIR=/c/msys64/tmp "$@"
elif command -v mingw32-make >/dev/null 2>&1; then
  exec mingw32-make TMP=/c/msys64/tmp TEMP=/c/msys64/tmp TMPDIR=/c/msys64/tmp "$@"
else
  echo "Error: make not found. Install MSYS2 make or mingw32-make." >&2
  exit 1
fi
