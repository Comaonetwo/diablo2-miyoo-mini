#!/usr/bin/env python3
"""Controller selection at range; normal Pickup walks to and collects the item.
Debug only places the reproducible hero/drop and checks collision for the path.
"""
import json,os,re,socket,subprocess,time,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=Path(os.environ['D2_TEST_DATA']);user=root/f'work/runtime/loot-range-040-{os.getpid()}';user.mkdir();(user/'d2d.cfg').write_text('text_scale=200\nui_scale=100\ncompact_hud=1\nloot_near=0\n')
sock=f'/tmp/d2-loot-range-{os.getpid()}.sock';log=open(root/'work/loot-range-040.log','w');out={'physical_console':False};p=None
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,'closed';a+=b.decode()
  assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
  return a
def frames(n=4):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;t=time.monotonic()+90
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<t;time.sleep(.03)
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def key(k):cmd('key '+k);frames()
def pad(k):cmd('pad '+k+' down');frames(2);cmd('pad '+k+' up');frames(3)
try:
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--no-video','--no-save','--data',str(data),'--seed','3','--start-screen','ingame','--start-class','0','--start-name','LootRange','--devctl',sock],env=env,stdout=log,stderr=log);t=time.monotonic()+120
 while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(12)
 # A clear corridor in camp; choose it using actual collision, not bypassing walls.
 route=None
 for x,y in [(30.5,13.5),(50.5,16.5),(40.5,30.5),(25.5,33.5),(35.5,20.5)]:
  for dx,dy in [(1,0),(-1,0),(0,1),(0,-1),(.707,.707),(-.707,.707)]:
   if all(cmd(f'debug blocked {x+dx*n/2} {y+dy*n/2}').strip()=='ok 0' for n in range(12)):
    route=(x,y,x+5.5*dx,y+5.5*dy);break
  if route:break
 assert route,'no free corridor';x,y,gx,gy=route;cmd(f'debug warp {gx} {gy}');frames();drop=int(cmd('debug give hp1').split('#')[1].split()[0]);frames()
 ground=next(l.split('\t') for l in cmd('ground').splitlines() if f'#{drop}' in l);gx,gy=map(float,ground[-2:]);endpoint=None
 for dx,dy in [(0.2,0.979795897),(0.979795897,0.2),(-0.2,-0.979795897),(-0.979795897,-0.2)]:
  xx,yy=gx+5.0*dx,gy+5.0*dy
  if all(cmd(f'debug blocked {gx+5*dx*n/20} {gy+5*dy*n/20}').strip()=='ok 0' for n in range(21)):
   sx,sy=400+((gx-xx)-(gy-yy))*80,292+((gx-xx)+(gy-yy))*40
   if 0<sx<800 and 0<sy<499:endpoint=(xx,yy);break
 assert endpoint,'no free visible corridor';x,y=endpoint;cmd(f'debug warp {x} {y}');frames();out['initial_distance']=math.hypot(gx-x,gy-y);assert 4.5<out['initial_distance']<6
 pad('x');frames(30);assert f'#{drop}\t' in cmd('ground');out['cursor_mode_keeps_short_controller_range']='PASS'
 cmd(f'cmd move {x} {y}');frames();key('Escape');cmd(f'debug warp {x} {y}');frames();
 if state()['gamemenu'].startswith('1'):key('Escape')
 key('F10')
 current=int(re.search(r'row=(\d+)',cmd('port'))[1]);
 for _ in range((6-current)%16):key('Down')
 key('Right');key('Escape');assert 'loot_near = 1' in (user/'d2d.cfg').read_text()
 cmd(f'debug warp {x} {y}');frames();cmd('pad x down');frames(1);cmd('pad x up');frames(2)
 cmd('screenshot '+str(root/'outputs/prototype-0.2.18-loot-selection.png'))
 out['after_x']=state();assert out['after_x']['walking']=='1',out['after_x']
 t=time.monotonic()+15
 while f'#{drop}\t' in cmd('ground') and time.monotonic()<t:frames(6)
 assert f'#{drop}\t' not in cmd('ground'),cmd('ground');out['nearby_selection_walked_and_picked_up']='PASS';out['inventory_after']=cmd('items');out['final_state']=state()
 cmd('quit');assert p.wait(timeout=20)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/loot-range-040-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
