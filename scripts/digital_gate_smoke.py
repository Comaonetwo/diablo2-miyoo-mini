#!/usr/bin/env python3
"""D-pad crossing from camp through the gate, after test-only placement nearby."""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/'work/runtime/digital-gate-031-user';user.mkdir(exist_ok=True)
sock=f'/tmp/d2-launch-{os.getpid()}.sock';log=open(root/'work/digital-gate-031.log','w');p=None;out={'physical_console':False}
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
 (user/'d2d.cfg').write_text('text_scale=175'+chr(10)+'ui_scale=175'+chr(10))
 data=Path(os.environ['D2_TEST_DATA'])
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_LOOPBACK='1',D2D_AUDIO_CAPTURE=str(root/'work/gate-031.pcm'),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--no-save','--seed','3','--start-screen','ingame','--start-class','0','--start-name','MiyooTest','--devctl',sock],env=env,stdout=log,stderr=log)
 t=time.monotonic()+120
 while not Path(sock).exists():
  assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(8);out['port_settings']=cmd('port');assert 'text=175 ui=175' in out['port_settings'],out['port_settings']
 out['initial_level']=cmd('debug level');out['town_audio']=cmd('audio status');out['sound_70']=cmd('audio info 70')
 cmd('debug warp 50.5 16.5');frames(4)
 out['route']=[]
 for i in range(130):
  level=cmd('debug level').split()
  out['route'].append(level[1:4])
  if level[1]=='2':break
  x,y=map(float,level[2:4]);direction='dpdown' if y<16.5 else 'dpright'
  cmd('pad '+direction+' down');frames(4);cmd('pad '+direction+' up');frames(2)
 assert cmd('debug level').split()[1]=='2',out
 out['digital_crossing']='PASS'
 for i in range(100):
  level=cmd('debug level').split();x,y=map(float,level[2:4])
  if x>=8:break
  direction='dpdown' if y<25 else 'dpright'
  cmd('pad '+direction+' down');frames(4);cmd('pad '+direction+' up');frames(2)
 frames(80)
 out['field_memory']=cmd('memory');out['monsters']=cmd('monsters');assert len(out['monsters'].splitlines())>1,out['monsters'];out['field_audio']=cmd('audio status');assert 'sfx_looped=0' in out['field_audio'];cmd('screenshot '+str(root/'outputs/prototype-0.2.11-digital-gate-mac.png'))
 cmd('quit');assert p.wait(timeout=15)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=15)
 log.close();Path(sock).unlink(missing_ok=True)
 (root/'work/digital-gate-031-result.json').write_text(json.dumps(out,indent=2)+chr(10))
print(json.dumps(out,indent=2))
