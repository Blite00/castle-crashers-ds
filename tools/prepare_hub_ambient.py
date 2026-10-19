"""Original LEVEL14 smith hammer loop, one source key every three ticks."""
from pathlib import Path
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level14-audit.xml').getroot();ts=r.find('tags')
for s in ts:
 if s.get('type')=='DefineSpriteTag' and s.get('spriteId') not in ('72','73'):freeze(s,1)
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='73',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0')
for _ in range(27):E.SubElement(ts,'item',type='ShowFrameTag')
for k,v in dict(Xmin=-3840,Xmax=3840,Ymin=-3840,Ymax=1280).items():r.find('displayRect').set(k,str(v))
r.set('frameCount','27');E.ElementTree(r).write(R/'assets/extracted/hub-ambient.xml')
