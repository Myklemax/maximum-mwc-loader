#!/usr/bin/env bash
# setup_requirements.sh
# Installs everything needed to build and run Maximus MWC Loader.
# Supports Ubuntu, Debian, Linux Mint, Arch, Fedora.
# Run once on a fresh PC: bash setup_requirements.sh

set -e

G="\033[92m"
Y="\033[93m"
R="\033[91m"
B="\033[1m"
X="\033[0m"

banner() {
    echo -e "${G}${B}"
    echo ' __       __                      __                                       '
    echo '/  \     /  |                    /  |                                      '
    echo '$$  \   /$$ |  ______   __    __ $$/  _____  ____   __    __  _____  ____  '
    echo '$$$  \ /$$$ | /      \ /  \  /  |/  |/     \/    \ /  |  /  |/     \/    \ '
    echo '$$$$  /$$$$ | $$$$$$  |$$  \/$$/ $$ |$$$$$$ $$$$  |$$ |  $$ |$$$$$$ $$$$  |'
    echo '$$ $$ $$/$$ | /    $$ | $$  $$<  $$ |$$ | $$ | $$ |$$ |  $$ |$$ | $$ | $$ |'
    echo '$$ |$$$/ $$ |/$$$$$$$ | /$$$$  \ $$ |$$ | $$ | $$ |$$ \__$$ |$$ | $$ | $$ |'
    echo '$$ | $/  $$ |$$    $$ |/$$/ $$  |$$ |$$ | $$ | $$ |$$    $$/ $$ | $$ | $$ |'
    echo '$$/ $$/ $$/  $$$$$$$/ $$/   $$/ $$/ $$/  $$/  $$/  $$$$$$/  $$/  $$/  $$/ '
    echo -e "${X}"
    echo -e "${G}──────────────────────────────────────────────────────────────────${X}"
    echo -e "${G}  Requirements Installer${X}"
    echo -e "${G}──────────────────────────────────────────────────────────────────${X}"
    echo
}

log()  { echo -e "${G}[maximus]${X} $*"; }
warn() { echo -e "${Y}[warning]${X} $*"; }
die()  { echo -e "${R}[error]${X} $*"; exit 1; }

banner

# ── Detect distro ────────────────────────────────────────────────────────────
if command -v apt-get >/dev/null 2>&1; then
    DISTRO="debian"
elif command -v pacman >/dev/null 2>&1; then
    DISTRO="arch"
elif command -v dnf >/dev/null 2>&1; then
    DISTRO="fedora"
else
    die "Unsupported distro. Install manually: python3, mingw-w64, git, gnome-terminal"
fi

log "Detected distro family: $DISTRO"
echo

# ── Install packages ─────────────────────────────────────────────────────────
case "$DISTRO" in
    debian)
        log "Updating package lists..."
        sudo apt-get update -qq
        log "Installing: python3, mingw-w64, git, gnome-terminal..."
        sudo apt-get install -y \
            python3 \
            mingw-w64 \
            git \
            gnome-terminal
        ;;
    arch)
        log "Installing: python3, mingw-w64, git, gnome-terminal..."
        sudo pacman -Sy --noconfirm \
            python \
            mingw-w64-gcc \
            git \
            gnome-terminal
        ;;
    fedora)
        log "Installing: python3, mingw-w64, git, gnome-terminal..."
        sudo dnf install -y \
            python3 \
            mingw64-gcc \
            git \
            gnome-terminal
        ;;
esac

echo

# ── Verify everything installed ───────────────────────────────────────────────
PASS=1

check() {
    local cmd=$1
    local label=$2
    if command -v "$cmd" >/dev/null 2>&1; then
        log "$label — $(command -v $cmd)"
    else
        warn "$label NOT FOUND — install failed?"
        PASS=0
    fi
}

check python3            "python3"
check x86_64-w64-mingw32-gcc "mingw-w64 (x86_64)"
check git                "git"
check gnome-terminal     "gnome-terminal"

echo
if [[ $PASS -eq 1 ]]; then
    echo -e "${G}${B}  All requirements installed successfully!${X}"
    echo
    echo -e "${G}  Next steps:${X}"
    echo -e "${G}    1. Run the Maximus installer:${X}"
    echo -e "${G}       bash install_maximus.sh${X}"
    echo -e "${G}    2. Set Steam Launch Options:${X}"
    echo -e "${G}       WINEDLLOVERRIDES=winmm=n,b \"/path/to/launch_maximus.sh\" %command%${X}"
    echo -e "${G}    3. Start the log daemon:${X}"
    echo -e "${G}       bash maximus_daemon.sh &${X}"
else
    warn "Some packages may not have installed correctly. Check the output above."
fi
echo
