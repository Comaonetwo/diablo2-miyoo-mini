#!/usr/bin/env python3
"""Crowded Cave Level 2, full pack, actual scroll use and controller portal entry."""
import os,re,json,socket,subprocess,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];tag=os.environ.get('D2_TEST_TAG','belt020');user=root/f'work/runtime/{tag}-{os.getpid()}';user.mkdir();sock=f'/tmp/d2-{tag}-{os.getpid()}.sock';log=open(root/f'work/release-audit-042/{tag}.log','w');result={'physical_console':False};p=None
def cmd(line):
 with socket.socket(socket.AF_UNIX)as s:
  s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());out=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',out):
   b=s.recv(65536);assert b;out+=b.decode()
  assert not re.search(r'(?:^|\n)err(?:\s|$)',out),(line,out)
  return out
def frames(n=4):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+180
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<end;time.sleep(.01)
def key(k):cmd('key '+k);frames()
def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames()
def give(code,pick=False):
 before={int(n)for n in re.findall(r' id=(\d+)',cmd('items'))}
 item=int(cmd('debug give '+code).split('#')[1].split()[0])
 if pick:
  cmd(f'cmd pickup {item}');frames(4)
  carried=[int(n)for n in re.findall(r' id=(\d+)',cmd('items'))if int(n)not in before]
  assert len(carried)==1,(code,cmd('items'));return carried[0]
 return item
try:
 (user/'d2d.cfg').write_text('sharp_panel_text='+os.environ.get('D2_TEST_SHARP','0')+'\ntext_scale=200\nui_scale=125\ncompact_hud=1\nbelt_sequential=1\nbelt_refill=1\n')
 data=root/'work/game-data';env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_SILENT='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 binary=Path(os.environ.get('D2_TEST_BINARY',str(root/'work/build/probe/d2-desktop')))
 p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen','ingame','--start-class','0','--start-name','BeltTest','--devctl',sock],env=env,stdout=log,stderr=log)
 end=time.monotonic()+180
 while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.1)
 cmd('ping');frames(8)
 def entries():
  return [dict(code=m[0],loc=int(m[1]),panel=int(m[2]),id=int(m[3]),box=int(m[4]))for m in re.findall(r'\[(\w+) loc=(\d+) slot=\d+ q=\d+ panel=(\d+) id=(\d+) size=\dx\d at=(\d+),\d+\]',cmd('items'))]
 def slot(n):return next(i for i in entries()if i['loc']==2 and i['box']==n)
 def exists(id):return any(i['id']==id for i in entries())
 def menu(row):
  key('F10');current=int(re.search(r'row=(\d+)',cmd('port'))[1])
  for _ in range((row-current)%16):key('Down')
 def change(row):menu(row);key('Right');key('Escape')
 def shot(name):cmd('screenshot '+str(root/'outputs/feedback-020'/f'{tag}-{name}-800.png'))
 def quick(id):
  xy=cmd(f'debug itemat {id}').split();cmd('move '+xy[1]+' '+xy[2]);frames();cmd('pad back down');frames(2);pad('x');cmd('pad back up');frames()
 # Refill must transfer, not duplicate; preserve the exact inventory item's id.
 reserve=give('hp1',True);assert next(i for i in entries()if i['id']==reserve)['loc']==0
 consumed=slot(2)['id'];cmd(f'cmd use {consumed}');frames();assert not exists(consumed)and slot(2)['id']==reserve
 result['exact_refill']='PASS';change(11);assert 'refill=0'in cmd('port')
 reserves=[give(code,True)for code in ('mp1','hp1','mp2','hp2')]
 for i in list(entries()):
  if i['loc']==2:cmd(f"cmd use {i['id']}");frames()
 assert not any(i['loc']==2 for i in entries());key('I')
 for n,id in enumerate(reserves):quick(id);assert slot(n)['id']==id,(n,entries())
 result['quick_belt_left_to_right']='PASS';shot('inventory');key('Escape')
 for button,n in [('leftshoulder',0),('leftshoulder',1),('rightshoulder',3),('rightshoulder',2)]:
  id=slot(n)['id'];pad(button);assert not exists(id),(button,n,entries())
 result['sequential_shoulders']='PASS';change(10);assert 'belt=potions'in cmd('port')
 for code in ('mp1','hp1','mp2','hp2'):give(code,True)
 health=slot(1)['id'];mana=slot(0)['id'];pad('leftshoulder');assert not exists(health)and exists(mana);pad('rightshoulder');assert not exists(mana)
 result['potions_shoulders']='PASS';key('C');shot('stats');key('Escape');menu(13);key('Return');assert 'controls=1'in cmd('port');shot('controls');key('Escape');key('Escape');menu(10);shot('settings');key('Escape')
 presets=cmd('debug presets').splitlines();line=next(x.split('\t')for x in presets if 'Charsi'in x);cmd('cmd trade '+line[0]);frames(8);assert 'store=-1'not in cmd('state');cmd('move 120 76');frames();pad('dpright');pad('a');frames();assert 'store_tab=1'in cmd('state'),cmd('state');shot('store');result['store_tab_dpad']='PASS'
 cmd('quit');assert p.wait(timeout=30)==0
 cfg=(user/'d2d.cfg').read_text().replace(' ','');assert 'belt_sequential=0'in cfg and 'belt_refill=0'in cfg
 Path(sock).unlink(missing_ok=True)
 p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--start-screen','title','--devctl',sock],env=env,stdout=log,stderr=log)
 end=time.monotonic()+180
 while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.1)
 cmd('ping');frames();assert 'belt=potions refill=0'in cmd('port');cmd('quit');assert p.wait(timeout=30)==0;result['settings_restart']='PASS';result['result']='PASS'
finally:
 if p and p.poll()is None:p.terminate();p.wait(timeout=30)
 log.close();Path(sock).unlink(missing_ok=True);(root/f'work/release-audit-042/{tag}-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
