#!/usr/bin/env python3
"""Validate cube fixtures on copies, preserving the distributed baselines."""
from pathlib import Path
import os,re,json,socket,subprocess,time,shutil
root=Path(__file__).resolve().parents[1];data=root/'work/game-data';bundle=root/'outputs/test-saves-0.2.25';allresults=[]
for name,parts,expected in [('QAActTwo',[('msf',0,0),('vip',1,0)],'hst'),('QAActThree',[('qey',0,0),('qhr',1,0),('qbr',2,0),('qf1',0,1)],'qf2')]:
 user=root/f'work/runtime/cube-{name}-{os.getpid()}';(user/'save').mkdir(parents=True)
 for ext in ('d2s','qa'):shutil.copyfile(bundle/'save'/f'{name}.{ext}',user/'save'/f'{name}.{ext}')
 (user/'d2d.cfg').write_text('ui_scale=125\nreadable_font=1\nsharp_panel_text=1\n');sock=f'/tmp/d2cube-{name}-{os.getpid()}.sock';log=(root/f'work/release-048/cube-{name}.log').open('w');p=None;row={'hero':name}
 def cmd(line):
  with socket.socket(socket.AF_UNIX)as s:
   s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());out=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',out):
    b=s.recv(65536);assert b;out+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',out),(line,out)
   return out
 def frames(n=5):
  at=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+180
  while int(cmd('info').split('frame=')[1].split()[0])<at:assert time.monotonic()<end;time.sleep(.01)
 def id_for(code,panel=None):
  candidates=[m for m in re.findall(r'\[(\w+) loc=(\d+) slot=\d+ q=\d+ panel=(\d+) id=(\d+)',cmd('items'))if m[0]==code and(panel is None or int(m[2])==panel)]
  assert candidates,(code,cmd('items'));return int(candidates[0][3])
 def pad(b):cmd('pad '+b+' down');frames(3);cmd('pad '+b+' up');frames()
 try:
  env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
  binary=os.environ.get('D2_TEST_BINARY',str(root/'work/build/probe/d2-desktop'))
  p=subprocess.Popen([binary,'--headless','--data',str(data),'--no-video','--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log);end=time.monotonic()+180
  while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.05)
  cmd('ping');frames(8);cmd('click 690 555');frames(30)
  box=id_for('box',5);cmd(f'cmd hand {box}');frames();cmd('cmd grid 8 2');frames();assert id_for('box',1)==box
  cmd('key I');frames();xy=cmd('debug itemat '+str(box)).split();cmd('move '+xy[1]+' '+xy[2]);frames();pad('x');assert 'cube=1'in cmd('state'),cmd('state');row['open_cube_with_Y']='PASS'
  for code,x,y in parts:
   id=id_for(code,1);cmd(f'cmd hand {id}');frames();cmd(f'cmd grid {x} {y} cube');frames();assert id_for(code,4)==id
  cmd('move 244 337');frames();pad('a');frames(12);id_for(expected,4);row['recipe']=expected;row['transmute_with_B']='PASS';cmd('screenshot '+str(root/f'work/release-048/cube-{name}-result.png'))
  if name=='QAActTwo':
   staff=id_for('hst',4);cmd(f'cmd hand {staff}');frames();cmd('cmd grid 0 0');frames();assert id_for('hst',1)==staff
   gems=[int(m)for m in re.findall(r'\[gcr loc=0 slot=0 q=\d+ panel=5 id=(\d+)',cmd('items'))];assert len(gems)==3
   for x,id in enumerate(gems):cmd(f'cmd hand {id}');frames();cmd(f'cmd grid {x} 0 cube');frames()
   cmd('move 244 337');frames();pad('a');frames(12);id_for('gfr',4);row['three_chipped_rubies']='PASS'
  cmd('quit');assert p.wait(timeout=30)==0;row['result']='PASS';allresults.append(row)
 finally:
  if p and p.poll()is None:p.terminate();p.wait(timeout=30)
  log.close();Path(sock).unlink(missing_ok=True)
(root/'work/release-048/cube-validation.json').write_text(json.dumps(allresults,indent=2)+'\n');print(json.dumps(allresults,indent=2))
