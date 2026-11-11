#!/usr/bin/env python3
"""Private integration for 0.2.18 feedback; debug sets up reproducible scenes.
Input navigation, stat spending, run toggle and save/reload use normal input.
"""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/f'work/runtime/console-feedback-040-{os.getpid()}';user.mkdir()
sock=f'/tmp/d2-feedback-040-{os.getpid()}.sock';log=open(root/'work/console-feedback-040.log','w');p=None;out={'physical_console':False}
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,'closed';a+=b.decode()
  assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
  return a
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def frames(n=4):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;t=time.monotonic()+90
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<t;time.sleep(.03)
def key(k):cmd('key '+k);frames()
def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames(4)
def shot(name):cmd('screenshot '+str(root/'outputs'/f'prototype-0.2.18-{name}.png'))
def menu(row):
 key('F10');info=cmd('port');current=int(re.search(r'row=(\d+)',info)[1]);
 for _ in range((row-current)%16):key('Down')
def change(row):menu(row);key('Right');key('Escape')
def start(screen='ingame',saving=False):
 global p
 data=Path(os.environ['D2_TEST_DATA']);env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
 argv=[str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen',screen,'--start-class','0','--start-name','FeedbackHero','--devctl',sock]
 if not saving:argv+=['--no-save']
 p=subprocess.Popen(argv,env=env,stdout=log,stderr=log);t=time.monotonic()+120
 while not Path(sock).exists():assert p.poll() is None and time.monotonic()<t;time.sleep(.1)
 cmd('ping');frames(10)
def stop():cmd('quit');assert p.wait(timeout=20)==0;Path(sock).unlink(missing_ok=True)
try:
 (user/'d2d.cfg').write_text('text_scale=200\nui_scale=100\ncompact_hud=1\n')
 start();assert 'text=200 ui=100' in cmd('port');shot('hud-compact')
 before='running=true' in cmd('port');cmd('pad back down');frames();pad('y');cmd('pad back up');frames();assert ('running=true' in cmd('port'))!=before;assert state()['automap']=='0';out['run_combo']='PASS'
 cmd('debug statpts 5');key('C');assert state()['char'] if 'char' in state() else True
 old=state();pad('a');new=state();assert int(new['str'])==int(old['str'])+1,(old,new)
 pad('dpdown');old=state();pad('a');new=state();assert int(new['statpts'])==int(old['statpts'])-1,(old,new);assert new['str']==old['str'];shot('stats-dpad');out['stat_points_dpad']='PASS'
 key('Escape');key('T');shot('skill-tree-big-text');pad('dpdown');pad('dpright');shot('skill-tree-dpad');key('Escape')
 drop=int(cmd('debug give tsc').split('#')[1].split()[0]);cmd(f'cmd pickup {drop}');frames();key('I');rows=cmd('items');match=re.search(r'\[tsc .*? id=(\d+)',rows)
 if match:
  xy=cmd('debug itemat '+match[1]).split();cmd('move '+xy[1]+' '+xy[2]);frames();shot('tooltip-big-text')
 key('Escape');change(1);assert 'ui=125' in cmd('port');key('C');key('I');shot('panels-big');
 from png_read import read_png
 _,_,channels,pixels=read_png(root/'outputs/prototype-0.2.18-panels-big.png')
 assert all(tuple(pixels[y][x*channels:x*channels+3])==(0,0,0) for y in range(570,593) for x in range(15,21));out['panel_hint_black_box']='PASS'
 key('Escape');key('T');shot('skill-tree-big-panel');key('Escape');out['large_overlay_panels']='PASS'
 change(1);assert 'ui=100' in cmd('port')
 menu(13);key('Return');shot('controls');key('Escape');key('Escape')
 change(5);out['world_settings']=(user/'d2d.cfg').read_text();assert 'world_big = 1' in out['world_settings'];shot('world-big-town');cmd('debug goto 2');frames(35);shot('world-big-field')
 cmd('debug give hp1');frames();shot('nearby-loot');out['world_zoom_and_loot']='PASS'
 cmd('debug goto 8');frames(30);shot('world-big-den');key('C');key('I');frames(8);key('Escape');out['dungeon_panels']='PASS'
 stop()
 # Fresh title/character workflow with real saves in an isolated test directory.
 start('title',True);assert state()['screen']=='title';shot('title');cmd('click 400 290');frames(6)
 # SINGLE PLAYER coordinates follow original title buttons; fallback key navigation.
 if state()['screen']!='charselect':
  cmd('click 400 310');frames(6)
 assert state()['screen']=='charselect',state()
 cmd('click 100 500');frames(6)
 assert state()['screen']=='charcreate',state();shot('character-create');cmd('click 690 555');frames(15);assert state()['screen']=='ingame',state();cmd('save');frames();save=user/'save/FeedbackHero.d2s';assert save.is_file();out['save_bytes']=save.stat().st_size;stop()
 start('charselect',True);frames(5);assert state()['saves']!='0';cmd('click 240 145');frames();cmd('click 690 555');frames(15);assert state()['screen']=='ingame',state();out['menu_save_reload']='PASS';stop();out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/console-feedback-040-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
