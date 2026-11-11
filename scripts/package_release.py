#!/usr/bin/env python3
"""Create a player release and corresponding source. Never reads an SD card or game data."""
import argparse,hashlib,json,shutil,struct,tarfile,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DENIED={'.mpq','.exe','.d2s','.d2x','.qa','.skills','.log','.pem','.p12'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def allowed(name):
 p=Path(name)
 return not p.is_absolute() and '..' not in p.parts and '.git' not in p.parts and p.suffix.lower() not in DENIED and p.as_posix()!='src/menu_art.hpp'
def inputs():
 names=['CMakeLists.txt','README.md','BUILDING.md','LICENSE','THIRD-PARTY.md','VERSION','dependencies.json','.gitignore']
 for directory in ['src','tests','scripts','patches','packaging','assets']:
  names += [p.relative_to(ROOT).as_posix() for p in (ROOT/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts]
 for dep in json.loads((ROOT/'dependencies.json').read_text()).values():
  names += [p.relative_to(ROOT).as_posix() for p in (ROOT/dep['path']).rglob('*') if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts]
 return sorted(set(names))
def export_source(dest):
 assert not dest.exists() or not any(dest.iterdir()),'Choose an empty source export directory'
 dest.mkdir(parents=True,exist_ok=True)
 names=inputs()
 for n in names:
  assert allowed(n),n
  p=dest/n;p.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/n,p)
 (dest/'CORRESPONDING-SOURCE-README.txt').write_text('Complete patched source. Start with BUILDING.md. Patches already applied. Game data, SDK/toolchain and Git metadata are not included.\n')
 names.append('CORRESPONDING-SOURCE-README.txt')
 manifest={n:sha(dest/n) for n in sorted(names)}
 (dest/'SOURCE-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print('Clean corresponding source exported:',len(names),'files',flush=True)
def elf(path,static):
 b=path.read_bytes();assert b[:6]==b'\x7fELF\x01\x01' and struct.unpack_from('<H',b,18)[0]==40,path
 assert struct.unpack_from('<I',b,36)[0]&0x400,'Expected hard-float ABI'
 phoff=struct.unpack_from('<I',b,28)[0];entsize,count=struct.unpack_from('<HH',b,42);interpreters=[]
 for i in range(count):
  row=struct.unpack_from('<8I',b,phoff+i*entsize)
  if row[0]==3:interpreters.append(b[row[1]:row[1]+row[4]].rstrip(b'\0').decode())
 assert interpreters==([] if static else ['/lib/ld-linux-armhf.so.3']),(path,interpreters)
def package(build,source):
 version=(source/'VERSION').read_text().strip();assert version==(ROOT/'VERSION').read_text().strip()
 source_manifest=json.loads((source/'SOURCE-MANIFEST.json').read_text())
 for n,h in source_manifest.items():assert allowed(n) and sha(source/n)==h,('source modified since export',n)
 folder=ROOT/'outputs/releases'/version;folder.mkdir(parents=True,exist_ok=True);stage=folder/f'Diablo-II-Miyoo-{version}'
 assert not stage.exists(),'Release staging already exists; preserve it or choose a new version'
 game=stage/'Roms/PORTS/Games/DiabloII';(game/'data').mkdir(parents=True)
 for binary,public,static in [('d2-desktop','diablo2-miyoo',True),('diablo2-audio','diablo2-audio',False)]:
  elf(build/binary,static);shutil.copy2(build/binary,game/public);(game/public).chmod(0o755)
 for src,dst in [('packaging/run.sh','run.sh'),('VERSION','VERSION'),('LICENSE','COPYING'),('THIRD-PARTY.md','THIRD-PARTY.md'),('assets/readable/OFL.txt','READABLE-FONT-LICENSE.txt'),('assets/ports/ARTWORK-NOTICE.txt','ARTWORK-NOTICE.txt'),('packaging/INSTALL.md','INSTALL.txt')]:shutil.copy2(source/src,game/dst)
 (game/'run.sh').chmod(0o755)
 (game/'data/PUT-GAME-FILES-HERE.txt').write_text('Copy your own classic Diablo II LoD MPQs and LODPatch_114d.exe here. Read INSTALL.txt one folder above. No nested data folder.\n')
 (game/'_required_files.txt').write_text('In data/: d2data.mpq, d2char.mpq, d2exp.mpq, LODPatch_114d.exe\nFor audio: d2sfx.mpq, d2speech.mpq, d2xtalk.mpq, d2music.mpq, d2xmusic.mpq\nOriginal game files are not supplied.\n')
 short=stage/'Roms/PORTS/Shortcuts/DIABLO II.port';short.parent.mkdir(parents=True)
 short.write_text('''#!/bin/sh
GameName="DIABLO II"
GameDir="DiabloII"
GameExecutable="run.sh"
GameDataFile="run.sh"
KillAudioserver=0
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
''');short.chmod(0o755)
 icon=stage/'Roms/PORTS/Imgs/DIABLO II.png';icon.parent.mkdir(parents=True);shutil.copy2(source/'assets/ports/DIABLO II.png',icon)
 for src,dst in [('packaging/INSTALL.md','INSTALL.txt'),('README.md','README.md'),('BUILDING.md','BUILDING.md'),('LICENSE','COPYING'),('THIRD-PARTY.md','THIRD-PARTY.md'),('assets/ports/ARTWORK-NOTICE.txt','ARTWORK-NOTICE.txt')]:shutil.copy2(source/src,stage/dst)
 readme=stage/'README.md';readme.write_text(readme.read_text().replace('(packaging/INSTALL.md)','(INSTALL.txt)').replace('(assets/ports/ARTWORK-NOTICE.txt)','(ARTWORK-NOTICE.txt)'))
 sources=stage/'Sources';sources.mkdir();archive=sources/f'diablo2-miyoo-{version}-corresponding-source.tar.gz'
 names=sorted(source_manifest)+['SOURCE-MANIFEST.json']
 with tarfile.open(archive,'w:gz',compresslevel=3) as t:
  for n in names:t.add(source/n,arcname=f'diablo2-miyoo-{version}/'+n,recursive=False)
 source_zip=folder/f'Diablo-II-Miyoo-{version}-source.zip'
 with zipfile.ZipFile(source_zip,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for n in names:
   if not n.startswith('work/'):z.write(source/n,f'diablo2-miyoo-{version}/'+n)
 manifest={'version':version,'game_data_included':False,'player_state_included':False,'optional_promotional_cover_included':True,'static_menu_logo_embedded':False,'source_manifest_sha256':sha(source/'SOURCE-MANIFEST.json'),'files':{p.relative_to(stage).as_posix():sha(p) for p in sorted(stage.rglob('*')) if p.is_file()}}
 (stage/'BUILD-MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
 for p in stage.rglob('*'):
  if p.is_file():assert allowed(p.relative_to(stage)) and 'state' not in p.relative_to(stage).parts,p
 zip_path=folder/f'Diablo-II-Miyoo-{version}.zip'
 with zipfile.ZipFile(zip_path,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(stage.rglob('*')):
   if p.is_file():z.write(p,p.relative_to(stage))
 for p in [zip_path,source_zip]:
  with zipfile.ZipFile(p) as z:assert z.testzip() is None
 (folder/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.name}\n' for p in [zip_path,source_zip]))
 print('Player ZIP:',zip_path,flush=True)
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--export-source',type=Path);p.add_argument('--source-dir',type=Path);p.add_argument('--build-dir',type=Path,default=ROOT/'work/build/console');a=p.parse_args()
 if a.export_source:export_source(a.export_source.resolve())
 else:
  if not a.source_dir:p.error('--source-dir must identify the clean source export used to build the release')
  package(a.build_dir.resolve(),a.source_dir.resolve())
