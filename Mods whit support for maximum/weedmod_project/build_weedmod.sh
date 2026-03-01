#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"

CC=${CC:-x86_64-w64-mingw32-gcc}
CFLAGS="-O2 -Wall -s"
LDFLAGS="-shared"

if ! command -v "$CC" >/dev/null 2>&1; then
    for try in \
        /var/run/host/usr/bin/x86_64-w64-mingw32-gcc \
        /var/run/host/usr/bin/x86_64-w64-mingw32-gcc-win32 \
        /usr/bin/x86_64-w64-mingw32-gcc-win32; do
        if [[ -x "$try" ]]; then CC="$try"; break; fi
    done
fi

echo "[weedmod] $CC weedmod.c -> weedmod.dll"
$CC weedmod.c $CFLAGS $LDFLAGS -o weedmod.dll
echo "[weedmod] done"
