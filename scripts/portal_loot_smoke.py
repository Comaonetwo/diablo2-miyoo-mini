#!/usr/bin/env python3
"""Crowded Cave Level 2, full pack, actual scroll use and controller portal entry."""
import os,re,json,socket,subprocess,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];tag=os.environ.get('D2_TEST_TAG','portal041');user=root/f'work/runtime/{tag}-{os.getpid()}';user.mkdir();sock=f'/tmp/d2-{tag}-{os.getpid()}.sock';log=open(root/f'work/release-041/{tag}.log','w');result={'physical_console':False};p=None
def cmd(line):
 with socket.socket(socket.AF_UNIX)as s:
  s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());out=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',out):
   b=s.recv(65536);assert b;out+=b.decode()
  assert not re.search(r'(?:^|\n)err(?:\s|$)',out),(line,out)
  return out
def frames(n=4):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+180
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<end;time.sleep(.01)
def key(k):cmd('key '+k);frames()
def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames()
def give(code,pick=False):
 item=int(cmd('debug give '+code).split('#')[1].split()[0])
 if pick:cmd(f'cmd pickup {item}');frames(3)
 return item
try:
 (user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\nworld_big=1\ncompact_hud=1\nmap_opacity=75\n')
 data=root/'work/game-data';env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2D_LOOPBACK='1',D2D_AUDIO_CAPTURE=str(user/'audio.pcm'),D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1');env.pop('D2D_SILENT',None)
 binary=Path(os.environ.get('D2_TEST_BINARY',str(root/'work/build/probe/d2-desktop')))
 p=subprocess.Popen([str(binary),'--headless','--data',str(data),'--no-video','--no-save','--seed','892088571','--start-screen','ingame','--start-class','4','--start-name','PortalTest','--devctl',sock],env=env,stdout=log,stderr=log)
 end=time.monotonic()+180
 while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end;time.sleep(.1)
 cmd('ping');frames(10);cmd('debug goto 13');frames(12);cmd('debug kill 0');frames(10);cmd('debug warp 22.6 5.1');frames()
 give('tsc',True);items=cmd('items');scroll=int(re.search(r'\[tsc .*? id=(\d+)',items)[1]);key('I');xy=cmd(f'debug itemat {scroll}').split();cmd('move '+xy[1]+' '+xy[2]);frames();pad('x');key('Escape');frames(30);result['portal']=cmd('debug portals');assert result['portal'].startswith('0\t13\t'),result
 for _ in range(18):give('cap',True)
 for _ in range(30):give('buc')
 frames(20);result['items_before']=cmd('items');cmd('debug warp 22.6 5.1');frames()
 for _ in range(int(os.environ.get('D2_PICK_ATTEMPTS','8'))):
  pad('x');frames(12)
  if not os.environ.get('D2_EXPECT_BLOCKED')and cmd('debug level').split()[1]=='1':break
 result['after_x']=cmd('debug level');result['portal_entered']=result['after_x'].split()[1]=='1'
 if os.environ.get('D2_EXPECT_BLOCKED'):
  assert not result['portal_entered'];result['baseline_loot_blocks_portal']='CONFIRMED'
 else:
  assert result['portal_entered'],result['after_x']
  # Return through the town end; loot pile remains in the cave.
  portal=next(line.split()for line in cmd('debug portals').splitlines()if line.startswith('1\t'));cmd(f'debug warp {portal[2]} {portal[3]}');frames();pad('x');frames(30);assert cmd('debug level').split()[1]=='13'
 result['memory']=cmd('memory');frames(int(os.environ.get('D2_STRESS_FRAMES','100')));cmd('quit');assert p.wait(timeout=30)==0;result['result']='PASS'
finally:
 if p and p.poll()is None:p.terminate();p.wait(timeout=30)
 log.close();Path(sock).unlink(missing_ok=True);(root/f'work/release-041/{tag}-result.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
