from pathlib import Path
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level2.xml').getroot();ts=r.find('tags')
for s in ts:
 if s.get('type')=='DefineSpriteTag':
  sid=int(s.get('spriteId'));freeze(s,1)
  if sid in (13,16,59,84,114,135,150,154,165,184,202,204,223,251,285,302,317,325,327,366):
   s.find('subTags').clear();E.SubElement(s.find('subTags'),'item',type='ShowFrameTag')
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
r.set('frameCount','1')
for k,v in [('Xmin',0),('Ymin',0),('Xmax',51200),('Ymax',17920)]:r.find('displayRect').set(k,str(v))
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='378',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='10000',translateY='14280');E.SubElement(ts,'item',type='ShowFrameTag')
E.ElementTree(r).write(R/'assets/extracted/campaign-barracks.xml')
