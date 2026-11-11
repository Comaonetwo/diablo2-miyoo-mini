#!/usr/bin/env python3
"""Compare repeatable desktop render workloads. Not a console FPS estimate."""
import json,os,re,socket,subprocess,time,statistics
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=Path(os.environ['D2_TEST_DATA']);out={'physical_console':False,'scope':'desktop frozen-frame rendering, excludes simulation, audio and scanout','runs':[]}
for tag,binary in [('0.2.15',root/'work/baseline-0.2.15/d2-desktop'),('0.2.16',root/'work/build/probe/d2-desktop')]:
 user=root/f'work/runtime/render-bench-036-{tag}-{os.getpid()}';user.mkdir();(user/'d2d.cfg').write_text('text_scale=200\nui_scale=100\ncompact_hud=1\nloot_near=1\nworld_big=0\n')
 sock=f'/tmp/d2-render-{os.getpid()}.sock';log=open(root/f'work/render-bench-036-{tag}.log','w');env=dict(os.environ,D2D_USER_DIR=str(user),D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 def cmd(line):
  with socket.socket(socket.AF_UNIX) as s:
   s.settimeout(180);s.connect(sock);s.sendall((line+'\n').encode());a=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
    b=s.recv(65536);assert b;a+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
   return a
 def frames(n=8):
  goal=int(cmd('info').split('frame=')[1].split()[0])+n
  while int(cmd('info').split('frame=')[1].split()[0])<goal:time.sleep(.02)
 p=subprocess.Popen([str(binary),'--headless','--no-video','--no-save','--data',str(data),'--start-screen','ingame','--start-class','0','--start-name','RenderBench','--seed','3','--devctl',sock],env=env,stdout=log,stderr=log)
 try:
  t=time.monotonic()+120
  while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
  cmd('ping');frames(15)
  for level in (1,2,8,3):
   if level!=1:cmd(f'debug goto {level}')
   frames(15);row={'version':tag,'level':level,'samples':[]}
   for _ in range(7):row['samples'].append({k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',cmd('benchmark 200'))})
   row['median_us']={k:statistics.median(s[k] for s in row['samples']) for k in ('total_us','light_us','scene_us','hud_us')};out['runs'].append(row)
  cmd('quit');assert p.wait(timeout=20)==0
 finally:
  if p.poll() is None:p.terminate();p.wait(timeout=20)
  log.close();Path(sock).unlink(missing_ok=True)
for level in (1,2,8,3):
 old,new=[x for x in out['runs'] if x['level']==level];new['speedup_over_015']=old['median_us']['total_us']/new['median_us']['total_us']
(root/'work/render-bench-036-result.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
