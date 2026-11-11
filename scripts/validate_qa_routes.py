#!/usr/bin/env python3
"""Reload and traverse every distributed QA fixture on isolated copies."""
from pathlib import Path
import os,subprocess,socket,json,re,shutil,time
root=Path(__file__).resolve().parents[1];bundle=root/'outputs/test-saves-0.2.25';results=[]
binary=Path(os.environ.get('D2_TEST_BINARY',root/'work/build/probe/d2-desktop'))
for fixture in json.loads((bundle/'manifest.json').read_text()):
 name=fixture['name'];user=root/f'work/runtime/routes-{name}-{os.getpid()}';(user/'save').mkdir(parents=True)
 for ext in ('d2s','qa'):shutil.copyfile(bundle/'save'/f'{name}.{ext}',user/'save'/f'{name}.{ext}')
 sock=f'/tmp/d2routes-{name}-{os.getpid()}.sock';log=(root/f'work/release-048/routes-{name}.log').open('w');p=None
 def cmd(line):
  with socket.socket(socket.AF_UNIX)as s:
   s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());out=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',out):
    b=s.recv(65536);assert b;out+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',out),(name,line,out)
   return out
 def frames(n=5):
  end=time.monotonic()+180;at=int(cmd('info').split('frame=')[1].split()[0])+n
  while int(cmd('info').split('frame=')[1].split()[0])<at:assert time.monotonic()<end;time.sleep(.01)
 def interact():cmd('pad x down');frames(3);cmd('pad x up');frames(18)
 def go(which,dest):
  portal=next(l.split('\t')for l in cmd('debug portals').splitlines()if l.startswith(str(which)+'\t'));end=time.monotonic()+20
  while int(cmd('debug level').split()[1])!=dest:
   assert time.monotonic()<end,(name,dest,cmd('state'))
   cmd('debug warp '+portal[2]+' '+portal[3]);frames();interact()
 try:
  data=root/'work/game-data';env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log);end=time.monotonic()+180
  while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.05)
  cmd('ping');frames();cmd('click 690 555');frames(20);state=cmd('state');assert 'name='+name in state and 'level='+str(fixture['level'])+' 'in state
  assert int(cmd('debug level').split()[1])==fixture['town'];assert cmd('debug unbuilt').strip()=='ok'
  items=cmd('items');assert items.count('[')==fixture['items'].count('[')
  go(1,fixture['portal_destination']);frames(20);boss={37:'Andariel',73:'Duriel',102:'Mephisto',108:'Diablo'}.get(fixture['portal_destination'])
  if boss:assert boss in cmd('monsters'),(name,boss)
  go(0,fixture['town']);cmd('save');cmd('quit');assert p.wait(timeout=30)==0
  results.append({'hero':name,'destination':fixture['portal_destination'],'result':'PASS'});print(name,'PASS',flush=True)
 finally:
  if p and p.poll()is None:p.terminate();p.wait(timeout=30)
  Path(sock).unlink(missing_ok=True);log.close()
(root/'work/release-048/qa-route-validation.json').write_text(json.dumps({'binary':str(binary),'physical_console':False,'results':results},indent=2)+'\n')
