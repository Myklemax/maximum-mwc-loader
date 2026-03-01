#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$ROOT_DIR"

GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
NC='\033[0m'
TAG='✦ MAXIMUM ✦'

log() {
  echo -e "${GREEN}${TAG}${NC} $*"
}

warn() {
  echo -e "${YELLOW}${TAG}${NC} $*"
}

err() {
  echo -e "${RED}${TAG}${NC} $*"
}

GAME_PATH=""
APPID="4164420"
RUN_AFTER_INSTALL=0

print_help() {
  cat <<'EOF'
Maximum installer (Linux)

Usage:
  ./install_maximum.sh [--game-path PATH] [--appid ID] [--run]

Options:
  --game-path PATH   Game install directory (contains My Winter Car exe)
  --appid ID         Steam AppID for auto-detect (default: 4164420)
  --run              Launch game after install
  -h, --help         Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --game-path)
      GAME_PATH="${2:-}"
      shift 2
      ;;
    --appid)
      APPID="${2:-}"
      shift 2
      ;;
    --run)
      RUN_AFTER_INSTALL=1
      shift
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      err "Unknown option: $1"
      print_help
      exit 2
      ;;
  esac
done

if ! command -v python3 >/dev/null 2>&1; then
  err "error: python3 not found"
  exit 1
fi

if [[ -z "$GAME_PATH" ]]; then
  log "detecting game path via AppID $APPID..."
  GAME_PATH=$(python3 mwc_loader.py detect --appid "$APPID")
fi

if [[ ! -d "$GAME_PATH" ]]; then
  err "error: game path not found: $GAME_PATH"
  exit 1
fi

log "target game path: $GAME_PATH"
# Copy prebuilt binaries from this folder
WINMM="$ROOT_DIR/winmm.dll"
HOST="$ROOT_DIR/MaximumHost.dll"
if [[ ! -f "$WINMM" || ! -f "$HOST" ]]; then
  err "error: packaged binaries missing (winmm.dll or MaximumHost.dll)"
  exit 1
fi

cp -f "$WINMM" "$GAME_PATH/winmm.dll"
cp -f "$HOST" "$GAME_PATH/MaximumHost.dll"

# Copy managed loader(s)
mkdir -p "$GAME_PATH/mods"
for dll in "$ROOT_DIR"/mods/*.dll; do
  [[ -f "$dll" ]] || continue
  cp -f "$dll" "$GAME_PATH/mods/"
done
log "mods folder ready → $GAME_PATH/mods"

# Install autostart entry for the log daemon
AUTOSTART_DIR="$HOME/.config/autostart"
mkdir -p "$AUTOSTART_DIR"
cat > "$AUTOSTART_DIR/maximum-daemon.desktop" << DESKTOP
[Desktop Entry]
Type=Application
Name=Maximum Log Daemon
Comment=Watches for My Winter Car maximum.log and opens the live log viewer
Exec=bash -c 'bash "${ROOT_DIR}/maximum_daemon.sh" &'
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
DESKTOP
log "autostart entry installed → $AUTOSTART_DIR/maximum-daemon.desktop"

# Start daemon now if not already running
if ! pgrep -f maximum_daemon.sh >/dev/null 2>&1; then
    setsid bash "$ROOT_DIR/maximum_daemon.sh" > /tmp/maximum_daemon.out 2>&1 &
    log "log daemon started (pid $!)"
else
    log "log daemon already running"
fi

log "installed successfully"
log "Steam Launch Options:"
log "  WINEDLLOVERRIDES=winmm=n,b \"${ROOT_DIR}/launch_maximum.sh\" %command%"

if [[ $RUN_AFTER_INSTALL -eq 1 ]]; then
  log "launching game..."
  python3 mwc_loader.py run --game-path "$GAME_PATH"
fi
