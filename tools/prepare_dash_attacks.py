"""Export source dashpunch/dashslam without changing existing pose IDs."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
original=E.parse(R/'assets/extracted/player.xml').getroot()
selectors={}
for t in original.find('tags'):
 if t.get('type')=='DefineSpriteTag':
  actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in t.find('subTags') if a.get('type')=='DoActionTag')
  if b'f_Weapon\0' in actions:selectors[t.get('spriteId')]=t
for color in ('blue','orange','red','green'):
 for weapon in (1,2,9,19,10,38,30,37,42):
  r=E.parse(R/f'assets/extracted/{color}.xml').getroot();tags=r.find('tags')
  definitions=[t for t in tags if t.get('type','').startswith('Define')];tags.clear();tags.extend(definitions)
  for i,t in enumerate(list(tags)):
   if t.get('spriteId') in selectors:
    replacement=deepcopy(selectors[t.get('spriteId')]);freeze(replacement,weapon);tags.remove(t);tags.insert(i,replacement)
  for sid,count in ((737,12),(736,12)):
   E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
   p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false',ratio='0',clipDepth='0')
   E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
   for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
  r.set('frameCount','24');E.ElementTree(r).write(R/f'assets/extracted/dash-attack-{color}-{weapon}.xml',encoding='utf-8',xml_declaration=True)
print('Prepared16 source timelines,24 frames each')
