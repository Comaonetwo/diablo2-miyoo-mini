#!/usr/bin/env python3
"""Private-asset integration: port menu, panel scaling, field crossing and exit."""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/'work/runtime/port-ui-034-user';user.mkdir(exist_ok=True)
sock=f'/tmp/d2-launch-{os.getpid()}.sock';log=open(root/'work/port-ui-034.log','w');p=None;out={'physical_console':False}
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
 def key(k):cmd('key '+k);frames(3)
 def shot(name):cmd('screenshot '+str(root/'outputs'/name))
 shot('prototype-0.2.14-hud125-mac.png')
 cmd('pad back down');cmd('pad start down');frames(4);assert 'port=1' in cmd('port')
 cmd('pad start up');cmd('pad back up');frames(4)
 cmd('pad b down');frames(4);cmd('pad b up');frames(4);assert 'port=0' in cmd('port')
 assert state()['picker']=='0',state()
 out['controller_port_menu']='PASS';out['fixed_hud_before_panel_scale']=cmd('port layout')
 key('F10');assert 'port=1' in cmd('port')
 shot('prototype-0.2.14-settings-mac.png')
 for value in [125,150,175,200,225]:
  key('Right');assert 'text='+str(value) in cmd('port'),cmd('port')
 key('Down')
 for value in [150,175,200,225]:
  key('Right');assert 'ui='+str(value) in cmd('port'),cmd('port')
 shot('prototype-0.2.14-settings225-mac.png')
 for i in range(4):key('Down')
 key('Return');assert 'controls=1' in cmd('port');shot('prototype-0.2.14-controls-mac.png');key('Escape');key('Escape')
 assert 'port=0' in cmd('port');shot('prototype-0.2.14-hud225-mac.png');assert cmd('port layout')==out['fixed_hud_before_panel_scale'];out['panel_scale_does_not_resize_hud']='PASS'
 key('i');shot('prototype-0.2.14-inventory225-mac.png');
 for _ in range(9):
  cmd('pad dpdown down');frames(4);cmd('pad dpdown up');frames(4)
 shot('prototype-0.2.14-inventory-grid225-mac.png');key('i');key('c');shot('prototype-0.2.14-character225-mac.png');key('c')
 out['port_menu_scales_controls']='PASS';out['preferences']=(user/'d2d.cfg').read_text()
 # Draw town NPCs/modes from several parts of the camp before leaving.
 rows=[r.split(chr(9)) for r in cmd('npcs').splitlines()[:-1] if r.split(chr(9))[3]=='1']
 for npc in rows[:8]:
  cmd('debug warp '+str(float(npc[5])+1.5)+' '+str(float(npc[6])+1.5));frames(18)
 out['town_cache_after_visit']=cmd('memory')
 cmd('debug warp 54.5 16.5');frames(15);cmd('cmd move 61.5 16.5');frames(180)
 out['field']=cmd('debug level');assert out['field'].split()[1]=='2',out
 shot('prototype-0.2.14-field-mac.png');out['field_memory']=cmd('memory')
 key('Escape');cmd('click 400 260');frames(10);out['after_exit']=state();assert out['after_exit']['screen']=='charselect',state()
 shot('prototype-0.2.14-exit-mac.png');out['after_exit_memory']=cmd('memory')
 cmd('quit');assert p.wait(timeout=15)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=15)
 log.close();Path(sock).unlink(missing_ok=True)
 (root/'work/port-ui-034-result.json').write_text(json.dumps(out,indent=2)+chr(10))
print(json.dumps(out,indent=2))
