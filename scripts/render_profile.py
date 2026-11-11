#!/usr/bin/env python3
from pathlib import Path
import os,socket,subprocess,time,json,re,shutil
import argparse
r=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser(description='Paired full-frame pixel equivalence and timing on isolated QA save copies. Desktop only; not console FPS.')
ap.add_argument('--output',type=Path,required=True)
ap.add_argument('--binary',type=Path,default=r/'work/build/probe/d2-desktop')
ap.add_argument('--mode',choices=['compare-tiles','compare-shadows'],default='compare-tiles')
ap.add_argument('--heroes',default='QAActTwo,QAActThree,QAActFour')
ap.add_argument('--samples',type=int,default=3)
ap.add_argument('--frames',type=int,default=80)
a=ap.parse_args();assert 1<=a.samples<=20 and 10<=a.frames<=500
outpath=a.output.resolve();outpath.parent.mkdir(parents=True,exist_ok=True);binary=a.binary.resolve();rows=[]
for hero in a.heroes.split(','):
 user=r/f'work/runtime/profile058-{hero}-{os.getpid()}';(user/'save').mkdir(parents=True);sock=f'/tmp/d2-profile058-{os.getpid()}.sock';log=(outpath.parent/f'{outpath.stem}-{hero}.log').open('w')
 for ext in ['d2s','qa']:shutil.copyfile(r/f'outputs/test-saves-0.2.25/save/{hero}.{ext}',user/f'save/{hero}.{ext}')
 (user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\ncompact_hud=1\nworld_big=1\nreadable_font=1\nsharp_panel_text=1\n')
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_SILENT='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(r/'work/game-data/LODPatch_114d.exe'))
 p=subprocess.Popen([str(binary),'--headless','--no-video','--data',str(r/'work/game-data'),'--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log)
 def cmd(x):
  with socket.socket(socket.AF_UNIX)as s:
   s.settimeout(180);s.connect(sock);s.sendall((x+'\n').encode());a=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
    b=s.recv(65536);assert b;a+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(x,a)
   return a
 def frames(n=5):
  goal=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+180
  while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<end;time.sleep(.01)
 try:
  end=time.monotonic()+180
  while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.05)
  frames();cmd('click 690 555');frames(12);cmd('debug stat 7 25600000');cmd('debug stat 6 25600000');frames()
  for place in ['town','field','pack']:
   if place=='field':
    twin=next(x.split('\t')for x in cmd('debug portals').splitlines()if x.startswith('1\t'));cmd('debug warp '+twin[2]+' '+twin[3]);frames();cmd('pad x down');frames(3);cmd('pad x up');frames(16)
   if place=='pack':
    import math
    for scan in range(2):
     monsters=[x.split('\t')for x in cmd('monsters').splitlines()if '\t'in x]
     alive=[x for x in monsters if len(x)>6 and int(x[5].split('/')[0])>0]
     if alive:
      chosen=max(alive,key=lambda x:sum(math.hypot(float(x[1])-float(y[1]),float(x[2])-float(y[2]))<6 for y in alive))
      cmd('debug warp '+chosen[1]+' '+chosen[2]);frames(8)
   samples=[]
   for _ in range(a.samples):samples.append({k:int(v)for k,v in re.findall(r'(\w+)=(\d+)',cmd(f'benchmark {a.frames} '+a.mode))})
   row={'hero':hero,'place':place,'level':cmd('debug level'),'state':cmd('state'),'samples':samples};rows.append(row)
   import statistics
   med={k:statistics.median(x[k]for x in samples)for k in samples[0]};row['median']=med
   print(hero,place,'total',med['current_total_us'],'shadow pixels%',round(100*med['current_shadow_pixels_us']/med['current_total_us'],2),'whole shadow%',round(100*med['current_shadows_us']/med['current_total_us'],2),'decode%',round(100*med['current_decode_us']/med['current_total_us'],2),'paired speedup',med.get('paired_speedup_milli',0)/1000,'decodes',med['current_decodes'],'units',med['current_units'],flush=True)
  cmd('quit');assert p.wait(timeout=30)==0
 finally:
  outpath.write_text(json.dumps({'rows':rows},indent=2)+'\n')
  if p.poll()is None:p.terminate();p.wait(timeout=30)
  log.close();Path(sock).unlink(missing_ok=True)
outpath.write_text(json.dumps({'scope':'Desktop frozen frame, excludes simulation/audio/present; alternating old/new pairs; identical clock/weather; not console FPS','comparison':a.mode,'physical_console':False,'rows':rows},indent=2)+'\n')
