"""Export original LEVEL13 hay-cart states and original FX hay fragments."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level13.xml').getroot();tags=r.find('tags');original={t.get('spriteId'):deepcopy(t) for t in tags if t.get('spriteId')}
for t in list(tags):
 if not t.get('type','').startswith('Define'):tags.remove(t)
for t in tags:
 if t.get('type')=='DefineSpriteTag':freeze(t,1)
for i in range(9):
 parent=deepcopy(original['367']);parent.set('spriteId',str(5000+i));freeze(parent,2 if i==0 else 3 if i==1 else 4)
 if i:
  child=deepcopy(original['364' if i==1 else '366']);child.set('spriteId',str(6000+i));freeze(child,3 if i==1 else i-1)
  for t in parent.find('subTags'):
   if t.get('characterId') in ('364','366'):t.set('characterId',str(6000+i))
  tags.append(child)
 tags.append(parent)
 p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(5000+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='true' if i else 'false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount','9')
for k,v in [('Xmin',-3200),('Ymin',-3200),('Xmax',3200),('Ymax',3200)]:r.find('displayRect').set(k,str(v))
E.ElementTree(r).write(R/'assets/extracted/campaign-breakables.xml')
r=E.parse(R/'assets/extracted/fx.xml').getroot();tags=r.find('tags')
for t in list(tags):
 if not t.get('type','').startswith('Define'):tags.remove(t)
 elif t.get('type')=='DefineSpriteTag':freeze(t,1)
for i,sid in enumerate([1138,1140,1142,1144,1146]):
 p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='true' if i else 'false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount','5')
for k,v in [('Xmin',-1280),('Ymin',-1280),('Xmax',1280),('Ymax',1280)]:r.find('displayRect').set(k,str(v))
E.ElementTree(r).write(R/'assets/extracted/campaign-hay.xml')
