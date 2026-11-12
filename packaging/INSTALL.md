# Diablo II for Miyoo Mini — installation 0.2.87

This port supports classic Diablo II: Lord of Destruction on Onion, single-player acts I–IV. Act V, multiplayer and cinematics are disabled. Diablo II: Resurrected files are not supported.

## 1. Prepare the card

Shut down the handheld and connect its SD card to your computer. Enable Ports Collection in Onion under Apps → Package Manager → Verified if necessary.

If updating an existing installation, first back up its entire `Roms/PORTS/Games/DiabloII/state` folder. Keep `state` and `data` when replacing port files. The release ZIP contains no player saves or settings.

## 2. Copy the port

Extract `Diablo-II-Miyoo-0.2.87.zip` on your computer. Copy these three items from the extracted `Roms/PORTS` directory:

1. `Games/DiabloII` into the card's `Roms/PORTS/Games/` directory.
2. `Shortcuts/DIABLO II.port` into the card's `Roms/PORTS/Shortcuts/` directory.
3. `Imgs/DIABLO II.png` into the card's `Roms/PORTS/Imgs/` directory.

Create `Shortcuts` or `Imgs` if missing. Replace this port's matching files when prompted, preserving user data. Do not replace or delete the entire existing `Roms` or `PORTS` directory: it contains your other games.

The correct launcher path is `SDCARD/Roms/PORTS/Games/DiabloII/run.sh`. Avoid an extra `Roms/Roms` directory or leaving the whole release folder between the card root and `Roms`.

The `Sources` folder is for developers and does not need to go on the card.

## 3. Add your own game files

Copy the following directly into `SDCARD/Roms/PORTS/Games/DiabloII/data/`.

Required to start:

- `d2data.mpq`
- `d2char.mpq`
- `d2exp.mpq`
- `LODPatch_114d.exe`

For sound effects, speech and music:

- `d2sfx.mpq`
- `d2speech.mpq`
- `d2xtalk.mpq`
- `d2music.mpq`
- `d2xmusic.mpq`

Use MPQs from your own classic Diablo II + Lord of Destruction installation or discs. The LoD 1.14d patch installer `LODPatch_114d.exe` is obtained separately and may not be present in an installed game's folder. The port reads its data; do not run this EXE on the handheld. No game MPQs or patch installer are supplied with the port.

Do not create another nested `data` folder. English classic resources were tested; other language/data combinations have not all been verified. Video MPQs are not needed.

## 4. Start and check saving

Safely eject the card, put it into the powered-off handheld and start Ports → DIABLO II. If the entry is missing, run `~Import ports` in the Ports list and refresh the game list.

Create a test character and enter town. Press MENU, choose Save and return to menu, then load the character again. This checks installation and saving without requiring a long gameplay session.

MENU → Options changes the display, font, sound, world scale and belt preferences. MENU → Controls shows the physical button guide.

## Saves and updates

Saves and settings are stored in `Roms/PORTS/Games/DiabloII/state/`. Back up the entire folder before removing or updating the port, including companion files alongside `.d2s` saves. Keep the original backup while testing an update.

To restore a backup, exit the game, shut down the handheld, connect the card to your computer and restore `state` into the `DiabloII` folder. Do not overwrite newer important saves with an older backup.

## Physical controls

- D-pad: movement and selection in panels.
- B: attack / talk / confirm. A: back / skill picker in the field.
- Y: loot, portals, entrances and selected items; buy/sell in shops.
- X: right skill. MENU: port menu.
- L2: character stats. R2: inventory.
- Tap SELECT: map. Hold SELECT for one second: cursor mode, moved with D-pad.
- SELECT+Up: skills. SELECT+Down: quests. SELECT+Left: expanded belt. SELECT+Right: mercenary panel.
- L/R: health/mana or sequential belt slots, according to Options. SELECT+L/R: belt slots 3/4.
- In inventory: SELECT+Y moves an item to the belt; SELECT+X drops it; SELECT+START arranges inventory.
- In the field: SELECT+Y swaps weapon sets; SELECT+X toggles run/walk.
- In shops: L/R changes tabs; SELECT+Y fills consumables.

Use short SELECT combinations before cursor mode activates. SELECT+L2/R2 remain reserved for Onion brightness.

## Troubleshooting

Missing from Ports: check the launcher path, enable Ports Collection and run `~Import ports`.

Missing game files: the launcher lists missing mandatory files. Copy them into the `data` folder above. For startup failures, inspect `Roms/PORTS/Games/DiabloII/diablo2.log`.

Missing music or speech: check the five audio MPQs and Sound/SFX/Music settings in MENU → Options.

Missing characters: check `DiabloII/state/save/` and restore the complete backed-up `state` folder.

Gameplay issues: most mechanics are implemented, but not everything has been tested and issues may occur. Report the port version, class, act/location, action before the issue and `diablo2.log`. Heavy Act IV scenes can have frame drops or loading pauses. See README for the supported scope and known limitations.

Onion's official Ports layout/import documentation: https://onionui.github.io/docs/ports
