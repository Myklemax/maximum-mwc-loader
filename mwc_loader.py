#!/usr/bin/env python3
"""
My Winter Car Linux-friendly loader helper (Maximus MVP).
- Auto-detects game path from Steam libraries.
- Installs Maximus native loader files (winhttp.dll + MaximusHost.dll).
- Launches the game via Proton or Wine with WinHTTP override.
"""

import argparse
import os
import re
import shutil
import subprocess
import time
from pathlib import Path

DEFAULT_APP_ID = "4164420"  # Steam AppID for My Winter Car; override with --appid if needed

STEAM_DEFAULTS = [
    Path("~/.steam/steam/steamapps").expanduser(),
    Path("~/.local/share/Steam/steamapps").expanduser(),
    Path("~/snap/steam/common/.local/share/Steam/steamapps").expanduser(),
]


def parse_libraryfolders(vdf_path: Path) -> list[Path]:
    """Parse Steam libraryfolders.vdf to collect library paths."""
    paths: list[Path] = []
    text = vdf_path.read_text(errors="ignore")
    for match in re.finditer(r'"(\d+)"\s+"([^"]+)"', text):
        _, raw_path = match.groups()
        paths.append(Path(raw_path).expanduser())
    return paths


def parse_appmanifest(appmanifest_path: Path) -> dict[str, str]:
    """Tiny parser to pull installdir from appmanifest_*.acf."""
    info: dict[str, str] = {}
    text = appmanifest_path.read_text(errors="ignore")
    for match in re.finditer(r'"(.*?)"\s+"(.*?)"', text):
        key, val = match.groups()
        info[key] = val
    return info


def find_game_install(app_id: str | None) -> Path | None:
    """Locate the game install by Steam appid. Returns None if not found."""
    if app_id is None:
        return None

    steam_roots: list[Path] = []
    for root in STEAM_DEFAULTS:
        library_vdf = root.parent / "libraryfolders.vdf"
        steam_roots.append(root)
        if library_vdf.exists():
            steam_roots.extend(parse_libraryfolders(library_vdf))

    seen = set()
    for steam_root in steam_roots:
        resolved = steam_root.resolve()
        if resolved in seen:
            continue
        seen.add(resolved)
        appmanifest = resolved / f"appmanifest_{app_id}.acf"
        if not appmanifest.exists():
            continue
        data = parse_appmanifest(appmanifest)
        installdir = data.get("installdir")
        if not installdir:
            continue
        candidate = steam_root / "common" / installdir
        if candidate.exists():
            return candidate
    return None


def find_game_path(args) -> Path:
    if args.game_path:
        return Path(args.game_path).expanduser().resolve()
    app_id = args.appid or os.environ.get("MWC_APPID") or DEFAULT_APP_ID
    path = find_game_install(app_id)
    if path:
        return path
    raise SystemExit("Game path not found. Pass --game-path or --appid (and ensure Steam is installed).")


def project_root() -> Path:
    return Path(__file__).resolve().parent


def native_dir() -> Path:
    return project_root() / "native"


def build_native() -> None:
    script = native_dir() / "build.sh"
    if not script.exists():
        raise SystemExit(f"Build script missing: {script}")
    try:
        subprocess.run(["bash", str(script)], check=True)
    except subprocess.CalledProcessError as exc:
        raise SystemExit(
            "Native build failed. Install mingw-w64 first (Ubuntu/Debian: sudo apt install mingw-w64). "
            "If using Flatpak VS Code, run the installer from your host terminal."
        ) from exc


def install_maximus(game_path: Path) -> None:
    src_root = native_dir()
    proxy_src = src_root / "winmm.dll"
    host_src = src_root / "MaximusHost.dll"

    if not proxy_src.exists() or not host_src.exists():
        print("Maximus binaries missing, building now...")
        build_native()

    if not proxy_src.exists() or not host_src.exists():
        raise SystemExit("Build finished but Maximus binaries were not found.")

    shutil.copy2(proxy_src, game_path / "winmm.dll")
    shutil.copy2(host_src, game_path / "MaximusHost.dll")

    mods_dir = game_path / "mods"
    mods_dir.mkdir(exist_ok=True)
    print(f"Installed Maximus to {game_path}")
    print(f"Mods folder: {mods_dir}")


