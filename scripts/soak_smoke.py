#!/usr/bin/env python3
"""Repeated map/UI/session transitions with original data and isolated test saves."""
import array,json,os,re,socket,subprocess,time
from pathlib import Path
from png_read import read_png
root=Path(__file__).resolve().parents[1];user=root/f'work/runtime/soak-037-user-{os.getpid()}';user.mkdir(exist_ok=True)
sock=f'/tmp/d2-soak-{os.getpid()}.sock';log=open(root/'work/soak-037.log','w');p=None;out={'physical_console':False,'campaign_playthrough':False,'cycles':[]}
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,'closed test connection';a+=b.decode()
  if re.search(r'(?:^|\n)err(?:\s|$)',a):raise RuntimeError(a)
  return a
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def frames(n):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;t=time.monotonic()+90
 while int(cmd('info').split('frame=')[1].split()[0])<goal:
  assert time.monotonic()<t;time.sleep(.03)
def memory():return dict((k,int(v)) for k,v in re.findall(r'(\w+)=(\d+)',cmd('memory')))
try:
 (user/'d2d.cfg').write_text('text_scale=175\nui_scale=175\ncompact_hud=1\nmusic_volume=50\n')
 data=Path(os.environ['D2_TEST_DATA']);capture=root/'work/soak-037-audio.pcm'
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2D_LOOPBACK='1',D2D_AUDIO_CAPTURE=str(capture),D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'));env.pop('D2D_SILENT',None)
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen','charcreate','--start-class','4','--start-name','SoakTest','--devctl',sock],env=env,stdout=log,stderr=log)
 start=time.monotonic();t=start+120
 while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(5);cmd('click 690 555');frames(15);assert state()['screen']=='ingame'
 for cycle in range(12):
  record={'cycle':cycle+1,'start':memory()}
  for k,field in [('i','inv'),('t','tree'),('c','str'),('q','quest')]:
   cmd('key '+k);frames(8);cmd('key Escape');frames(5)
  # Visit maps through test setup, not a claim of completed campaign travel.
  level=[2,3,8][cycle%3];cmd(f'debug goto {level}');frames(90)
  record['level']=cmd('debug level');assert int(record['level'].split()[1])==level
  record['field_memory']=memory();record['monsters']=len(cmd('monsters').splitlines())-1
  if cycle<3:
   path=root/'outputs'/f'prototype-0.2.17-level-{level}.png';cmd('screenshot '+str(path))
   w,h,ch,pixels=read_png(path);visible=sum(any(row[x*ch:x*ch+3]) for row in pixels[:350] for x in range(w));assert visible>(5000 if level==8 else 40000),(level,visible);record['world_nonblack_pixels']=visible
  cmd('key Escape');frames(5);cmd('click 400 260');frames(12);assert state()['screen']=='charselect',state()
  record['after_exit']=memory();cmd('click 690 555');frames(18);assert state()['screen']=='ingame',state()
  record['after_reload']=memory();out['cycles'].append(record)
  (root/'work/soak-037-result.json').write_text(json.dumps(out,indent=2)+'\n')
 warm=out['cycles'][3]['after_exit']['heap_in_use'];last=out['cycles'][-1]['after_exit']['heap_in_use']
 out['exit_heap_growth_after_warmup_bytes']=last-warm;assert last-warm<4*1024*1024,(warm,last)
 cmd('quit');assert p.wait(timeout=20)==0
 samples=array.array('h');samples.frombytes(capture.read_bytes());nonzero=sum(x!=0 for x in samples);peak=max(map(abs,samples),default=0)
 out['audio']={'samples':len(samples),'nonzero':nonzero,'peak':peak,'speaker_tested':False};assert nonzero>10000 and peak>500
 out['elapsed_seconds']=round(time.monotonic()-start,2);out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/soak-037-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
