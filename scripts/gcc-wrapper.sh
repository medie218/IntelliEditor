#!/usr/bin/env bash
set -euo pipefail

PATH="/c/msys64/ucrt64/bin:/c/msys64/mingw64/bin:/usr/bin:/bin:$PATH"
export PATH

TMP="/c/msys64/tmp"
TEMP="/c/msys64/tmp"
TMPDIR="/c/msys64/tmp"

export TMP TEMP TMPDIR

exec gcc "$@"
