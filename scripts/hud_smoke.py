#!/usr/bin/env python3
"""Private-asset integration: port menu, panel scaling, field crossing and exit."""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/'work/runtime/hud-036-user';user.mkdir(exist_ok=True)
sock=f'/tmp/d2-launch-{os.getpid()}.sock';log=open(root/'work/hud-036.log','w');p=None;out={'physical_console':False}
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(60);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):a+=s.recv(65536).decode()
  if re.search(r'(?:^|\n)err(?:\s|$)',a):raise RuntimeError(a)
  return a
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def frames(n):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;t=time.monotonic()+60
 while int(cmd('info').split('frame=')[1].split()[0])<goal:
  assert time.monotonic()<t;time.sleep(.03)
try:
 (user/'d2d.cfg').write_text('text_scale=100'+chr(10)+'ui_scale=125'+chr(10))
 data=Path(os.environ.get('D2_TEST_DATA',root/('outputs/private/diablo2-miyoo-test-'+(root/'VERSION').read_text().strip()+'/Roms/PORTS/Games/DiabloIIResearch/data')))
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_SILENT='1',D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--no-save','--seed','3','--start-screen','ingame','--start-class','0','--start-name','MiyooTest','--devctl',sock],env=env,stdout=log,stderr=log)
 t=time.monotonic()+120
 while not Path(sock).exists():
  assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(8)

 def key(k):cmd('key '+k);frames(4)
 def shot(name):cmd('screenshot '+str(root/'outputs'/('prototype-0.2.16-'+name+'.png')))
 def click(x,y):cmd(f'click {x} {y}');frames(5)
 out['default']=cmd('port');assert 'hud=compact' in out['default'];shot('hud-compact')
 out['layout']=cmd('port layout')
 rects={int(line.split()[0]):list(map(int,line.split()[1:5])) for line in out['layout'].splitlines() if line and line!='ok'}
 def centre(part):
  x,y,w,h=rects[part];return x+w//2,y+h//2
 def quick(index):
  click(*centre(100+index))

 # The experience strip remains above stamina and responds to actual stat data.
 from png_read import read_png
 w,h,ch,pixels=read_png(root/'outputs'/'prototype-0.2.16-hud-compact.png')
 assert rects[6][:2]==[148,509] and rects[7][:2]==[568,509],rects
 out['single_row_layout']='PASS'
 xp_counts=[]
 for exp in (0,100,375):
  cmd(f'debug stat 13 {exp}');frames(4);assert int(state()['exp'])==exp
  name='hud-xp-'+str(exp);shot(name)
  w,h,ch,pixels=read_png(root/'outputs'/('prototype-0.2.16-'+name+'.png'))
  x,y,bw,bh=rects[5];row=pixels[y+(562-558)*bh//10+1]
  # Count the most frequent newly filled bar colour rather than background art.
  strip=[tuple(row[px*ch:px*ch+3]) for px in range(x+10,x+bw-10)]
  xp_counts.append(strip)
 delta1=sum(a!=b for a,b in zip(xp_counts[0],xp_counts[1]));delta2=sum(a!=b for a,b in zip(xp_counts[0],xp_counts[2]))
 assert 70<delta1<150 and 330<delta2<410,(delta1,delta2)
 out['experience_strip']={'result':'PASS','changed_pixels_100_xp':delta1,'changed_pixels_375_xp':delta2}
 cmd('debug stat 13 0');frames(3)

 assert state()['held']=='-',state()
 click(*centre(4));assert state()['held']!='-',state()
 click(*centre(4));assert state()['held']=='-',state();out['belt_pick_and_replace']='PASS'
 cmd('debug statpts 5');cmd('debug skillpts 2');frames(4);shot('hud-points')
 key('C');assert state()['inv']=='0';shot('hud-character-button');key('Escape')
 key('I');assert state()['inv']=='1';key('Escape')
 key('T');assert state()['tree']=='1';key('Escape')
 out['shoulder_panels']='PASS'
 click(*centre(8));shot('hud-stat-button');key('Escape')
 click(*centre(9));assert state()['tree']=='1',state();key('Escape');out['raised_notifications']='PASS'
 before=cmd('port');cmd('pad back down');frames(2);cmd('pad y down');frames(2);cmd('pad y up');cmd('pad back up');frames(4);assert ('running=true' in before)!=('running=true' in cmd('port'));out['run_combo']='PASS'
 click(*centre(6));assert state()['picker']=='1',state();shot('hud-skill-picker');cmd('pad dpdown down');frames(4);cmd('pad dpdown up');frames(3);cmd('pad dpup down');frames(4);cmd('pad dpup up');frames(3);cmd('pad a down');frames(4);cmd('pad a up');frames(4);assert state()['picker']=='0',state();out['skill_hitbox']='PASS'
 before=state()['cam'];click(110,500);assert state()['cam']==before;out['globe_does_not_walk']='PASS'
 cmd('debug statpts 0');cmd('debug skillpts 0');cmd('move 400 300');frames(4);shot('hud-compact')
 # Original HUD selector is independent of the interface scale.
 key('F10');key('Down');key('Down');key('Right');assert 'hud=original' in cmd('port');shot('hud-settings-original');key('Escape');shot('hud-original')
 out['original_preferences']=(user/'d2d.cfg').read_text();assert 'compact_hud = 0' in out['original_preferences']
 argv=p.args;cmd('quit');assert p.wait(timeout=15)==0;Path(sock).unlink(missing_ok=True)
 p=subprocess.Popen(argv,env=env,stdout=log,stderr=log)
 t=time.monotonic()+120
 while not Path(sock).exists():
  assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(8);assert 'hud=original' in cmd('port');out['original_restored_after_restart']='PASS'
 key('F10');key('Down');key('Down');key('Right');assert 'hud=compact' in cmd('port');shot('hud-settings-compact');key('Escape')
 out['compact_preferences']=(user/'d2d.cfg').read_text();assert 'compact_hud = 1' in out['compact_preferences']
 out['style_toggle_and_persistence']='PASS';out['memory']=cmd('memory')
 cmd('quit');assert p.wait(timeout=15)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=15)
 log.close();Path(sock).unlink(missing_ok=True)
 (root/'work/hud-036-result.json').write_text(json.dumps(out,indent=2)+chr(10))
print(json.dumps(out,indent=2))
