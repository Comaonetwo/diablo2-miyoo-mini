#!/usr/bin/env python3
"""Private integration for 0.2.18 feedback; debug sets up reproducible scenes.
Input navigation, stat spending, run toggle and save/reload use normal input.
"""
import json,os,re,socket,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];user=root/f'work/runtime/options-feedback-040-{os.getpid()}';user.mkdir()
sock=f'/tmp/d2-feedback-040-{os.getpid()}.sock';log=open(root/'work/options-feedback-040.log','w');p=None;out={'physical_console':False}
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
 (user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\ncompact_hud=1\nworld_big=0\nmap_opacity=100\n')
 start('ingame',True)
 key('Escape');cmd('click 400 210');frames();cmd('click 400 215');frames();shot('native-video')
 # Video: top=80, resolution row1: 80+45+8+22=155.
 cmd('click 400 155');frames();out['cfg_after_resolution']=(user/'d2d.cfg').read_text();assert 'world_big = 1' in out['cfg_after_resolution'],out
 cmd('click 400 200');frames();out['cfg_after_light']=(user/'d2d.cfg').read_text();assert 'fast_render = 0' in out['cfg_after_light'],out
 key('Escape');key('F10');assert 'fast=0' in cmd('port');key('Escape')
 # Back to safe town, map stays frozen while port menu cycles opacity.
 cmd('debug automap');frames();key('Tab');frames();shot('map-100')
 for expected in (25,50,75,100):
  change(7);text=(user/'d2d.cfg').read_text();assert f'map_opacity = {expected}' in text,text;shot('map-'+str(expected))
 menu(13);key('Return');shot('controls');key('Escape');key('Escape')
 cmd('key C');cmd('key I');frames(5);shot('panels-final');key('Escape')
 # First-party roster screen at Big; save/reload still uses normal exit.
 cmd('save');key('Escape');cmd('click 400 260');frames(10);assert state()['screen']=='charselect';shot('character-select')
 cmd('quit');assert p.wait(timeout=20)==0;out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/options-feedback-040-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
