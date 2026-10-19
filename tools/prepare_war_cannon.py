from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json,subprocess,os
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
spec=[('ball',560,1,[1]),('bomb',1068,1,[1]),('fire',1402,20,[1,5,10,15,20])]
for name,sid,count,keys in spec:
 r=E.parse(R/'assets/extracted/fx.xml').getroot();ts=r.find('tags')
 if name=='ball':freeze(next(s for s in ts if s.get('spriteId')==str(sid)),1)
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
 for _ in range(count):E.SubElement(ts,'item',type='ShowFrameTag')
 for k,v in dict(Xmin=-10240,Xmax=10240,Ymin=-8960,Ymax=3840).items():r.find('displayRect').set(k,str(v))
 r.set('frameCount',str(count));xml=R/f'assets/extracted/war-cannon-{name}.xml';swf=xml.with_suffix('.swf');E.ElementTree(r).write(xml)
 for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(R/f'assets/war-cannon-{name}'),str(swf)]]:
  p=subprocess.run(['java','-Xmx2g','-jar',jar]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1000:]
 print(name,'exported',flush=True)
(R/'assets/war-cannon-spec.json').write_text(json.dumps(spec))
