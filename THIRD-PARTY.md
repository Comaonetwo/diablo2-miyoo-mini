# Open-source components

The source snapshot contains project-owned files and patches. Build dependencies
are fetched from the exact commits in `dependencies.json`; their license and
copyright notices must be retained with distributions.

- d2decomped — GPL-3.0-or-later, https://github.com/psi29a/d2decomped
- StormLib — MIT, with bundled compression dependencies and their notices,
  https://github.com/ladislav-zezula/StormLib
- SDL — zlib license, https://github.com/libsdl-org/SDL
- OpenAL Soft — LGPL-2.0-or-later, https://github.com/kcat/openal-soft
- CLI11 — BSD-3-Clause, https://github.com/CLIUtils/CLI11

The release package includes a corresponding source archive for all
statically linked dependencies, the engine, and this project's source/patches.
It also includes each component's license files. To rebuild a modified binary,
unpack the project source, restore the dependency folders as specified in the
corresponding-source README, and follow the direct CMake command in that README (the snapshot already has patches applied). Static linkage of
OpenAL Soft permits rebuilding/relinking with a modified library using these
sources. There is no signature lock or installation restriction in this package.

DevilutionX was consulted as a control/layout reference; no code or assets from
DevilutionX were copied. Diablo II, LoD, patch executables and game assets remain
their owners' property; the public release omits MPQ archives, original game executables, patch installers and player saves. The optional Ports thumbnail uses commercial promotional artwork with a separate notice: assets/ports/ARTWORK-NOTICE.txt. That image is excluded from the project code license; no additional artwork license is claimed. In-game game artwork, including the animated logo, is loaded from the user's own files at runtime.

The optional diablo2-audio helper calls the SDL 1.2 ABI provided by the owner's
console firmware. No console SDL or vendor library is distributed. The helper
and its shared-memory protocol are project source under GPL-3.0-or-later.

OpenAL Soft: локальное изменение начального резерва голосов loopback (32 вместо 256, автоматическое расширение сохранено). Точный diff находится в patches/openal.patch; исходники включены в соответствующий архив.

Port Readable is a coverage atlas rasterized from Forum Regular, copyright
(c) 2011 Denis Masharov, under SIL Open Font License 1.1. The original font,
full license, provenance and generator are in assets/readable/ and
scripts/build_readable_font.py; the generated atlas is src/readable_font_data.hpp.
The converted font is named Port Readable. The binary package carries
READABLE-FONT-LICENSE.txt. This font data retains its OFL license.

The console allocator uses the dlmalloc 2.8.6 core included in the pinned SDL
source. Doug Lea released dlmalloc to the public domain under CC0; SDL's
modifications retain their zlib notice in src/vendor/dlmalloc.c. Extraction
provenance and configuration are recorded in src/vendor/provenance.json. The
project's C API adapter is GPL-3.0-or-later. The allocator source is included in
the corresponding source archive and can be rebuilt or replaced.

Compact HUD material in src/hud_art.hpp was sampled from AI-generated illustrations approved by the project owner. It is project UI material, licensed with the project code; see assets/hud/provenance.json. Original game figures, orb fills and item icons are loaded from user MPQs separately.
