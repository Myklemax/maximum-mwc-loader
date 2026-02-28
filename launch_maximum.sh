#!/usr/bin/env bash
# launch_maximum.sh — Steam Launch Options:
#   WINEDLLOVERRIDES=winmm=n,b "/home/max/Desktop/Maximus (MWC LOADER)/launch_maximum.sh" %command%

LOADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME="/home/max/snap/steam/common/.local/share/Steam/steamapps/common/My Winter Car"

# Write proof that this script ran
echo "=== script ran $(date) ===" > "$LOADER_DIR/launch_debug.log"
echo "args: $@" >> "$LOADER_DIR/launch_debug.log"
echo "WINEDLLOVERRIDES=$WINEDLLOVERRIDES" >> "$LOADER_DIR/launch_debug.log"

# Delete old log so daemon triggers fresh on next game session
rm -f "$GAME/maximum.log"

# Launch game — WINEDLLOVERRIDES is already set in the environment by Steam
exec "$@"
