"""Original horse walk, four mounted knights, and sleeping Barbarian pose."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1];spec=[]
for name,source,sid,count,variant in [('horse','horse',15,8,0),('rider-blue','player',1026,8,3),('rider-orange','player',1026,8,5),('rider-red','player',1026,8,4),('rider-green','player',1026,8,2),('sleep','player',1886,60,7)]:
 r=E.parse(R/f'assets/extracted/{source}.xml').getroot();ts=r.find('tags')
 for s in ts:
  if s.get('type')!='DefineSpriteTag':continue
  actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in s.find('subTags') if a.get('type')=='DoActionTag')
  if any(('f_'+key+'\0').encode() in actions for key in ('Helmet','Belt','Emblem','Shield')):freeze(s,variant)
  elif b'f_Weapon\0' in actions:freeze(s,10 if name=='sleep' else 1)
  elif any(('f_'+key+'\0').encode() in actions for key in ('Wings','Flag')):freeze(s,1)
  elif source=='player' and s.get('spriteId')=='371':freeze(s,2)
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
 E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0')
 for _ in range(count):E.SubElement(ts,'item',type='ShowFrameTag')
 for k,v in dict(Xmin=-3840,Xmax=3840,Ymin=-3520,Ymax=3200 if name=='horse' else 1280).items():r.find('displayRect').set(k,str(v))
 r.set('frameCount',str(count));E.ElementTree(r).write(R/f'assets/extracted/war-live-{name}.xml');spec.append(dict(name=name,samples=[1,3,5,7] if name!='sleep' else [1,16,31,46],variant=variant))
(R/'assets/war-live-spec.json').write_text(json.dumps(spec,indent=2))
