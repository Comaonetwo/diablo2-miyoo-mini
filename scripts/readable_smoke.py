#!/usr/bin/env python3
"""Integration and device-projected screenshots of the approved Readable font."""
import json,os,re,socket,subprocess,time,struct,zlib
from pathlib import Path
from png_read import read_png
root=Path(__file__).resolve().parents[1];output=root/'outputs/readable-040';output.mkdir(exist_ok=True);(root/'work/readable-040').mkdir(exist_ok=True);data=root/'work/game-data';out={'release_font_changed':True,'version':'0.2.18','runs':[]}
def physical(source,dest):
 w,h,ch,rows=read_png(source);assert (w,h)==(800,600)
 raw=b''.join(rows);assert ch==4
 projected=subprocess.run([str(root/'work/release-040/frame-export')],input=raw,stdout=subprocess.PIPE,check=True).stdout
 pixels=[projected[y*640*3:(y+1)*640*3]for y in range(480)]
 def chunk(kind,bytes):return struct.pack('>I',len(bytes))+kind+bytes+struct.pack('>I',zlib.crc32(kind+bytes)&0xffffffff)
 dest.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',640,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\0'+row for row in pixels)))+chunk(b'IEND',b''))
for candidate in (False,True):
 tag='readable'if candidate else'original';user=root/f'work/runtime/readable-040-{tag}-{os.getpid()}';user.mkdir();(user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\ncompact_hud=1\nworld_big=0\nreadable_font='+('1'if candidate else'0')+'\nsmooth_frame=1\n')
 sock=f'/tmp/d2-font-{os.getpid()}.sock';log=open(root/f'work/readable-040/{tag}.log','w');p=None;row={'font':tag};out['runs'].append(row)
 def cmd(line):
  with socket.socket(socket.AF_UNIX) as s:
   s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
    b=s.recv(65536);assert b;a+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
   return a
 def frames(n=5):
  goal=int(cmd('info').split('frame=')[1].split()[0])+n;t=time.monotonic()+90
  while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<t;time.sleep(.02)
 def key(k):cmd('key '+k);frames()
 def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames()
 def shot(name):
  path=output/f'{tag}-{name}-800.png';cmd('screenshot '+str(path));physical(path,output/f'{tag}-{name}-640.png')
 try:
  env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
  p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen','ingame','--start-class','0','--start-name','Arkon','--devctl',sock],env=env,stdout=log,stderr=log)
  t=time.monotonic()+120
  while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
  cmd('ping');frames(12)
  assert ('font=readable'if candidate else'font=original')in cmd('port')
  key('F10')
  for _ in range(8):key('Down')
  shot('font-setting');key('Right');assert ('font=original'if candidate else'font=readable')in cmd('port');key('Right')
  key('Down');key('Right');assert 'smooth=0'in cmd('port');key('Right');assert 'smooth=1'in cmd('port');key('Escape')
  cfg=(user/'d2d.cfg').read_text().replace(' ','');assert ('readable_font='+('1'if candidate else'0'))in cfg and 'smooth_frame=1'in cfg
  row['option_toggle_persistence']='PASS' 
  for stat,value in [(0,113),(1,43),(2,63),(3,95),(12,12),(13,43100),(14,12345)]:cmd(f'debug stat {stat} {value}')
  cmd('debug statpts 5');frames(8);key('C');shot('stats');key('I');shot('stats-inventory');key('Escape');key('T');shot('skills');key('Escape')
  # Real town waypoint object, ordinary interaction + normal d-pad click transport.
  entries=cmd('debug presets').splitlines();record=next(line.split('\t')for line in entries if '\t119\t' in line or '\t398\t' in line);npc=int(record[0]);x,y=map(float,record[2:4]);cmd(f'debug warp {x+1} {y+1}');frames(5)
  for index in range(9):cmd(f'debug wp {index}')
  frames(550);cmd(f'cmd interact {npc}');frames(12);row['waypoint_open']=cmd('state');assert 'waypoint=9' in row['waypoint_open'],row
  shot('waypoints');pad('dpdown');shot('waypoint-focus');pad('a');frames(15);row['destination']=cmd('debug level');assert row['destination'].split()[1]=='3',row
  cmd('save');key('Escape');cmd('click 400 260');frames(12);assert 'screen=charselect' in cmd('state');shot('roster');cmd('quit');assert p.wait(timeout=20)==0
  Path(sock).unlink(missing_ok=True)
  p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--start-screen','title','--devctl',sock],env=env,stdout=log,stderr=log)
  t=time.monotonic()+120
  while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
  cmd('ping');frames(4);assert ('font=readable'if candidate else'font=original')in cmd('port');assert 'smooth=1'in cmd('port')
  row['option_restart']='PASS';cmd('quit');assert p.wait(timeout=20)==0;row['result']='PASS'
 finally:
  if p and p.poll() is None:p.terminate();p.wait(timeout=20)
  log.close();Path(sock).unlink(missing_ok=True);(root/'work/readable-040/result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