def status_maximus(game_path: Path) -> None:
    winmm = game_path / "winmm.dll"
    host = game_path / "MaximusHost.dll"
    winmm_off = game_path / "winmm.dll.off"
    host_off = game_path / "MaximusHost.dll.off"
    log = game_path / "maximus.log"

    if winmm.exists() and host.exists():
        print("loader: ENABLED")
    elif winmm_off.exists() and host_off.exists():
        print("loader: DISABLED")
    else:
        print("loader: PARTIAL/UNKNOWN")

    print(f"winmm.dll: {'OK' if winmm.exists() else 'MISSING'}")
    print(f"MaximusHost.dll: {'OK' if host.exists() else 'MISSING'}")
    print(f"winmm.dll.off: {'found' if winmm_off.exists() else '-'}")
    print(f"MaximusHost.dll.off: {'found' if host_off.exists() else '-'}")
    # Warn if legacy broken proxies are still present
    for legacy in ["version.dll", "version.dll.off", "winhttp.dll", "winhttp.dll.off"]:
        if (game_path / legacy).exists():
            print(f"WARNING: {legacy} found — remove it (it will crash the game)")
    print(f"maximus.log: {'FOUND' if log.exists() else 'NOT FOUND YET'}")
    mods_dir = game_path / "mods"
    if mods_dir.exists():
        mods = sorted(mods_dir.glob("*.dll"))
        if mods:
            print(f"mods ({len(mods)}):")
            for m in mods:
                print(f"  {m.name}")
        else:
            print("mods folder: empty (drop .dll plugins here)")
    else:
        print("mods folder: MISSING (run install-maximus)")


def disable_loader(game_path: Path) -> None:
    winmm = game_path / "winmm.dll"
    host = game_path / "MaximusHost.dll"
    winmm_off = game_path / "winmm.dll.off"
    host_off = game_path / "MaximusHost.dll.off"

    if winmm.exists():
        if winmm_off.exists():
            winmm_off.unlink()
        winmm.rename(winmm_off)
    if host.exists():
        if host_off.exists():
            host_off.unlink()
        host.rename(host_off)

    print("Maximus loader disabled.")


def enable_loader(game_path: Path) -> None:
    winmm = game_path / "winmm.dll"
    host = game_path / "MaximusHost.dll"
    winmm_off = game_path / "winmm.dll.off"
    host_off = game_path / "MaximusHost.dll.off"

    if not winmm.exists() and winmm_off.exists():
        winmm_off.rename(winmm)
    if not host.exists() and host_off.exists():
        host_off.rename(host)

    if not winmm.exists() or not host.exists():
        raise SystemExit(
            "Could not enable loader because one or both DLLs are missing. "
            "Run install-maximus first."
        )

    print("Maximus loader enabled.")


def detect_proton() -> Path | None:
    candidates = []
    # Compatibility tools from Steam
    candidates.extend(Path("~/.steam/root/compatibilitytools.d").expanduser().glob("*/proton"))
    candidates.extend(Path("~/.local/share/Steam/compatibilitytools.d").expanduser().glob("*/proton"))
    candidates.extend(Path("~/snap/steam/common/.steam/root/compatibilitytools.d").expanduser().glob("*/proton"))
    candidates.extend(Path("~/snap/steam/common/.local/share/Steam/compatibilitytools.d").expanduser().glob("*/proton"))
    # Built-in Proton installs
    candidates.extend(Path("~/.steam/root/steamapps/common").expanduser().glob("Proton*/*/proton"))
    candidates.extend(Path("~/.local/share/Steam/steamapps/common").expanduser().glob("Proton*/*/proton"))
    candidates.extend(Path("~/snap/steam/common/.steam/root/steamapps/common").expanduser().glob("Proton*/*/proton"))
    candidates.extend(Path("~/snap/steam/common/.local/share/Steam/steamapps/common").expanduser().glob("Proton*/*/proton"))
    for cand in sorted(candidates, reverse=True):  # prefer newer-looking names
        if cand.is_file():
            return cand
    return None


def find_game_exe(game_path: Path) -> Path:
    for guess in ("MyWinterCar.exe", "Game.exe", "My Winter Car.exe"):
        cand = game_path / guess
        if cand.exists():
            return cand
    exes = list(game_path.glob("*.exe"))
    if not exes:
        raise SystemExit("No .exe found in game directory. Provide --game-path pointing at the correct folder.")
    return exes[0]


def run_game(game_path: Path, proton_path: Path | None) -> None:
    exe = find_game_exe(game_path)
    env = os.environ.copy()
    env.setdefault("WINEDEBUG", "-all")

    if proton_path:
        cmd = [str(proton_path), "run", str(exe)]
    else:
        cmd = ["wine", str(exe)]
    print("Launching:", " ".join(cmd))
    subprocess.run(cmd, env=env)


