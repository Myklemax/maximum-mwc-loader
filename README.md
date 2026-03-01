# Maximum — MWC Loader

Mod loader for **My Winter Car** on Linux + Steam (Proton). Ships prebuilt DLLs; no build step required.

---

## Requirements

- Linux (Steam native or Snap)
- Proton (auto-detected)
- Python 3 — `python3 --version`
- gnome-terminal — `gnome-terminal --version`

If something is missing, run the helper once:

```bash
bash setup_requirements.sh
```

---

## Quick install

1) Place this folder anywhere (e.g., `~/maximum-mwc-loader`).
2) In a terminal in the folder, run:

```bash
bash install_maximum.sh
```

It copies the packaged `winmm.dll`, `MaximumHost.dll`, and `mods/MSCLoader.dll` into the game, creates `mods/`, installs the log daemon, and prints the Steam Launch Options.

3) In Steam → My Winter Car → Properties → Launch Options, paste the printed line (your path will be filled in):

```
WINEDLLOVERRIDES=winmm=n,b "/path/to/maximum-mwc-loader/launch_maximum.sh" %command%
```

Then launch the game. A green terminal titled **✦ MAXIMUM LOG ✦** opens with live output.

---

## Installing mods

Put mod DLLs into the game `mods/` folder:

```
My Winter Car/
  winmm.dll
  MaximumHost.dll
  mods/
    MSCLoader.dll        ← managed loader shim (installed automatically)
    MyNativeMod.dll      ← native plugin (exports MaximumModInit)
    MyManagedMod.dll     ← managed mod (e.g., Second Kettle)
```

- Native mods: loaded directly by MaximumHost; `MaximumModInit` is called if present.
- Managed mods: loaded by MSCLoader (included). Drop the managed mod DLL next to `MSCLoader.dll`.

No reinstall is needed after adding mods—just relaunch the game.

---

## Useful commands

```bash
python3 mwc_loader.py status           # Show install state and game path
python3 mwc_loader.py enable-loader    # Re-enable after disabling
python3 mwc_loader.py disable-loader   # Temporarily disable (renames DLLs)
python3 mwc_loader.py watch-log        # Open the live log viewer manually
```

All commands accept `--game-path "/path/to/My Winter Car"` if auto-detect fails.

---

## Troubleshooting

- Game crashes or no hooks: ensure `WINEDLLOVERRIDES=winmm=n,b` is in Launch Options.
- No `maximum.log`: remove any stray `version.dll` / `winhttp.dll` in the game folder.
- Log terminal missing: run `bash maximum_daemon.sh &` in a normal terminal.
- Installer cannot find the game: `bash install_maximum.sh --game-path "/path/to/My Winter Car"`.

---

## Uninstall

```bash
python3 mwc_loader.py disable-loader   # removes the DLLs from the game folder
rm ~/.config/autostart/maximum-daemon.desktop
```

Clear the Steam Launch Options and delete this folder if desired.
