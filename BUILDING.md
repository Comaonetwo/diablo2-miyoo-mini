# Rebuilding 0.2.92

Players should use the prebuilt release ZIP. This file is for developers.

## Complete corresponding-source archive

The release includes `Sources/diablo2-miyoo-0.2.92-corresponding-source.tar.gz`. Extract it on a build computer. It includes this project, the exact patched engine and dependency sources, their license notices and the patches; it omits Git metadata, tools, game MPQs, patch executables and player saves. Patches are already applied in this archive: do not apply them again.

Tools used: **CMake 3.28+**, **Zig 0.17.0**, a make-compatible build tool. Python 3 is needed for the optional symbol-retention and packaging scripts. Game resources are not needed to compile the console executable.

From the extracted source root:

```sh
export D2_ZIG=/absolute/path/to/zig
cmake -S . -B work/build/console \
  -DCMAKE_TOOLCHAIN_FILE=scripts/arm32.cmake \
  -DCMAKE_BUILD_TYPE=Release -DD2_BUILD_CONSOLE=ON \
  -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG -g0" \
  -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -g0" \
  -DCMAKE_EXE_LINKER_FLAGS="-static -Wl,--strip-all"
cmake --build work/build/console --target d2-desktop d2-audio-helper --parallel 4
python3 scripts/retain_console_symbols.py
```

Install `work/build/console/d2-desktop` as `Roms/PORTS/Games/DiabloII/diablo2-miyoo` and `work/build/console/diablo2-audio` beside it. Keep `packaging/run.sh` as `run.sh`. Executables and launcher need execute permission on filesystems that enforce it.

The main executable is static ARM32 hard-float musl, Cortex-A7. The audio helper uses the SDL 1.2 ABI supplied by Onion; no firmware SDL/vendor library is distributed. All sources needed to modify and relink the engine and OpenAL Soft are included. There is no signature lock or hardware installation restriction imposed by this port.

## Repository checkout

A source-only checkout can fetch the pinned public dependencies and apply project patches:

```sh
export D2_ZIG=/absolute/path/to/zig
python3 scripts/build.py console --prepare --cmake /path/to/cmake
python3 scripts/retain_console_symbols.py
```

This route requires Git and network access. Dependency commits are fixed in `dependencies.json`; existing checkouts with a different revision are preserved and rejected rather than reset automatically.

## Packaging and validation

`python3 scripts/package_release.py --source-dir /path/to/clean-export --build-dir /path/to/clean-export/work/build/console` creates a clean release folder and ZIP under `outputs/releases/0.2.92/`, including corresponding source. It never reads or installs to an SD card and never packages MPQs, original executables, patch installers, player saves or runtime logs. The separately attributed optional Ports artwork is included.

Gameplay validation for 0.2.81 passed 41 CTests and nine integration stages. Version 0.2.84 fixes fractional Firestorm damage, skill cast/impact sounds and controller selection of Assassin. Version 0.2.85 adds retained fractional breath damage (Inferno, Arctic Blast and Inferno Sentry), Arctic Blast chill and missing action/breath sound cues. Targeted validation uses real-data combat tests on native sanitizers and ARM and isolated gameplay audio output. The 0.2.84 console fixes were accepted by the owner. Version 0.2.86 fixes multi-tick camera history and adds an opt-in experimental 50 FPS display mode; the simulation stays at 25 Hz. Version 0.2.87 adds render-only actor/projectile interpolation and stronger roof cutouts; Experimental defaults on with a retained toggle. It fixes zero-durability weapons and missing monster hit art, adds compact corner mercenary/map indicators, 100 ms menu fades and optional Quality lighting. Targeted real-data native/ARM tests and isolated ASan gameplay cover this package. Changes to simulation, saves or input require appropriate gameplay tests; unrelated historical scenarios need not be repeated for packaging-only edits.

Create that export before building with `python3 scripts/package_release.py --export-source /path/to/empty/clean-export`. Build inside it using the CMake commands above; package from its own build directory. The exporter freezes source hashes so later source edits cannot silently change the corresponding-source archive.

Version 0.2.88 fixes monster pose weapon variants, adds original NPC inventory art and active mercenary portraits, restores repair/greeting/weapon and skill sound events, and adds optional combat loot dimming with actor coverage. Ice fragment directions use a bounded cache; exact source-frame hashes remain unchanged. Native, sanitizer and ARM checks target these changes rather than replaying unrelated campaign scenarios.

Version 0.2.90 adds a title-screen version label, raises the four Readable base-stat numbers, and extends direction caching to selected electrical/fire/ice projectiles and Teeth. The shared effect cache admits sprites using a conservative retained-frame reservation within 5 MiB (decoder scratch is separate), and case-insensitive resource aliases share entries. Original title-button labels use increased contrast without changing their font or size. Heavy mixed-effect scenes, repeated Act IV transitions, pixel/reservation tests and an ARM check target this change.

Version 0.2.91 fixes demand loading for fixed walking links missing from the outdoor placement graph (including the monastery). Explicit direct neighbours are requested without enabling broad area/warp prefetch. Open-door collision is restored for adjacent levels after border-room regeneration, so the return trip cannot acquire an invisible door barrier. Regression checks traverse the monastery and Act IV seams in both directions across three map seeds, with a bounded ARM run.

Version 0.2.92 adds atomic belt-to-inventory transfer with physical X, applies Objects.txt mode frame counts/speeds to object rendering (including closed doors), and restores confirmed original waypoint ignition/container sound mappings. Focused native, sanitizer and ARM checks cover this package.
