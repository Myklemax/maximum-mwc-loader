#!/usr/bin/env bash
# maximus_daemon.sh
# Runs at login via autostart. Watches for maximus.log to appear
# (Steam launch script deletes it before each game session), then
# opens a gnome-terminal with the live log viewer automatically.
# Requires NO special permissions — runs normally on the host desktop.

LOADER_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME="/home/max/snap/steam/common/.local/share/Steam/steamapps/common/My Winter Car"
LOG="$GAME/maximus.log"
WATCH_CMD="cd \"$LOADER_DIR\" && python3 mwc_loader.py watch-log; exec bash"

echo "[maximus daemon] started, watching for $LOG"

while true; do
    # Wait until the log file does NOT exist (launch_maximus.sh deleted it)
    while [[ -f "$LOG" ]]; do sleep 1; done

    # Now wait for it to be CREATED (game launched and hook fired)
    while [[ ! -f "$LOG" ]]; do sleep 0.5; done

    echo "[maximus daemon] log appeared — opening terminal"

    # Open terminal — try direct path (works at login/autostart),
    # then flatpak-spawn fallback (works when run from VS Code terminal)
    if [[ -x "/usr/bin/gnome-terminal" ]]; then
        /usr/bin/gnome-terminal \
            --title="✦ MAXIMUS LOG ✦" \
            --geometry=100x30 \
            -- bash -c "$WATCH_CMD" &
    elif command -v flatpak-spawn >/dev/null 2>&1; then
        flatpak-spawn --host /usr/bin/gnome-terminal \
            --title="✦ MAXIMUS LOG ✦" \
            --geometry=100x30 \
            -- bash -c "$WATCH_CMD" &
    else
        echo "[maximus daemon] ERROR: could not find gnome-terminal"
    fi

    # Wait a moment so we don't double-spawn
    sleep 5
done
