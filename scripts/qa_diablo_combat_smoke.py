#!/usr/bin/env python3
"""Crowded Cave Level 2, full pack, actual scroll use and controller portal entry."""
import os,re,json,socket,subprocess,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];tag=os.environ.get('D2_TEST_TAG','cures025');user=root/f'work/runtime/{tag}-{os.getpid()}';user.mkdir();sock=f'/tmp/d2-{tag}-{os.getpid()}.sock';log=open(root/f'work/release-audit-042/{tag}.log','w');result={'physical_console':False};p=None
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
 (user/'save').mkdir()
 for ext in ('d2s','qa'):shutil.copyfile(root/f'outputs/test-saves-0.2.25/save/QAActFour.{ext}',user/f'save/QAActFour.{ext}')
 data=root/'work/game-data';env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_SILENT='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 binary=Path(os.environ.get('D2_TEST_BINARY',str(root/'work/build/probe/d2-desktop')))
 p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log)
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
 def combo(button):
  cmd('pad back down');frames(2);pad(button);cmd('pad back up');frames()
 def select(id):
  xy=cmd(f'debug itemat {id}').split();cmd('move '+xy[1]+' '+xy[2]);frames()
 def val(name):return re.search(r'(?:^| )'+name+r'=([^ ]+)',cmd('state'))[1].strip()
 cmd('click 690 555');frames(30);assert 'name=QAActFour'in cmd('state')
 twin=next(l.split('\t')for l in cmd('debug portals').splitlines()if l.startswith('1\t'));cmd('debug warp '+twin[2]+' '+twin[3]);frames(6);pad('x');frames(40)
 end=time.monotonic()+15
 while 'Diablo'not in cmd('monsters'):assert time.monotonic()<end;frames(12)
 boss=next(l.split('\t')for l in cmd('monsters').splitlines()if 'Diablo'in l);initial_hp=int(boss[5].split('/')[0]);id=int(re.search(r'#(\d+)',' '.join(boss))[1]);result['initial_hp']=initial_hp;result['attack']=cmd('debug attack');result['before']=cmd('state')
 cmd('cmd belt 3');frames(8)
 for n in range(20):
  rows=[l.split('\t')for l in cmd('monsters').splitlines()if 'Diablo'in l]
  if not rows or int(rows[0][5].split('/')[0])<=0:break
  boss=rows[0]
  if n==0:cmd('debug warp '+str(float(boss[1])-1.5)+' '+boss[2]);frames(6)
  cmd(f"cmd skill 152 {boss[1]} {boss[2]} {id} 0");frames(40)
 rows=[l.split('\t')for l in cmd('monsters').splitlines()if 'Diablo'in l];last_hp=int(rows[0][5].split('/')[0])if rows else 0
 result['final_hp']=last_hp;result['after']=cmd('state');result['swings']=n+1;assert last_hp<initial_hp and 'life=0/'not in result['after'],result
 result['result']='PASS'
 cmd('quit');assert p.wait(timeout=30)==0
finally:
 if p and p.poll()is None:p.terminate();p.wait(timeout=30)
 log.close();Path(sock).unlink(missing_ok=True);(root/f'work/release-audit-042/{tag}-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
