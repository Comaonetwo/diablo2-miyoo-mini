# Rebuilding 0.2.85

Players should use the prebuilt release ZIP. This file is for developers.

## Complete corresponding-source archive

The release includes `Sources/diablo2-miyoo-0.2.85-corresponding-source.tar.gz`. Extract it on a build computer. It includes this project, the exact patched engine and dependency sources, their license notices and the patches; it omits Git metadata, tools, game MPQs, patch executables and player saves. Patches are already applied in this archive: do not apply them again.

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

`python3 scripts/package_release.py --source-dir /path/to/clean-export --build-dir /path/to/clean-export/work/build/console` creates a clean release folder and ZIP under `outputs/releases/0.2.85/`, including corresponding source. It never reads or installs to an SD card and never packages MPQs, original executables, patch installers, player saves or runtime logs. The separately attributed optional Ports artwork is included.

Gameplay validation for 0.2.81 passed 41 CTests and nine integration stages. Version 0.2.84 fixes fractional Firestorm damage, skill cast/impact sounds and controller selection of Assassin. Version 0.2.85 adds retained fractional breath damage (Inferno, Arctic Blast and Inferno Sentry), Arctic Blast chill and missing action/breath sound cues. Targeted validation uses real-data combat tests on native sanitizers and ARM and isolated gameplay audio output. The 0.2.84 console fixes were accepted by the owner. Changes to simulation, saves or input require appropriate gameplay tests; unrelated historical scenarios need not be repeated for packaging-only edits.

Create that export before building with `python3 scripts/package_release.py --export-source /path/to/empty/clean-export`. Build inside it using the CMake commands above; package from its own build directory. The exporter freezes source hashes so later source edits cannot silently change the corresponding-source archive.
