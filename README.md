# Maximus — MWC Loader

A mod loader  hook framework for My Winter Car on Linux Steam, + Proton



# How it works

Steam launches game
launch_maximus.sh (Steam Launch Options wrapper)
game loads winmm.dll (our proxy, from game folder)
winmm_maximus.c: DllMain fires, spawns thread
loads MaximusHost.dll
calls MaximusEntry() ← put mod code here
writes maximus.log
maximus_daemon.sh detects log
opens ✦ MAXIMUS LOG ✦ terminal




# Requirements

Linux with Steam (native or snap)
Proton (auto-detected from Steam library)
Python 3
mingw-w64 cross compiler

bash
sudo apt install mingw-w64       # Ubuntu / Debian / Mint
sudo pacman -S mingw-w64-gcc     # Arch





## 2. Install

bash
./install_maximus.sh


Builds the DLLs and copies them into your My Winter Car game folder automatically.

# 3. Set Steam Launch Options

Right-click **My Winter Car** in Steam → **Properties** → **Launch Options**:


WINEDLLOVERRIDES=winmm=n,b "/path/to/maximus-mwc-loader/launch_maximus.sh" %command%

# 4. Start the log daemon (once per login)

bash
bash maximus_daemon.sh &


Runs automatically at every login after install. To start now without logging out, run the above in any host terminal (`Ctrl+Alt+T`).

# 5. Play

Launch the game from Steam. A green terminal titled **✦ MAXIMUS LOG ✦** appears with live hook output.



# Commands

bash
python3 mwc_loader.py detect            # Show detected game path
python3 mwc_loader.py build-native      # Build DLLs with mingw-w64
python3 mwc_loader.py install-maximus   # Copy DLLs into game folder
python3 mwc_loader.py status            # Show loader state
python3 mwc_loader.py enable-loader     # Re-enable after disabling
python3 mwc_loader.py disable-loader    # Disable without uninstalling
python3 mwc_loader.py watch-log         # Open live log viewer manually


All commands accept `--game-path "/path/to/My Winter Car"` to override auto-detection.






## Adding mod code

Edit `native/maximus_host.c` → `MaximusEntry()`:

c
void MaximusEntry(void) {
    log_msg("[maximus] MaximusHost loaded");
    log_msg("[mymod] MyMod v1.0 loaded");  // add your code here
}


Then rebuild and reinstall:

bash
./install_maximus.sh




## Troubleshooting

 Problem | Fix |
 Game crashes on launch | Ensure `WINEDLLOVERRIDES=winmm=n,b` is in Launch Options 
 `maximus.log` not created | Delete any leftover `version.dll` / `winhttp.dll` from game folder 
 Log terminal doesn't open | Run `bash maximus_daemon.sh &` in a host terminal (not VS Code) 
 Build fails | Install `mingw-w64`: `sudo apt install mingw-w64` 





# License

Skibdi toilet
Max AM
