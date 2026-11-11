#!/usr/bin/env python3
"""Cast Raise Skeleton and Revive through the digital pad on an isolated QA save."""
import os,re,socket,subprocess,time,json
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'work/runtime'/('corpse-skills-'+str(os.getpid()));user=out/'input-user';user.mkdir(parents=True,exist_ok=True);sock='/tmp/d2-corpse-'+str(os.getpid())+'.sock';Path(sock).unlink(missing_ok=True)
(user/'d2d.cfg').write_text('text_scale=200\nui_scale=125\nreadable_font=1\ncompact_hud=1\n')
import shutil
(user/'save').mkdir(exist_ok=True)
for ext in ('d2s','qa'):shutil.copyfile(root/'outputs/test-saves-0.2.25/save'/('QAActThree.'+ext),user/'save'/('QAActThree.'+ext))
log=open(out/'revive-input.log','w');env=dict(os.environ,D2D_USER_DIR=str(user.resolve()),D2D_DIGITAL_PAD='1',D2D_SILENT='1',D2D_SERIAL_LOADING='1',D2D_FAST_RENDER='1',D2_PATCH_INSTALLER=str(root/'work/game-data/LODPatch_114d.exe'),ASAN_OPTIONS='detect_leaks=0')
p=subprocess.Popen([os.environ.get('D2_TEST_BINARY',str(root/'work/build/sanitize/d2-desktop')),'--headless','--data',str(root/'work/game-data'),'--no-video','--seed','3','--start-screen','charselect','--devctl',sock],env=env,stdout=log,stderr=log)
def cmd(line):
 with socket.socket(socket.AF_UNIX) as s:
  s.settimeout(120);s.connect(sock);s.sendall((line+'\n').encode());a=''
  while not re.search(r'(?:^|\n)(ok|err)(?:\s|$)',a):
   b=s.recv(65536);assert b,(line,p.poll());a+=b.decode()
  assert not re.search(r'(?:^|\n)err(?:\s|$)',a),(line,a)
  return a
def frames(n=3):
 goal=int(cmd('info').split('frame=')[1].split()[0])+n;end=time.monotonic()+120
 while int(cmd('info').split('frame=')[1].split()[0])<goal:assert p.poll() is None and time.monotonic()<end;time.sleep(.03)
def key(k):cmd('key '+k);frames()
def shot(name):cmd('screenshot '+str(out.resolve()/(name+'.png')))
result={}
try:
 end=time.monotonic()+150
 while not Path(sock).exists():assert p.poll() is None and time.monotonic()<end;time.sleep(.1)
 frames();cmd('click 690 555');frames(20);assert 'name=QAActThree' in cmd('state')
 portal=next(x.split('\t')for x in cmd('debug portals').splitlines()if x.startswith('1\t'))
 cmd('debug warp '+portal[2]+' '+portal[3]);frames();cmd('pad x down');frames(3);cmd('pad x up');frames(20)
 assert int(cmd('debug level').split()[1])==83
 cmd('debug kill');frames(100)
 result['corpses']=cmd('monsters')
 def pets():return cmd('state').split('pets=')[1].split()[0]
 for skill in (70,95):
  before=pets();cmd('debug skill right '+str(skill));frames();assert ('rskill='+str(skill)+' ') in cmd('state')
  rows=[l.split('\t')for l in cmd('monsters').splitlines()if '\t' in l and l.split('\t')[6]=='DD']
  assert rows,result
  for row in rows[:20]:
   cmd('debug warp '+str(float(row[1])-.5)+' '+row[2]);frames(3)
   cmd('pad y down');frames(3);cmd('pad y up');frames(18)
   if len(pets().split(','))>(0 if before=='-' else len(before.split(','))):break
  assert len(pets().split(','))>(0 if before=='-' else len(before.split(','))),(skill,cmd('state'),result)
  result[str(skill)]={'before':before,'after':pets()}
 result['state']=cmd('state');cmd('quit');assert p.wait(timeout=30)==0;result['result']='PASS'
finally:
 if p.poll() is None:p.terminate();p.wait(timeout=30)
 log.close();Path(sock).unlink(missing_ok=True);(out/'revive-input.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items()if k!='corpses'},indent=2))
