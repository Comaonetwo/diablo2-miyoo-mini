#!/usr/bin/env python3
"""Build isolated QA fixtures through the engine's own commands/save writer."""
from pathlib import Path
import os,re,json,socket,subprocess,time,shutil,hashlib
root=Path(__file__).resolve().parents[1];out=root/'outputs/test-saves-0.2.25';out.mkdir(exist_ok=True);logs=root/'work/release-048';data=root/'work/game-data'
heroes=[
 dict(name='QAStart',ui=0,level=5,town=1,target=3,weapon='hax',right='Bash'),
 dict(name='QAActOne',ui=2,level=18,town=1,target=37,weapon='scm',right='Holy Fire'),
 dict(name='QAActTwo',ui=4,level=30,town=40,target=-2,weapon='lst',right='Fire Ball',parts=['msf','vip'],merc=True),
 dict(name='QAActThree',ui=1,level=35,town=75,target=83,weapon='wnd',right='Raise Skeleton',parts=['qey','qhr','qbr','qf1'],merc=True),
 dict(name='QAActFour',ui=0,level=70,town=103,target=108,weapon='7cr',right='Berserk',diablo=True,power=True),
 dict(name='QADuriel',ui=3,level=35,town=40,target=73,weapon='jav',right='Power Strike',merc=True),
 dict(name='QAMephisto',ui=4,level=45,town=75,target=102,weapon='lst',right='Fire Ball',merc=True),
 dict(name='QADruid',ui=5,level=35,town=40,target=3,weapon='clb',right='Summon Spirit Wolf',merc=True),
 dict(name='QAAssassin',ui=6,level=35,town=75,target=83,weapon='ktr',right='Fire Blast',merc=True),
]
# Keep each route in its act; the druid's field route uses the Sewers.
heroes[7]['target']=47
only=os.environ.get('D2_QA_ONLY');results=[]
for hero in heroes:
 if only and hero['name']!=only:continue
 name=hero['name'];user=root/f'work/runtime/qa-{name}-{os.getpid()}';user.mkdir();(user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\nreadable_font=1\nsharp_panel_text=1\ncompact_hud=1\n');sock=f'/tmp/d2qa-{name}-{os.getpid()}.sock';log=(logs/(name+'.log')).open('w');p=None
 def cmd(line):
  with socket.socket(socket.AF_UNIX)as s:
   s.settimeout(180);s.connect(sock);s.sendall((line+'\n').encode());v=''
   while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',v):
    b=s.recv(65536);assert b,(name,line);v+=b.decode()
   assert not re.search(r'(?:^|\n)err(?:\s|$)',v),(name,line,v)
   return v
 def frames(n=4):
  target=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+180
  while int(cmd('info').split('frame=')[1].split()[0])<target:assert time.monotonic()<end;time.sleep(.01)
 def run(screen):
  global p
  env=dict(os.environ,D2D_USER_DIR=str(user),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2_PATCH_INSTALLER=str(data/'LODPatch_114d.exe'))
  args=[str(root/'work/build/probe/d2-desktop'),'--headless','--data',str(data),'--no-video','--seed','3','--start-screen',screen,'--devctl',sock]
  if screen=='ingame':args+=['--start-class',str(hero['ui']),'--start-name',name]
  p=subprocess.Popen(args,env=env,stdout=log,stderr=log);end=time.monotonic()+180
  while not Path(sock).exists():assert p.poll()is None and time.monotonic()<end,(name,'startup');time.sleep(.05)
  cmd('ping');frames(8)
 def key(k):cmd('key '+k);frames()
 def prop(slot,stat,val):cmd(f'debug qa prop {slot} {stat} {val}');frames(2)
 try:
  run('ingame');cmd('debug qa clear');cmd(f"debug qa level {hero['level']}");frames()
  power=hero.get('power',False);lvl=hero['level']
  for stat,val in [(0,250 if power else 70),(1,80),(2,160 if power else 65),(3,200 if power else 65),(7,(4000 if power else 500 if lvl>5 else 120)*256),(6,(4000 if power else 500 if lvl>5 else 120)*256),(9,(1500 if power else 350 if lvl>5 else 60)*256),(8,(1500 if power else 350 if lvl>5 else 60)*256),(11,200*256),(10,200*256),(14,40000),(15,100000)]:cmd(f'debug stat {stat} {val}')
  for slot,code in [(1,'cap'),(3,'ful'if power else'ltp'if lvl>5 else'qui'),(4,hero['weapon']),(8,'hbl'if lvl>5 else'lbl'),(9,'lbt'),(10,'lgl')]:cmd(f'debug equip {code} {slot}')
  frames()
  if lvl>5:
   prop(1,127,2 if not power else 3)
   for stat in (39,41,43,45):prop(3,stat,75 if power else 30)
  if power:
   for stat,val in [(21,60),(22,120),(17,200),(18,200),(19,900),(93,60),(60,10)]:prop(4,stat,val)
   for stat,val in [(7,300),(76,40),(36,40),(34,30),(35,30),(31,400)]:prop(3,stat,val)
   prop(1,7,300);prop(9,99,60)
  skills=[];spent=0
  for line in cmd('debug qa skills').splitlines():
   if '\t'not in line:continue
   id,page,req,label=line.split('\t');id=int(id);req=int(req)
   if req<=lvl and spent<lvl-1:cmd(f'debug points {id} 1');spent+=1;skills.append(dict(id=id,page=int(page),name=label,points=1))
  if power:
   for sk in skills:
    if sk['name']in('Berserk','Sword Mastery'):cmd(f"debug points {sk['id']} 20");spent+=19;sk['points']=20
  cmd('debug skillpts '+str(max(0,lvl-1-spent)));cmd('debug statpts 5')
  desired=next((sk for sk in skills if sk['name']==hero['right']),skills[0]);cmd(f"debug skill right {desired['id']}")
  for code in hero.get('parts',[]):cmd('debug qa inventory '+code)
  for code in ['hp3','hp3','mp3','mp3','vps','yps','wms','tsc','tsc','isc']:cmd('debug qa inventory '+code)
  if lvl>5:
   cmd('debug qa stash lbl');cmd('debug qa stash mbl')
   for code in ['hp5']*8+['mp5']*8+['rvl']*4:cmd('debug qa stash '+code)
   cmd('debug qa stash box')
   for code in ['gcr']*3+['gcb','r01','r03']:cmd('debug qa stash '+code)

  if hero['town']>1:
   cmd('debug quest 6')
   if hero['town']>=75:cmd('debug quest 14')
   if hero['town']>=103:cmd('debug quest 22')
   cmd('debug goto '+str(hero['town']));frames(16)
  if hero.get('merc'):cmd('debug qa merc');frames()
  for wp in (0,9,18,27):
   if (wp==0 or wp==9 and hero['town']>=40 or wp==18 and hero['town']>=75 or wp==27 and hero['town']>=103):cmd('debug wp '+str(wp))
  for box in range(8 if lvl==5 else 16):cmd('debug qa belt '+('vps'if box==2 else'rvl'if box==3 else'hp3'if box%2==0 else'mp3')+' '+str(box))
  frames(8);cmd('save');before=cmd('items');assert (user/'save'/f'{name}.d2s').is_file();baseline_bytes=(user/'save'/f'{name}.d2s').read_bytes();cmd('quit');assert p.wait(timeout=30)==0;Path(sock).unlink(missing_ok=True)
  # This is a separate explicit test scenario, not a claim that .d2s persists portals.
  (user/'save'/f'{name}.qa').write_text(f"D2QA1 {hero['target']} {int(hero.get('diablo',False))}\n")
  run('charselect');cmd('click 690 555');frames(40);state=cmd('state');assert 'screen=ingame'in state and f'name={name}'in state,state;assert f'level={lvl} 'in state,state
  assert int(cmd('debug level').split()[1])==hero['town'];portals=cmd('debug portals');assert '0\t'in portals and '1\t'in portals,(name,portals)
  unbuilt=cmd('debug unbuilt');after=cmd('items');assert after.count('[')==before.count('['),(name,before,after)
  for code in hero.get('parts',[]):assert '['+code+' loc=0'in after,(name,code,after)
  if lvl>5:assert '[box loc=0 slot=0'in after and 'panel=5'in after
  # Enter and return through the real digital interaction, not debug goto.
  twin=next(l.split('\t')for l in portals.splitlines()if l.startswith('1\t'));cmd('debug warp '+twin[2]+' '+twin[3]);frames(6);cmd('pad x down');frames(4);cmd('pad x up');frames(35)
  loaded_level=int(cmd('debug level').split()[1]);assert loaded_level!=hero['town'],(name,cmd('debug level'),portals)
  boss_list=cmd('monsters');
  for boss_name in ({37:'Andariel',73:'Duriel',102:'Mephisto',108:'Diablo'}.get(loaded_level,''),):
   if boss_name:
    end_boss=time.monotonic()+15
    while boss_name.lower()not in boss_list.lower():assert time.monotonic()<end_boss,(name,loaded_level,boss_list);frames(12);boss_list=cmd('monsters')
  cmd('screenshot '+str(logs/f'{name}-portal.png'))
  # Return before nearby enemies can make the baseline save unsafe.
  home=next(l.split('\t')for l in cmd('debug portals').splitlines()if l.startswith('0\t'));cmd('debug warp '+home[2]+' '+home[3]);frames(4);cmd('pad x down');frames(3);cmd('pad x up');frames(20);end_home=time.monotonic()+15
  while int(cmd('debug level').split()[1])!=hero['town']:
   assert time.monotonic()<end_home,(name,cmd('state'),cmd('debug level'),cmd('debug portals'))
   cmd('debug warp '+home[2]+' '+home[3]);frames(4);cmd('pad x down');frames(3);cmd('pad x up');frames(20)
  cmd('quit');assert p.wait(timeout=30)==0
  # Export the initial baseline, not the validation run's combat/quest mutations.
  backup=user/'save'/f'{name}.d2s.bak';baseline=backup if backup.exists()else user/'save'/f'{name}.d2s'
  dest=out/'save';dest.mkdir(exist_ok=True);(dest/f'{name}.d2s').write_bytes(baseline_bytes);shutil.copyfile(user/'save'/f'{name}.qa',dest/f'{name}.qa')
  row=dict(hero,unbuilt=unbuilt,skills=skills,initial_state=state,portal_destination=loaded_level,portal_roundtrip='PASS',items=after,monsters=boss_list,sha256=hashlib.sha256((dest/f'{name}.d2s').read_bytes()).hexdigest());results.append(row);(out/(name+'.json')).write_text(json.dumps(row,ensure_ascii=False,indent=2)+'\n');print(name,'PASS',loaded_level,flush=True)
 finally:
  if p and p.poll()is None:p.terminate();p.wait(timeout=30)
  Path(sock).unlink(missing_ok=True);log.close()
(out/'manifest.json').write_text(json.dumps(results,ensure_ascii=False,indent=2)+'\n')
