#!/usr/bin/env python3
"""Run release checks once per exact input fingerprint; logs stay in work/.

renderer: all small CTests, sprite pixel comparison, one heavy combat smoke.
gameplay: the same, plus inventory/death/shop, potions, QA routes and cube.
Changing simulation, saves, protocol or controls requires gameplay.
This is a local check runner, never an SD installer or a physical FPS test.
"""
import argparse,hashlib,json,os,subprocess,sys,time
from pathlib import Path
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--profile',choices=['renderer','gameplay'],required=True)
p.add_argument('--build-dir',type=Path,default=r/'work/build/sanitize')
p.add_argument('--cmake',default=os.environ.get('D2_CMAKE','cmake'))
p.add_argument('--output',type=Path,required=True)
p.add_argument('--force',action='store_true')
p.add_argument('--no-build',action='store_true',help='Use only after building the selected directory')
a=p.parse_args();build=a.build_dir.resolve();data=(r/'work/game-data').resolve();a.output=a.output.resolve()
ctest=str(Path(a.cmake).with_name('ctest')) if '/'in a.cmake else 'ctest'
cache=r/'work/check-cache';cache.mkdir(parents=True,exist_ok=True)
def digest(path):
 h=hashlib.sha256()
 with path.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def fingerprint():
 listing=json.loads(subprocess.check_output([ctest,'--test-dir',str(build),'--show-only=json-v1'],text=True))
 binaries={build/'d2-desktop'}
 for t in listing['tests']:binaries.add(Path(t['command'][0]))
 files={r/'VERSION',r/'CMakeLists.txt',Path(__file__)}
 for folder in ['src','tests','scripts','work/sources/d2decomped/apps/d2d','work/sources/d2decomped/components']:
  files.update(x for x in(r/folder).rglob('*')if x.is_file()and x.suffix in ['.hpp','.h','.cpp','.c','.py','.cmake','.json','.tsv']and '__pycache__'not in x.parts)
 signature={'profile':a.profile,'build':str(build),'tests':listing['tests'],'source':{str(x.relative_to(r)):digest(x)for x in sorted(files)},'binaries':{str(x):digest(x)for x in sorted(binaries)},'data':{},'environment':{k:v for k,v in os.environ.items()if k.startswith(('D2','SDL','ASAN','UBSAN','LSAN','TSAN'))}}
 # Private archives are immutable inputs; size/mtime changes invalidate reuse.
 # --force bypasses reuse, and package_private verifies their full SHA hashes.
 for x in sorted(data.iterdir()):
  if x.suffix.lower()in ['.mpq','.exe']:
   s=x.stat();signature['data'][str(x)]=[s.st_size,s.st_mtime_ns]
 for x in sorted((r/'outputs/test-saves-0.2.25').rglob('*')):
  if x.is_file():signature['data'][str(x)]=digest(x)
 key=hashlib.sha256(json.dumps(signature,sort_keys=True).encode()).hexdigest()
 return key,len(listing['tests']),signature['binaries']
# Let CMake discover changed dependency headers/generated inputs before reuse.
# Its unchanged build is cheap; skipping it requires the explicit --no-build.
if not a.no_build:subprocess.run([a.cmake,'--build',str(build),'-j4'],cwd=r,check=True,stdout=subprocess.DEVNULL)
if not a.force:
 try:
  key,count,binaries=fingerprint();prior=cache/(key+'.json')
  if prior.exists():
   result=json.loads(prior.read_text());assert result['passed']
   result['reused']=True;a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS: unchanged checked inputs; reused',key[:12]);sys.exit(0)
 except (FileNotFoundError,subprocess.CalledProcessError):pass
key,count,binaries=fingerprint();logdir=cache/key;logdir.mkdir(exist_ok=True)
steps=[('ctest',[ctest,'--test-dir',str(build),'--output-on-failure'])]
if a.profile=='gameplay':
 # Synthetic CTests cannot catch missing or wrongly filtered original rows.
 # Exercise real item properties/skills as part of the normal release gate.
 steps += [(name,[str(build/binary),str(data)]) for name,binary in [
  ('real-item-effects','d2-item-effects-test'),('real-item-mechanics','d2-item-mechanics-test')]]
smokes=['qa_diablo_combat_smoke.py']
if a.profile=='gameplay':smokes+=['cure_potions_smoke.py','feedback_024_smoke.py','validate_qa_routes.py','validate_qa_cube.py','corpse_skills_smoke.py']
steps +=[(n,[sys.executable,str(r/'scripts'/n)])for n in smokes]
result={'passed':False,'reused':False,'profile':a.profile,'fingerprint':key,'ctest_count':count,'binaries':binaries,'steps':[],'physical_console':False}
for name,command in steps:
 start=time.monotonic();log=logdir/(name+'.log');env=dict(os.environ,D2_TEST_BINARY=str(build/'d2-desktop'),D2_TEST_TAG='release-'+key[:10]+'-'+name.split('.')[0])
 env['ASAN_OPTIONS']=env.get('ASAN_OPTIONS','detect_leaks=0')+':halt_on_error=1'
 env['UBSAN_OPTIONS']=env.get('UBSAN_OPTIONS','')+':halt_on_error=1:print_stacktrace=1'
 with log.open('w')as f:code=subprocess.run(command,cwd=r,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=900).returncode
 result['steps'].append({'name':name,'exit_code':code,'seconds':round(time.monotonic()-start,3),'log':str(log.relative_to(r))})
 if code:break
result['passed']=len(result['steps'])==len(steps)and all(x['exit_code']==0 for x in result['steps'])
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
if result['passed']:(cache/(key+'.json')).write_text(json.dumps(result,indent=2)+'\n')
print(('PASS'if result['passed']else'FAIL')+': '+a.profile+'; '+str(count)+' CTests; logs '+str(logdir.relative_to(r)))
sys.exit(0 if result['passed']else 1)