def watch_log(game_path: Path) -> None:
    G     = "\033[92m"   # bright green
    DG    = "\033[32m"   # dark green
    BOLD  = "\033[1m"
    R     = "\033[0m"    # reset

    art = [
        r" __       __                      __                                       ",
        r"/  \     /  |                    /  |                                      ",
        r"$$  \   /$$ |  ______   __    __ $$/  _____  ____   __    __  _____  ____  ",
        r"$$$  \ /$$$ | /      \ /  \  /  |/  |/     \/    \ /  |  /  |/     \/    \ ",
        r"$$$$  /$$$$ | $$$$$$  |$$  \/$$/ $$ |$$$$$$ $$$$  |$$ |  $$ |$$$$$$ $$$$  |",
        r"$$ $$ $$/$$ | /    $$ | $$  $$<  $$ |$$ | $$ | $$ |$$ |  $$ |$$ | $$ | $$ |",
        r"$$ |$$$/ $$ |/$$$$$$$ | /$$$$  \ $$ |$$ | $$ | $$ |$$ \__$$ |$$ | $$ | $$ |",
        r"$$ | $/  $$ |$$    $$ |/$$/ $$  |$$ |$$ | $$ | $$ |$$    $$/ $$ | $$ | $$ |",
        r"$$/      $$/  $$$$$$$/ $$/   $$/ $$/ $$/  $$/  $$/  $$$$$$/  $$/  $$/  $$/ ",
    ]

    print()
    for line in art:
        print(f"{G}{BOLD}{line}{R}")
    bar = "─" * 64
    print(f"{DG}{bar}{R}")
    print(f"{DG}  MWC LOADER  ·  Live Hook Log  ·  Ctrl+C to exit{R}")
    print(f"{DG}{bar}{R}")
    print()

    log_path = game_path / "maximus.log"
    pos = 0

    def decode(data: bytes) -> str:
        for enc in ("utf-8", "latin-1"):
            try:
                return data.decode(enc)
            except Exception:
                pass
        return data.decode("latin-1", errors="replace")

    def print_lines(text: str) -> None:
        for line in text.splitlines():
            line = line.strip("\r\n ")
            if line:
                print(f"{G}  {line}{R}", flush=True)

    if log_path.exists():
        print_lines(decode(log_path.read_bytes()))
        pos = log_path.stat().st_size
    else:
        print(f"{DG}  [waiting for game launch...]{R}", flush=True)

    print(f"{DG}  [watching {log_path}]{R}")
    print()

    try:
        while True:
            time.sleep(0.4)
            if not log_path.exists():
                continue
            size = log_path.stat().st_size
            if size > pos:
                with open(log_path, "rb") as f:
                    f.seek(pos)
                    new_data = f.read()
                pos = size
                print_lines(decode(new_data))
    except KeyboardInterrupt:
        print(f"\n{DG}  [log closed]{R}\n")


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="My Winter Car Maximus loader helper (Linux/Proton)")
    parent = argparse.ArgumentParser(add_help=False)
    parent.add_argument("--game-path", help="Path to game directory (contains the .exe)")
    parent.add_argument("--appid", help="Steam AppID for auto-detect")

    parser.add_argument("--game-path", help="Path to game directory (contains the .exe)")
    parser.add_argument("--appid", help="Steam AppID for auto-detect")
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("detect", help="Show detected game path", parents=[parent])
    sub.add_parser("build-native", help="Build winmm.dll and MaximusHost.dll with mingw-w64")
    sub.add_parser("install-maximus", help="Install Maximus DLLs into game directory", parents=[parent])
    setup_p = sub.add_parser("setup", help="Build, install, check status, and optionally run", parents=[parent])
    setup_p.add_argument("--proton", help="Path to proton script; defaults to auto-detect")
    setup_p.add_argument("--no-run", action="store_true", help="Only build/install/status; do not launch game")
    sub.add_parser("status", help="Show Maximus install status in game directory", parents=[parent])
    sub.add_parser("enable-loader", help="Enable Maximus by restoring DLLs", parents=[parent])
    sub.add_parser("disable-loader", help="Disable Maximus by renaming DLLs to .off", parents=[parent])
    sub.add_parser("watch-log", help="Live green log viewer — shows maximus.log as it updates", parents=[parent])

    run_p = sub.add_parser("run", help="Launch the game via Proton or Wine", parents=[parent])
    run_p.add_argument("--proton", help="Path to proton script; defaults to auto-detect")

    args = parser.parse_args(argv)
    if args.command is None:
        parser.print_help()
        return

    if args.command == "build-native":
        build_native()
        return

    if args.command == "detect":
        game_path = find_game_path(args)
        print(game_path)
        return

    game_path = find_game_path(args)

    if args.command == "install-maximus":
        install_maximus(game_path)
    elif args.command == "setup":
        install_maximus(game_path)
        status_maximus(game_path)
        if not args.no_run:
            proton = Path(args.proton).expanduser() if args.proton else detect_proton()
            if proton is None:
                print("Proton not found; falling back to Wine.")
            run_game(game_path, proton)
    elif args.command == "status":
        status_maximus(game_path)
    elif args.command == "enable-loader":
        enable_loader(game_path)
    elif args.command == "disable-loader":
        disable_loader(game_path)
    elif args.command == "watch-log":
        watch_log(game_path)
    elif args.command == "run":
        proton = Path(args.proton).expanduser() if args.proton else detect_proton()
        if proton is None:
            print("Proton not found; falling back to Wine.")
        run_game(game_path, proton)


if __name__ == "__main__":
    main()
