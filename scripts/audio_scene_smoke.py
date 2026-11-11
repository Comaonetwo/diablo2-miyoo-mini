#!/usr/bin/env python3
"""D-pad crossing from camp through the gate, after test-only placement nearby."""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/'work/runtime/audio-scene-031-user';user.mkdir(exist_ok=True)
sock=f'/tmp/d2-launch-{os.getpid()}.sock';log=open(root/'work/audio-scene-031.log','w');p=None;out={'physical_console':False}
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
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_LOOPBACK='1',D2D_AUDIO_CAPTURE=str(root/'work/audio-scene-031.pcm'),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--no-save','--seed','3','--start-screen','ingame','--start-class','0','--start-name','MiyooTest','--devctl',sock],env=env,stdout=log,stderr=log)
 t=time.monotonic()+120
 while not Path(sock).exists():
  assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(8)
 out['day']=cmd('audio status')
 cmd('debug day 270');frames(8);out['night']=cmd('audio status')
 assert re.search(r'requested=(\d+)',out['day'])[1]!=re.search(r'requested=(\d+)',out['night'])[1],out
 cmd('debug day 90');frames(8);out['day_again']=cmd('audio status')
 assert re.search(r'requested=(\d+)',out['day'])[1]==re.search(r'requested=(\d+)',out['day_again'])[1],out
 out['rain_asset']=cmd('audio info 64');cmd('audio cue 64');out['cue_started']=cmd('audio status');assert 'sfx_looped=0' in out['cue_started'] and 'sfx=1' in out['cue_started'],out
 frames(380);out['cue_finished']=cmd('audio status');assert 'sfx=0' in out['cue_finished'] and 'sfx_bytes=0' in out['cue_finished'],out
 out['result']='PASS'
 cmd('quit');assert p.wait(timeout=15)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=15)
 log.close();Path(sock).unlink(missing_ok=True)
 (root/'work/audio-scene-031-result.json').write_text(json.dumps(out,indent=2)+chr(10))
print(json.dumps(out,indent=2))
