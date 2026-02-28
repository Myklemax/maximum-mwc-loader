# Maximum MWC Loader — Installation Guide

A mod loader for **My Winter Car** on Linux + Steam (Proton).

---

## What's in this folder

| File | Purpose |
|------|---------|
| `winmm.dll` | Proxy DLL — hooks into the game at launch |
| `MaximumHost.dll` | Mod host — loads your plugins from `mods/` |
| `install_maximum.sh` | One-click installer |
| `launch_maximum.sh` | Steam launch wrapper (set in Steam) |
| `maximum_daemon.sh` | Background daemon — auto-opens the log terminal |
| `mwc_loader.py` | Loader helper (used by the scripts above) |
| `mods/` | Place your native `.dll` mods here (empty by default) |

---

## Requirements

- Linux (Ubuntu, Debian, Mint, Arch, or Fedora)
- Steam (native or Snap)
- Proton (any version — auto-detected)
- **Python 3** — check with: `python3 --version`
- **gnome-terminal** — check with: `gnome-terminal --version`

If anything is missing, run the requirements installer first:

```bash
bash setup_requirements.sh
```

_(Only needed on a fresh PC — most Linux desktops already have everything.)_

---

## Quick install (3 steps)

1) Put this folder somewhere you keep games/mods (example: `~/maximum-mwc-loader`)
2) Open a terminal **inside the folder** and run:
  ```bash
  bash install_maximum.sh
  ```
  It installs the DLLs, creates `mods/`, sets up the log daemon, and prints the Steam Launch Options line.
3) In Steam → My Winter Car → Properties → Launch Options, paste the printed line (it looks like this, with your path):
  ```
  WINEDLLOVERRIDES=winmm=n,b "/home/you/maximum-mwc-loader/launch_maximum.sh" %command%
  ```

Then launch the game. A green terminal **✦ MAXIMUM LOG ✦** will pop up automatically.

---

## Installing mods

Drop any `.dll` mod plugin into the `mods/` folder inside your **game directory**:

```
My Winter Car/
  mods/
    MyMod.dll       ← put mod DLLs here
    AnotherMod.dll
```

Mods are loaded automatically every time the game starts. No reinstall needed.

---

## Useful commands

Run these from the loader folder:

```bash
python3 mwc_loader.py status           # Check if loader is installed correctly
python3 mwc_loader.py disable-loader   # Temporarily disable (rename DLLs to .off)
python3 mwc_loader.py enable-loader    # Re-enable loader
python3 mwc_loader.py watch-log        # Open the live log viewer manually
```

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| Game crashes on launch | Make sure `WINEDLLOVERRIDES=winmm=n,b` is in Launch Options |
| Log terminal doesn't open | Run `bash maximum_daemon.sh &` in a normal terminal (`Ctrl+Alt+T`) |
| `maximum.log` never appears | Check that `winmm.dll` is in the game folder (`python3 mwc_loader.py status`) |
| Installer says "game path not found" | Pass the path manually: `bash install_maximum.sh --game-path "/path/to/My Winter Car"` |
| Build fails | Install mingw-w64: `sudo apt install mingw-w64` |

---

## Uninstall

```bash
python3 mwc_loader.py disable-loader   # removes DLLs from game folder
rm ~/.config/autostart/maximum-daemon.desktop   # removes autostart entry
```

Then remove this folder and clear the Steam Launch Options.

---

*Maximum MWC Loader — github.com/Myklemax/maximum-mwc-loader*
