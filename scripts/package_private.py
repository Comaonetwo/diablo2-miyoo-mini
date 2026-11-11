#!/usr/bin/env python3
"""Assemble a local owner-only test package; never downloads or publishes assets."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import tarfile
import zipfile

parser=argparse.ArgumentParser()
parser.add_argument("--no-private-archive",action="store_true",help="Prepare source and update inputs without another full asset archive")
args=parser.parse_args()

root = Path(__file__).resolve().parents[1]
version = (root / 'VERSION').read_text().strip()
outputs = root / 'outputs'
pkg = outputs / 'private' / f'diablo2-miyoo-test-{version}'
game = pkg / 'Roms/PORTS/Games/DiabloIIResearch'
data = game / 'data'
owner = json.loads((data / 'OWNER-ASSETS.json').read_text())
for entry in owner['files']:
    path = data / entry['name']
    if path.stat().st_size != entry['bytes'] or hashlib.sha256(path.read_bytes()).hexdigest() != entry['sha256']:
        raise RuntimeError('Owner asset differs from extraction manifest: ' + path.name)

build = root / 'work/build/console'
manifest = {'version': version, 'tested_on_physical_console': False, 'binaries': []}
for src, name in [('d2-desktop', 'diablo2-miyoo'), ('d2-hardware', 'diablo2-hardware')]:
    binary = (build / src).read_bytes()
    if binary[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', binary, 18)[0] != 40:
        raise RuntimeError('Expected ARM32 little-endian ELF')
    flags = struct.unpack_from('<I', binary, 36)[0]
    if not flags & 0x400:
        raise RuntimeError('Expected hard-float ELF ABI')
    phoff = struct.unpack_from('<I', binary, 28)[0]
    entsize, count = struct.unpack_from('<HH', binary, 42)
    if any(struct.unpack_from('<I', binary, phoff + i * entsize)[0] == 3 for i in range(count)):
        raise RuntimeError('Unexpected ELF dynamic interpreter')
    path = game / name; path.write_bytes(binary); path.chmod(0o755)
    manifest['binaries'].append({'name': name, 'bytes': len(binary), 'sha256': hashlib.sha256(binary).hexdigest(), 'elf_flags': hex(flags), 'dynamic_interpreter': False})
helper = build / 'diablo2-audio'
if helper.exists():
    binary = helper.read_bytes()
    if binary[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', binary, 18)[0] != 40:
        raise RuntimeError('Expected ARM32 audio helper')
    flags = struct.unpack_from('<I', binary, 36)[0]
    phoff = struct.unpack_from('<I', binary, 28)[0]
    entsize, count = struct.unpack_from('<HH', binary, 42)
    interpreters=[]
    for i in range(count):
        header = struct.unpack_from('<8I', binary, phoff+i*entsize)
        if header[0] == 3: interpreters.append(binary[header[1]:header[1]+header[4]].rstrip(b'\0').decode())
    if not flags & 0x400 or interpreters != ['/lib/ld-linux-armhf.so.3']:
        raise RuntimeError('Unexpected audio helper ABI/interpreter')
    path = game / 'diablo2-audio'; path.write_bytes(binary); path.chmod(0o755)
    manifest['binaries'].append({'name':path.name,'bytes':len(binary),'sha256':hashlib.sha256(binary).hexdigest(),'elf_flags':hex(flags),'dynamic_interpreter':interpreters[0],'uses_console_sdl_1_2':True})
for name in ('run.sh', 'run-hardware.sh'):
    shutil.copyfile(root / 'packaging' / name, game / name); (game / name).chmod(0o755)
shutil.copyfile(root / 'VERSION', game / 'VERSION')
shutil.copyfile(root / 'packaging/INSTALL.md', pkg / 'INSTALL.md')
for name in (f'PROTOTYPE-{version}-RU.md', f'prototype-{version}-results.json'):
    shutil.copyfile(outputs / name, pkg / name)
for name in ('CONSOLE-TEST-PLAN-RU.md','FEEDBACK-2026-10-27-RU.md','PORT-READINESS-RU.md','GAMEPLAY-GAPS-2026-10-29-RU.md','MECHANICS-TRACKER-RU.md'):
    if (outputs/name).exists(): shutil.copyfile(outputs/name,pkg/name)
qa = outputs / f'test-saves-{version}'
if qa.is_dir(): shutil.copytree(qa, pkg / 'QA-SAVES', dirs_exist_ok=True)
for name in (f'PERFORMANCE-{version}-RU.md', f'performance-{version}-results.json'):
    if (outputs / name).exists(): shutil.copyfile(outputs / name, pkg / name)
if (outputs / 'MEMORY-LIMIT-FIX-RU.md').exists():
    shutil.copyfile(outputs / 'MEMORY-LIMIT-FIX-RU.md', pkg / 'MEMORY-LIMIT-FIX-RU.md')
for title, script, folder in [('Diablo II Screen Test', 'run-hardware.sh', 'Experimental'), ('DIABLO II', 'run.sh', '')]:
    path = pkg / 'Roms/PORTS/Shortcuts' / folder / (title + '.port'); path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f'''#!/bin/sh
GameName="{title}"
GameDir="DiabloIIResearch"
GameExecutable="{script}"
GameDataFile="d2data.mpq"
KillAudioserver=0
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
'''); path.chmod(0o755)

image = pkg / 'Roms/PORTS/Imgs/DIABLO II.png'
image.parent.mkdir(parents=True,exist_ok=True)
shutil.copyfile(root/'assets/ports/DIABLO II.png',image)

project = []
for name in ('CMakeLists.txt', 'README.md', 'THIRD-PARTY.md', 'LICENSE', 'VERSION', 'dependencies.json', '.gitignore'):
    project.append(root / name)
for name in ('src', 'tests', 'scripts', 'patches', 'packaging', 'assets'):
    project.extend(p for p in (root / name).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
source_zip = outputs / f'diablo2-miyoo-research-{version}-source.zip'
with zipfile.ZipFile(source_zip, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for p in sorted(project): z.write(p, f'diablo2-miyoo-source-{version}/' + p.relative_to(root).as_posix())
print('Asset-free source:', source_zip.name, flush=True)

sources = pkg / 'Sources'; sources.mkdir(exist_ok=True)
source_readme = root / 'work/CORRESPONDING-SOURCE-README.txt'
source_readme.write_text('''Corresponding source for the private Diablo II Miyoo prototype.
The work/deps/* and work/sources/d2decomped folders contain the exact patched
sources used for the binary. Git metadata, game MPQs, patch executables and saves
are absent. dependencies.json records upstream revisions. Project patches are
already applied in this snapshot: do not apply them a second time.

Use CMake 3.28+ and Zig 0.17.0 on a build computer. From the extracted project:
export D2_ZIG=/absolute/path/to/zig
cmake -S . -B work/build/console -DCMAKE_TOOLCHAIN_FILE=scripts/arm32.cmake -DCMAKE_BUILD_TYPE=Release -DD2_BUILD_CONSOLE=ON -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG -g0" -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG -g0" -DCMAKE_EXE_LINKER_FLAGS="-static -Wl,--strip-all"
cmake --build work/build/console --parallel 4
# Diagnostic console builds retain function symbols for PC/LR decoding:
python3 scripts/retain_console_symbols.py

Use work/build/console/d2-desktop as diablo2-miyoo and d2-hardware as
diablo2-hardware in the private package. Modified versions can be rebuilt and
relinked, including OpenAL Soft. No signature mechanism restricts replacements.
This archive includes dependency build files, headers, bundled compression
sources and their licensing notices. Toolchains themselves are not bundled.
''')
corresponding = sources / f'diablo2-miyoo-{version}-corresponding-source.tar.gz'
deps = json.loads((root / 'dependencies.json').read_text())
with tarfile.open(corresponding, 'w:gz', compresslevel=3) as tar:
    for p in sorted(project): tar.add(p, arcname=f'diablo2-miyoo-{version}/' + p.relative_to(root).as_posix(), recursive=False)
    tar.add(source_readme, arcname=f'diablo2-miyoo-{version}/CORRESPONDING-SOURCE-README.txt')
    for dep in deps.values():
        folder = root / dep['path']
        for p in sorted(folder.rglob('*')):
            if '.git' in p.parts or not p.is_file(): continue
            tar.add(p, arcname=f'diablo2-miyoo-{version}/' + p.relative_to(root).as_posix(), recursive=False)
shutil.copyfile(root / 'LICENSE', game / 'COPYING')
shutil.copyfile(root / 'THIRD-PARTY.md', game / 'THIRD-PARTY.md')
shutil.copyfile(root / 'assets/readable/OFL.txt', game / 'READABLE-FONT-LICENSE.txt')
(pkg / 'BUILD-MANIFEST.json').write_text(json.dumps(manifest, indent=2) + '\n')
print('Corresponding source:', corresponding.stat().st_size, 'bytes', flush=True)

if args.no_private_archive:
    print('Full private archive skipped; staged files ready for asset-free update',flush=True)
    raise SystemExit(0)

archive = outputs / 'private' / (pkg.name + '-PRIVATE.zip')
with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=1) as z:
    for p in sorted(pkg.rglob('*')):
        if p.is_file():
            method = zipfile.ZIP_STORED if p.suffix.lower() in ('.mpq', '.gz') else zipfile.ZIP_DEFLATED
            z.write(p, (Path(pkg.name) / p.relative_to(pkg)).as_posix(), compress_type=method)
with zipfile.ZipFile(archive) as z:
    bad = z.testzip()
    if bad: raise RuntimeError('Archive failed CRC: ' + bad)
print('PRIVATE test package:', archive.name, archive.stat().st_size, 'bytes; CRC verified', flush=True)
