#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$ROOT_DIR"

GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
NC='\033[0m'
TAG='✦ MAXIMUS ✦'

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
Maximus installer (Linux)

Usage:
  ./install_maximus.sh [--game-path PATH] [--appid ID] [--run]

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

if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  if [[ -x "/var/run/host/usr/bin/x86_64-w64-mingw32-gcc" ]]; then
    export CC="/var/run/host/usr/bin/x86_64-w64-mingw32-gcc"
  elif [[ -x "/var/run/host/usr/bin/x86_64-w64-mingw32-gcc-win32" ]]; then
    export CC="/var/run/host/usr/bin/x86_64-w64-mingw32-gcc-win32"
  elif [[ -x "/usr/bin/x86_64-w64-mingw32-gcc-win32" ]]; then
    export CC="/usr/bin/x86_64-w64-mingw32-gcc-win32"
  else
    err "error: mingw-w64 compiler missing"
    warn "install on Debian/Ubuntu: sudo apt install mingw-w64"
    warn "if using Flatpak VS Code, run this script from your host terminal"
    exit 1
  fi
fi

python3 mwc_loader.py setup --game-path "$GAME_PATH" --no-run

# Install autostart entry for the log daemon
AUTOSTART_DIR="$HOME/.config/autostart"
mkdir -p "$AUTOSTART_DIR"
cat > "$AUTOSTART_DIR/maximus-daemon.desktop" << DESKTOP
[Desktop Entry]
Type=Application
Name=Maximus Log Daemon
Comment=Watches for My Winter Car maximus.log and opens the live log viewer
Exec=bash -c 'bash "${ROOT_DIR}/maximus_daemon.sh" &'
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
DESKTOP
log "autostart entry installed → $AUTOSTART_DIR/maximus-daemon.desktop"

# Start daemon now if not already running
if ! pgrep -f maximus_daemon.sh >/dev/null 2>&1; then
    setsid bash "$ROOT_DIR/maximus_daemon.sh" > /tmp/maximus_daemon.out 2>&1 &
    log "log daemon started (pid $!)"
else
    log "log daemon already running"
fi

log "installed successfully"
log "Steam Launch Options:"
log "  WINEDLLOVERRIDES=winmm=n,b \"${ROOT_DIR}/launch_maximus.sh\" %command%"

if [[ $RUN_AFTER_INSTALL -eq 1 ]]; then
  log "launching game..."
  python3 mwc_loader.py run --game-path "$GAME_PATH"
fi
