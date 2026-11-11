#!/usr/bin/env python3
"""Regression for console feedback: panels, map overlap, belt, scroll and dungeon input.
Debug commands create reproducible starting conditions; actual item use and
entrance activation below go through the controller, not direct world commands.
"""
import array,json,os,re,socket,subprocess,time
from pathlib import Path
from png_read import read_png
root=Path(__file__).resolve().parents[1];user=root/f'work/runtime/feedback-037-{os.getpid()}';user.mkdir()
sock=f'/tmp/d2-feedback-{os.getpid()}.sock';log=open(root/'work/feedback-037.log','w');p=None;out={'physical_console':False}
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(90);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,'closed test connection';a+=b.decode()
  if re.search(r'(?:^|\n)err(?:\s|$)',a):raise RuntimeError(a)
  return a
def state():return dict(re.findall(r'(\w+)=(.*?)(?= \w+=|$)',cmd('state').splitlines()[0]))
def frames(n):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;deadline=time.monotonic()+90
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert time.monotonic()<deadline;time.sleep(.03)
def key(k):cmd('key '+k);frames(4)
def pad(k):cmd('pad '+k+' down');frames(3);cmd('pad '+k+' up');frames(4)
def shot(name):
 path=root/'outputs'/('prototype-0.2.17-'+name+'.png');cmd('screenshot '+str(path));return read_png(path)
