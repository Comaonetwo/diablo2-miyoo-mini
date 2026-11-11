# Diablo II for Miyoo Mini — 0.2.83

A community port of classic **Diablo II: Lord of Destruction** for Miyoo Mini / Mini Plus running Onion. Single-player acts **I–IV**; Act V, multiplayer and cinematics are disabled in this release.

[Installation guide](packaging/INSTALL.md) · [Build from source](BUILDING.md) · [Third-party notices](THIRD-PARTY.md)

## Install — no compiling required

Download the release named **Diablo-II-Miyoo-0.2.83.zip**, not GitHub's “Source code” download. Extract it on your computer.

1. Enable **Ports Collection** in Onion: Apps → Package Manager → Verified.
2. Copy `Roms/PORTS/Games/DiabloII` to the card's `Roms/PORTS/Games/` folder; copy `DIABLO II.port` into `Roms/PORTS/Shortcuts/` and `DIABLO II.png` into `Roms/PORTS/Imgs/`. Keep all other games. Do not replace the whole existing `Roms` folder.
3. Put your own game files directly in `Roms/PORTS/Games/DiabloII/data/` as listed below.
4. Safely eject the card, insert it into the powered-off Miyoo, and start **Ports → DIABLO II**. If missing from the list, run **~Import ports**.

These folders follow the [official Onion Ports layout](https://onionui.github.io/docs/ports). The `Sources` folder contains rebuildable source and does not need to go on the card.

Tested setup: the owner's Miyoo Mini with **Onion v4.3.1-1**. Other models/firmware combinations have not all been physically tested; this is an ARM32 Onion port, not a general handheld build.

## Required owner-supplied files

Use the classic game plus **Lord of Destruction**, not Diablo II: Resurrected. No game download, MPQ, patch installer or player save is bundled.

Required to start:

```text
d2data.mpq
d2char.mpq
d2exp.mpq
LODPatch_114d.exe
```

For sound, speech and music:

```text
d2sfx.mpq
d2speech.mpq
d2xtalk.mpq
d2music.mpq
d2xmusic.mpq
```

Copy MPQs from your own classic installation or discs. `LODPatch_114d.exe` is the LoD 1.14d patch installer, read as a data source; it is not executed on the handheld. English classic resources plus this patch were tested. Other language/data combinations are not certified. Video MPQs are not needed for this release.

## Saves and updates

The port stores saves and settings in `Roms/PORTS/Games/DiabloII/state/`. Back up that entire folder before removing or updating the port. Preserve both `state` and `data` when updating. The release ZIP contains no `state` folder and never supplies somebody else's characters.

## Physical Miyoo controls

| Button | Action |
|---|---|
| D-pad | Move / select in panels |
| B | Attack / talk / confirm |
| A | Back / skill picker in the field |
| Y | Loot / portal / entrance / use selected item; buy/sell in shop |
| X | Right skill |
| MENU | Port menu, options, controls, save and return to menu |
| L2 / R2 | Character stats / inventory |
| Tap SELECT / hold 1 second | Map / cursor mode with D-pad |
| SELECT + Up / Down | Skill tree / quests |
| SELECT + Left / Right | Expanded belt / mercenary panel |
| L / R | Health/mana, or sequential belt slots according to Options |
| SELECT + L / R | Belt slots 3 / 4 |
| SELECT + Y, in inventory | Move selected item to belt |
| SELECT + X, in inventory | Drop selected item |
| SELECT + START, in inventory | Arrange inventory |
| SELECT + Y, in the field | Swap weapon sets |
| SELECT + X, in the field | Run/walk |
| L / R, in shop | Previous/next shop tab |
| SELECT + Y, in shop | Fill consumables |

SELECT combinations are short presses before cursor mode activates. SELECT+L2/R2 are reserved for Onion brightness. Full controls are available from MENU → Controls.

## Scope and known limits

The release retains the mechanics of the console-tested 0.2.81 candidate. Compact HUD, Readable/Original fonts, smoothing, controller panels, cube, equipment, mercenaries, saves, difficulties and original procedural levels are present for the supported scope.

- Heavy scenes, especially Act IV, can still have frame drops and loading pauses. No fixed console FPS is promised.
- Complex damage displays show the primary hit/projectile, not the total over every projectile and target. Some original formatting/tooltip details differ.
- Item self-repair/refill timer phase restarts after reload; Rabies uses local propagation pulses; some RNG, rounding and trajectory details differ from the original.
- No exhaustive full-campaign playthrough or every rare skill/item combination is claimed. Automated tests and the owner's console testing provide bounded evidence.
- Act V, multiplayer and cinematics are excluded, not hidden completed features.

If startup fails, check the required files and `Roms/PORTS/Games/DiabloII/diablo2.log`. For a gameplay issue, provide the version, character class, act/location, action before the problem and that log. There is no automatic crash upload.

## Source and artwork

Code is GPL-3.0-or-later with dependency notices retained. Releases include complete corresponding source for the statically linked components; see [BUILDING.md](BUILDING.md). The Forum font retains its SIL OFL license.

The optional Ports thumbnail is Diablo II promotional box artwork and is **excluded from the code license**; see [its separate notice](assets/ports/ARTWORK-NOTICE.txt). Game artwork and trademarks remain with their owners. The animated menu logo is read from the user's MPQs; no static logo bitmap is embedded in this build. No endorsement or additional commercial-art license is claimed.

Based on the pinned `psi29a/d2decomped` engine with substantial handheld/gameplay work. Exact dependency revisions are recorded in `dependencies.json`.
