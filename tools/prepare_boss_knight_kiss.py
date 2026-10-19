"""Original PLAYER root kiss label (sprite1686), four selected knights."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
for color,variant in [('blue',3),('orange',5),('red',4),('green',2)]:
 r=E.parse(R/'assets/extracted/player.xml').getroot();ts=r.find('tags')
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
  elif t.get('type')=='DefineSpriteTag':
   actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in t.find('subTags') if a.get('type')=='DoActionTag')
   if any(('f_'+s+'\0').encode() in actions for s in ['Helmet','Belt','Shield']):freeze(t,variant)
   elif any(('f_'+s+'\0').encode() in actions for s in ['Weapon','Wings','Flag']):freeze(t,1)
   elif t.get('spriteId')=='371':freeze(t,2)
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='1686',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
 for _ in range(86):E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount','86')
 for k,v in [('Xmin',-10240),('Xmax',10240),('Ymin',-12800),('Ymax',2560)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/boss-actor-kiss-{color}.xml')
p=R/'assets/boss-actor-spec.json';spec=json.loads(p.read_text())[:13];spec.extend([['kiss-'+c,175,86,[1,33,65,69,73,77,81,85]] for c in ['blue','orange','red','green']]);p.write_text(json.dumps(spec,indent=2))