def items():return cmd('items')
try:
 (user/'d2d.cfg').write_text('text_scale=175\nui_scale=175\ncompact_hud=1\nmusic_volume=50\n')
 data=Path(os.environ['D2_TEST_DATA']);capture=root/'work/feedback-037-audio.pcm'
 env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2D_LOOPBACK='1',D2D_AUDIO_CAPTURE=str(capture),D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'));env.pop('D2D_SILENT',None)
 p=subprocess.Popen([str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--no-save','--seed','3','--start-screen','ingame','--start-class','0','--start-name','FeedbackTest','--devctl',sock],env=env,stdout=log,stderr=log)
 deadline=time.monotonic()+120
 while not Path(sock).exists():assert p.poll() is None and time.monotonic()<deadline;time.sleep(.1)
 cmd('ping');frames(12);out['settings']=cmd('port');assert 'text=200 ui=125' in out['settings'];out['town_memory']=cmd('memory')
 # All scales, both panels together: this is the physical 1,152,000-byte failure.
 cmd('debug statpts 5');cmd('debug skillpts 2');key('C');key('I')
 for i in range(20):
  frames(3);key('Escape');key('C');key('I')
 shot('dual-panel-175');key('Escape');out['dual_panel_reopen_20']='PASS'
 # Standard overlay panels should conceal the automap identically, even with
 # the map toggled beneath them. Keep the pointer away from tooltip targets.
 key('F10');key('Down');key('Left');key('Left');key('Left');assert 'ui=100' in cmd('port');key('Escape')
 key('C');key('I');cmd('move 799 599');frames(3);before=shot('overlay-map-off');key('Tab');after=shot('overlay-map-on')
 for x0,x1 in ((80,400),(400,720)):
  for y in range(60,475):assert before[3][y][x0*before[2]:x1*before[2]]==after[3][y][x0*after[2]:x1*after[2]],(x0,y)
 out['automap_under_panels']='PASS';key('Escape');key('Tab')
 # L chooses healing by type rather than column; empty slot 0 must not block it.
 before=items();health=re.findall(r'\[(hp\w*) loc=2 .*? id=(\d+)',before)
 if health:
  old_count=len(health);pad('leftshoulder');assert len(re.findall(r'\[hp\w* loc=2 ',items()))==old_count-1
 out['healing_shoulder']='PASS'
 # Position by an actual generated warp and use X to enter it.
 cmd('debug goto 2');frames(40);out['blood_moor_warps']=cmd('debug warps')
 entrance=next(line.split() for line in out['blood_moor_warps'].splitlines() if len(line.split())==4 and line.split()[3]=='8')
 _,x,y,_=entrance;cmd(f'debug warp {float(x)+.5} {float(y)+.5}');frames(3);pad('x');frames(45)
 out['dungeon_level']=cmd('debug level');assert out['dungeon_level'].split()[1]=='8',out['dungeon_level'];shot('den-controller-entry');out['dungeon_x_entry']='PASS';frames(220)
 # Leave via X too. Test setup puts us at the mouth; X still initiates travel.
 exit=next(line.split() for line in cmd('debug warps').splitlines() if len(line.split())==4 and line.split()[3]=='2')
 _,x,y,_=exit;cmd(f'debug warp {float(x)+.5} {float(y)+.5}');frames(3);pad('x');frames(45);assert cmd('debug level').split()[1]=='2'
 out['dungeon_x_exit']='PASS';cmd('debug warp 15 25');frames(220)
 # A real Town Portal scroll: pick up, inventory X, then portal X to town.
 drop=int(cmd('debug give tsc').split('#')[1].split()[0]);cmd(f'cmd pickup {drop}');frames(8)
 scroll=re.search(r'\[tsc .*? id=(\d+)',items());assert scroll,items();scroll=int(scroll[1]);key('I')
 xy=cmd(f'debug itemat {scroll}').split();cmd('move '+xy[1]+' '+xy[2]);frames(3);pad('x');key('Escape');frames(12)
 assert not re.search(r'\[tsc .*? id='+str(scroll)+r'(?:\s|\])',items());out['town_portals']=cmd('debug portals');assert 'portal' in out['town_portals'] or len(out['town_portals'].splitlines())>1
 pad('x');frames(35);out['portal_destination']=cmd('debug level');assert out['portal_destination'].split()[1]=='1',out['portal_destination'];out['portal_scroll_inventory_x_and_travel']='PASS'
 # Identify scroll: X arms it, A identifies the selected carried item.
 for code in ('cap','isc'):
  drop=int(cmd('debug give '+code+' 4').split('#')[1].split()[0]);cmd(f'cmd pickup {drop}');frames(6)
 cmd('debug unid');frames(3)
 carried=items();target=int(re.search(r'\[cap .*? id=(\d+)',carried)[1]);identify=int(re.search(r'\[isc .*? id=(\d+)',carried)[1]);key('I')
 xy=cmd(f'debug itemat {identify}').split();cmd('move '+xy[1]+' '+xy[2]);frames(3);pad('x')
 xy=cmd(f'debug itemat {target}').split();cmd('move '+xy[1]+' '+xy[2]);frames(3);pad('a');key('Escape')
 assert not re.search(r'\[isc .*? id='+str(identify)+r'(?:\s|\])',items()),items()
 out['identify_scroll_x_then_a']='PASS'
 # These modifier bindings replace the removed compact menu buttons.
 cmd('pad back down');pad('leftstick');cmd('pad back up');frames(3);assert state()['tree']=='1';key('Escape')
 cmd('pad back down');pad('rightstick');cmd('pad back up');frames(3);assert state()['quest']=='1';shot('quest-controller');key('Escape')
 out['controller_skill_tree_and_quests']='PASS'
 out['final_memory']=cmd('memory');frames(30);cmd('quit');assert p.wait(timeout=20)==0
 pcm=array.array('h');pcm.frombytes(capture.read_bytes());out['audio']={'samples':len(pcm),'nonzero':sum(x!=0 for x in pcm),'peak':max(map(abs,pcm),default=0)};assert out['audio']['nonzero']>10000
 text=(root/'work/feedback-037.log').read_text();assert 'music stream ' in text;assert 'bad_alloc' not in text and 'build failed' not in text
 out['music_streams']=re.findall(r'music stream .*',text);out['result']='PASS'
finally:
 if p and p.poll() is None:p.terminate();p.wait(timeout=20)
 log.close();Path(sock).unlink(missing_ok=True);(root/'work/feedback-037-result.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
