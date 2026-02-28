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
| `mods/` | Drop your `.dll` plugin mods here |

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

## Step 1 — Copy this folder somewhere permanent

Move or copy `maximum-mwc-loader/` to a permanent location.
**Do not delete it after installing** — Steam needs to reach `launch_maximum.sh` every time you play.

Good locations:
```
~/maximum-mwc-loader/
~/Games/maximum-mwc-loader/
```

Example:
```bash
cp -r maximum-mwc-loader ~/maximum-mwc-loader
cd ~/maximum-mwc-loader
```

---

## Step 2 — Run the installer

Open a terminal inside the folder and run:

```bash
bash install_maximum.sh
```

This will:
1. Auto-detect your My Winter Car game folder
2. Copy `winmm.dll` and `MaximumHost.dll` into the game folder
3. Create the `mods/` folder inside the game directory
4. Install the log daemon to start automatically at login
5. Print your Steam Launch Options

---

## Step 3 — Set Steam Launch Options

1. Open **Steam**
2. Right-click **My Winter Car** → **Properties** → **Launch Options**
3. Paste the line printed by the installer. It looks like:

```
WINEDLLOVERRIDES=winmm=n,b "/home/YOUR_NAME/maximum-mwc-loader/launch_maximum.sh" %command%
```

> Replace the path with wherever **you** put the folder in Step 1.

---

## Step 4 — Play

Launch **My Winter Car** from Steam as normal.

A green terminal titled **✦ MAXIMUM LOG ✦** will pop up automatically showing live output from the mod loader.

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
