#!/usr/bin/env python3
"""Crowded Cave Level 2, full pack, actual scroll use and controller portal entry."""
import os,re,json,socket,subprocess,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];tag=os.environ.get('D2_TEST_TAG','feedback024');user=root/f'work/runtime/{tag}-{os.getpid()}';user.mkdir();sock=f'/tmp/d2-{tag}-{os.getpid()}.sock';log=open(root/f'work/release-audit-042/{tag}.log','w');result={'physical_console':False};p=None
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
 (user/'d2d.cfg').write_text('sharp_panel_text='+os.environ.get('D2_TEST_SHARP','1')+'\ntext_scale=200\nui_scale=125\ncompact_hud=1\nbelt_sequential=1\nbelt_refill=1\n')
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
  for _ in range((0-current)%5):key('Down')
  key('Return')
  option={0:0,1:1,2:2,4:3,5:4,6:5,7:6,8:7,10:8,11:9}[row]
  for _ in range(option):key('Down')
 def change(row):menu(row);key('Right');key('Escape');key('Escape')
 def shot(name):cmd('screenshot '+str(root/'outputs/feedback-020'/f'{tag}-{name}-800.png'))
 def quick(id):
  xy=cmd(f'debug itemat {id}').split();cmd('move '+xy[1]+' '+xy[2]);frames();cmd('pad back down');frames(2);pad('x');cmd('pad back up');frames()
 def combo(button):
  cmd('pad back down');frames(2);pad(button);cmd('pad back up');frames()
 def select(id):
  xy=cmd(f'debug itemat {id}').split();cmd('move '+xy[1]+' '+xy[2]);frames()
 def val(name):return re.search(r'(?:^| )'+name+r'=([^ ]+)',cmd('state'))[1].strip()
 # Field SELECT+physical Y switches sets, without opening a panel/map.
 initial=val('wset');combo('x');assert val('wset')!=initial,cmd('state');combo('x');assert val('wset')==initial
 result['weapon_swap_shortcut']='PASS'
 key('I');cmd('move 455 200');frames();pad('dpup');pad('dpup');pad('dpright');pad('a');assert val('wset')=='1',cmd('state');pad('dpleft');pad('a');assert val('wset')=='0',cmd('state');key('Escape');result['weapon_swap_tabs']='PASS'
 # Drop an inventory item through physical SELECT+X and pick it back up.
 worn_cap=give('cap',True);assert next(i for i in entries()if i['id']==worn_cap)['loc']==1
 dropped=give('cap',True);key('I');select(dropped);combo('y');assert not exists(dropped)and 'Cap'in cmd('ground'),(cmd('items'),cmd('ground'));key('Escape')
 result['drop_shortcut']='PASS'
 # Sequential visits an otherwise ordinary stamina potion and consumes it.
 change(11)
 for i in list(entries()):
  if i['loc']==2:cmd(f"cmd use {i['id']}");frames()
 stamina=give('vps',True)
 key('I');quick(stamina);key('Escape');cmd('debug stat 10 0');frames();pad('leftshoulder');assert not exists(stamina),cmd('items')
 assert val('stamina').split('/')[0]==val('stamina').split('/')[1] and int(val('stamina_until'))>0,cmd('state')
 result['stamina_sequential']='PASS'
 # Check each standard potion family through the real UseItem path.
 result['potions']={}
 for code in ('hp1','hp2','hp3','hp4','hp5','mp1','mp2','mp3','mp4','mp5','rvs','rvl','yps','wms','vps'):
  id=give(code,True);cmd('cmd use '+str(id));frames(12);assert not exists(id),(code,cmd('items'));result['potions'][code]='consumed'
 # Stats labels must not survive a later opaque shop panel.
 key('C');presets=cmd('debug presets').splitlines();line=next(x.split('\t')for x in presets if 'Charsi'in x);cmd('cmd trade '+line[0]);frames(8);shot('shop-over-stats');assert 'store=-1'not in cmd('state');key('Escape')
 result['shop_over_stats']='rendered'
 # Death/corpse: gear leaves the body slots; inventory survives; Y restores gear.
 cmd('debug goto 2');frames(12);cmd('debug kill 0');frames(8)
 equipment={i['id']for i in entries()if i['loc']==1};assert equipment,cmd('items')
 inventory={i['id']for i in entries()if i['loc']==0};cmd('debug stat 14 159');frames();cmd('debug die');frames(16)
 end_death=time.monotonic()+30
 while 'view=1'not in cmd('debug corpses'):assert time.monotonic()<end_death,cmd('state');frames(32)
 assert 'view=1'in cmd('debug corpses'),cmd('debug corpses');assert not any(i['loc']==1 for i in entries())
 assert inventory<={i['id']for i in entries()},cmd('items');result['death_ground']=cmd('ground');result['death_before']=cmd('debug corpses');result['death_state']=cmd('state')
 cmd('cmd resurrect');frames(60);corpse=cmd('debug corpses').splitlines()[0].split('\t');cmd('debug goto '+corpse[0]);frames(8);cmd('debug kill 0');cmd('debug warp '+corpse[1]+' '+corpse[2]);frames(8);pad('x');frames(120)
 assert 'view=0'in cmd('debug corpses'),cmd('debug corpses');assert equipment<={i['id']for i in entries()if i['loc']==1},cmd('items');result['corpse_digital_recovery']='PASS'
 # Save while a second corpse exists, then recover it after loading in town.
 gear_codes=sorted(i['code']for i in entries()if i['loc']==1)
 cmd('debug die');frames(16);end_death=time.monotonic()+30
 while 'view=1'not in cmd('debug corpses'):assert time.monotonic()<end_death,cmd('state');frames(32)
 cmd('save');cmd('quit');assert p.wait(timeout=30)==0;Path(sock).unlink(missing_ok=True)
 p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log)
 end=time.monotonic()+180
 while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.1)
 cmd('ping');frames(10);cmd('click 690 555');frames(30);assert 'screen=ingame'in cmd('state'),cmd('state')
 assert 'view=1'in cmd('debug corpses'),cmd('debug corpses');corpse=cmd('debug corpses').splitlines()[0].split('\t');cmd('debug warp '+corpse[1]+' '+corpse[2]);frames(8);pad('x');frames(120)
 assert 'view=0'in cmd('debug corpses'),cmd('debug corpses');assert sorted(i['code']for i in entries()if i['loc']==1)==gear_codes,cmd('items')
 result['corpse_save_reload_recovery']='PASS';cmd('save');result['result']='PASS' 
 cmd('quit');assert p.wait(timeout=30)==0
finally:
 if p and p.poll()is None:p.terminate();p.wait(timeout=30)
 log.close();Path(sock).unlink(missing_ok=True);(root/f'work/release-audit-042/{tag}-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
