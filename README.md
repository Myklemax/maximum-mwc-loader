# Maximum — MWC Loader

A mod loader  hook framework for My Winter Car on Linux Steam, + Proton



# How it works

Steam launches game
launch_maximum.sh (Steam Launch Options wrapper)
game loads winmm.dll (our proxy, from game folder)
winmm_maximum.c: DllMain fires, spawns thread
loads MaximumHost.dll
calls MaximumEntry() ← put mod code here
writes maximum.log
maximum_daemon.sh detects log
opens ✦ MAXIMUM LOG ✦ terminal




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
./install_maximum.sh


Builds the DLLs and copies them into your My Winter Car game folder automatically.

# 3. Set Steam Launch Options

Right-click **My Winter Car** in Steam → **Properties** → **Launch Options**:


WINEDLLOVERRIDES=winmm=n,b "/path/to/maximus-mwc-loader/launch_maximum.sh" %command%

# 4. Start the log daemon (once per login)

bash
bash maximum_daemon.sh &


Runs automatically at every login after install. To start now without logging out, run the above in any host terminal (`Ctrl+Alt+T`).

# 5. Play

Launch the game from Steam. A green terminal titled **✦ MAXIMUM LOG ✦** appears with live hook output.



# Commands

bash
python3 mwc_loader.py detect            # Show detected game path
python3 mwc_loader.py build-native      # Build DLLs with mingw-w64
python3 mwc_loader.py install-maximum   # Copy DLLs into game folder
python3 mwc_loader.py status            # Show loader state
python3 mwc_loader.py enable-loader     # Re-enable after disabling
python3 mwc_loader.py disable-loader    # Disable without uninstalling
python3 mwc_loader.py watch-log         # Open live log viewer manually


All commands accept `--game-path "/path/to/My Winter Car"` to override auto-detection.






## Mods folder

After install, a `mods/` folder is created inside the game directory:

```
My Winter Car/
  winmm.dll          ← Maximum proxy
  MaximumHost.dll    ← Maximum host
  mods/
    MyMod.dll        ← drop your plugin here
    AnotherMod.dll
```

Drop any `.dll` plugin into `mods/`. On every game launch, Maximum loader scans the folder and loads each DLL automatically. If the DLL exports a `MaximumModInit` function it will be called, then the result is written to `maximum.log`:

```
[mods] loaded: MyMod.dll
[mods] loaded (no MaximumModInit): SomeDll.dll
[mods] FAILED to load: BrokenMod.dll
```

### Writing a plugin

A minimal plugin only needs one exported function:

```c
// mymod.c  —  compile with mingw-w64
#include <windows.h>

__declspec(dllexport) void MaximumModInit(void) {
    // your mod code runs here at game start
    OutputDebugStringA("[mymod] hello from MyMod!\n");
}
```

```bash
x86_64-w64-mingw32-gcc -shared -o MyMod.dll mymod.c
# copy MyMod.dll into  "My Winter Car/mods/"
```

## Adding core code

To modify the host itself, edit `native/maximum_host.c` → `MaximumEntry()`, then rebuild:

bash
./install_maximum.sh




## Troubleshooting

 Problem | Fix |
 Game crashes on launch | Ensure `WINEDLLOVERRIDES=winmm=n,b` is in Launch Options 
 `maximum.log` not created | Delete any leftover `version.dll` / `winhttp.dll` from game folder 
 Log terminal doesn't open | Run `bash maximum_daemon.sh &` in a host terminal (not VS Code) 
 Build fails | Install `mingw-w64`: `sudo apt install mingw-w64` 





# License

Skibdi toilet
Max AM
