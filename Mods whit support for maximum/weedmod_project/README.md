# Weed Mod Project (Maximum MWC Loader)

This is a starter external DLL mod project that adds a simple grow/sell gameplay loop.

## Loop

1. Plant a batch
2. Water crop
3. Advance growth stage
4. Harvest buds
5. Sell inventory for mk

The mod uses trigger files (same style as your other Maximum mods).

## Files

- `weedmod.c` — DLL source (`MaximumModInit` export)
- `build_weedmod.sh` — build script for `weedmod.dll`
- `weedmod.ini.example` — balance and trigger config template

## Build

From this folder:

```bash
chmod +x build_weedmod.sh
./build_weedmod.sh
```

Output: `weedmod.dll`

## Install

1. Copy `weedmod.dll` to your game mods folder used by Maximum loader.
2. Copy `weedmod.ini.example` as `weedmod.ini` next to loader/game root (same place where trigger files are read).
3. Launch game through Maximum loader.

## Trigger files

Create empty files in game root:

- `weed_plant.trigger`
- `weed_water.trigger`
- `weed_advance.trigger`
- `weed_harvest.trigger`
- `weed_sell.trigger`

Each file is consumed (deleted) after detected.

## Runtime outputs

- `maximum.log` — action logs
- `weed_state.ini` — persisted state
- `weed_status.txt` — quick status snapshot

## Notes

- Growth only advances if the crop was watered for that stage.
- Harvest requires max stage reached.
- Selling converts all inventory buds to mk using config price.
