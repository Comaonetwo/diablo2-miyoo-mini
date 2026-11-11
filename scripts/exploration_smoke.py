#!/usr/bin/env python3
"""Exercise decoded tile residency across map positions; debug movement, not campaign play."""
import json,os,re,socket,subprocess,time,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1];data=Path(os.environ['D2_TEST_DATA']);out={'physical_console':False,'debug_positioning':True,'runs':[]}
for tag,binary in [('016',root/'work/baseline-0.2.16/d2-desktop'),('017',root/'work/build/probe/d2-desktop')]:
 user=root/f'work/runtime/explore-037-{tag}-{os.getpid()}';user.mkdir();(user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\ncompact_hud=1\nworld_big=1\n')
 sock=f'/tmp/d2-explore-{os.getpid()}.sock';log=open(root/f'work/explore-037-{tag}.log','w');env=dict(os.environ,D2D_USER_DIR=str(user),D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 def cmd(line):
  with socket.socket(socket.AF_UNIX) as s:
   s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());a=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
    b=s.recv(65536);assert b;a+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
   return a
 def frames(n=3):
  goal=int(cmd('info').split('frame=')[1].split()[0])+n
  while int(cmd('info').split('frame=')[1].split()[0])<goal:time.sleep(.01)
 p=subprocess.Popen([str(binary),'--headless','--no-video','--no-save','--data',str(data),'--start-screen','ingame','--start-class','0','--start-name','ExploreTest','--seed','892088571','--devctl',sock],env=env,stdout=log,stderr=log)
 run={'version':tag,'sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'samples':[]};out['runs'].append(run)
 try:
  t=time.monotonic()+120
  while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
  cmd('ping');frames(10)
  for level in (2,8,2,3,2):
   cmd(f'debug goto {level}');frames(6);dims=cmd('debug level').split();w,h=map(int,dims[4:6]);grid=cmd('debug grid').splitlines()[:-1]
   spots=[]
   for y in range(5,h-3,8):
    xs=range(5,w-3,8) if (y//8)%2 else reversed(range(5,w-3,8))
    for x in xs:
     spots.append((x+.5,y+.5))
   for i,(x,y) in enumerate(spots):
    cmd(f'debug warp {x} {y}');frames(3)
    m={k:int(v) for k,v in re.findall(r'(\w+)=(\d+)',cmd('memory'))};m.update(level=level,x=x,y=y);run['samples'].append(m)
    if tag=='017':assert m['loaded_level_tile_pixel_capacity']<=3*1024*1024,m
   (root/'work/exploration-037-result.json').write_text(json.dumps(out,indent=2)+'\n')
  run['peak_tile_bytes']=max(m['loaded_level_tile_pixel_capacity'] for m in run['samples']);run['last_heap_bytes']=run['samples'][-1].get('heap_in_use');cmd('quit');assert p.wait(timeout=20)==0;run['result']='PASS'
 finally:
  if p.poll() is None:p.terminate();p.wait(timeout=20)
  log.close();Path(sock).unlink(missing_ok=True);(root/'work/exploration-037-result.json').write_text(json.dumps(out,indent=2)+'\n')
assert out['runs'][1]['peak_tile_bytes']<=3*1024*1024;out['result']='PASS';(root/'work/exploration-037-result.json').write_text(json.dumps(out,indent=2)+'\n');print([(r['version'],len(r['samples']),r['peak_tile_bytes'],r['last_heap_bytes']) for r in out['runs']])
