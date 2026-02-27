#!/usr/bin/env bash
set -euo pipefail

# Build Maximus native DLLs using mingw-w64 (x86_64)
# Requires: x86_64-w64-mingw32-gcc

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"

PROXY_OUT=winmm.dll
PROXY_SRC=winmm_maximus.c
PROXY_DEF=winmm.def

HOST_OUT=MaximusHost.dll
HOST_SRC=maximus_host.c
HOST_DEF=maximus_host.def
CC=${CC:-x86_64-w64-mingw32-gcc}
CFLAGS="-O2 -Wall -Wextra -s"
LDFLAGS="-shared"

if ! command -v "$CC" >/dev/null 2>&1; then
	if [[ -x "/var/run/host/usr/bin/x86_64-w64-mingw32-gcc" ]]; then
		CC="/var/run/host/usr/bin/x86_64-w64-mingw32-gcc"
	elif [[ -x "/var/run/host/usr/bin/x86_64-w64-mingw32-gcc-win32" ]]; then
		CC="/var/run/host/usr/bin/x86_64-w64-mingw32-gcc-win32"
	elif [[ -x "/usr/bin/x86_64-w64-mingw32-gcc-win32" ]]; then
		CC="/usr/bin/x86_64-w64-mingw32-gcc-win32"
	else
		echo "[build] error: compiler '$CC' not found"
		echo "[build] install mingw-w64 (Ubuntu/Debian: sudo apt install mingw-w64)"
		echo "[build] if using Flatpak VS Code, run installer from a host terminal"
		exit 127
	fi
fi

echo "[build] $CC $PROXY_SRC + $PROXY_DEF -> $PROXY_OUT"
$CC $PROXY_SRC $CFLAGS $LDFLAGS -o "$PROXY_OUT" "$PROXY_DEF"

echo "[build] $CC $HOST_SRC + $HOST_DEF -> $HOST_OUT"
$CC $HOST_SRC $CFLAGS $LDFLAGS -o "$HOST_OUT" "$HOST_DEF"

echo "[build] done: $PROXY_OUT, $HOST_OUT"
