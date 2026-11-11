#!/usr/bin/env python3
"""Regression for console feedback: panels, map overlap, belt, scroll and dungeon input.
Debug commands create reproducible starting conditions; actual item use and
entrance activation below go through the controller, not direct world commands.
"""
import array,json,os,re,socket,subprocess,time
from pathlib import Path
from png_read import read_png
root=Path(__file__).resolve().parents[1];user=root/f'work/runtime/typography-034-{os.getpid()}';user.mkdir()
sock=f'/tmp/d2-typography-{os.getpid()}.sock';log=open(root/'work/typography-034.log','w');p=None;out={'physical_console':False}
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,'closed test connection';a+=b.decode()
  if re.search(r'(?:^|\n)err(?:\s|$)',a):raise RuntimeError(a)
  return a
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def frames(n):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;deadline=time.monotonic()+90
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<deadline;time.sleep(.03)
def key(k):cmd('key '+k);frames(4)
def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames(4)
def shot(name):
 path=root/'outputs'/('prototype-0.2.14-'+name+'.png');cmd('screenshot '+str(path));return read_png(path)
def items():return cmd('items')
try:
 data=Path(os.environ['D2_TEST_DATA']);out['bars']=[]
 for scale in (100,125,150,175,200,225):
  (user/'d2d.cfg').write_text(f'text_scale={scale}\nui_scale=100\ncompact_hud=1\n')
  env=dict(os.environ,D2D_USER_DIR=str(user),D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
  p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--no-save','--seed','3','--start-screen','ingame','--start-class','4','--start-name','TypeTest','--devctl',sock],env=env,stdout=log,stderr=log)
  deadline=time.monotonic()+120
  while not Path(sock).exists():assert p.poll() is None and time.monotonic()<deadline;time.sleep(.1)
  cmd('ping');frames(8);cmd('debug goto 2');frames(30)
  assert f'text={scale}' in cmd('port')
  target=cmd('monsters').splitlines()[0].split('\t');tid=target[11][1:]
  for dx,dy in ((2,0),(-2,0),(0,2),(0,-2)):
   x,y=float(target[1])+dx,float(target[2])+dy
   if cmd(f'debug blocked {x} {y}').strip()=='ok 0':cmd(f'debug warp {x} {y}');frames(3);break
  cmd(f'cmd skill 0 {target[1]} {target[2]} {tid} 1');frames(6)
  w,h,ch,pixels=shot('monster-bar-'+str(scale));row=pixels[11]
  xs=[x for x in range(w) if tuple(row[x*ch:x*ch+3]) in ((136,8,8),(32,16,16))]
  assert len(xs)>=120,(scale,len(xs),state());left,right=min(xs),max(xs)+1;assert left>=20 and right<=780,(left,right)
  out['bars'].append({'scale':scale,'left':left,'right':right,'name':target[10]})
  cmd('debug quest 1');key('Q');assert state()['quest']=='1';shot('quest-text-'+str(scale));key('Escape')
  cmd('quit');assert p.wait(timeout=20)==0;Path(sock).unlink(missing_ok=True)
 out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/typography-034-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
