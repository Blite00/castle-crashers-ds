from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import sys
sys.path.insert(0,'tools')
from prepare_frames import freeze
r=E.parse('assets/extracted/fx.xml').getroot();ts=r.find('tags');original=next(t for t in ts if t.get('spriteId')=='834')
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for i,f in enumerate((2,3,4,5,6)):
 s=deepcopy(original);s.set('spriteId',str(2100+i));freeze(s,f);ts.append(s)
for k,v in [('Xmin',-960),('Ymin',-1920),('Xmax',960),('Ymax',1280)]:r.find('displayRect').set(k,str(v))
for i in range(5):
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(2100+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag')
r.set('frameCount','5');E.ElementTree(r).write('assets/extracted/food-variants.xml')
r=E.parse('assets/extracted/level19.xml').getroot();ts=r.find('tags')
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for k,v in [('Xmin',-960),('Ymin',-1920),('Xmax',960),('Ymax',1280)]:r.find('displayRect').set(k,str(v))
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='102',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag');r.set('frameCount','1');E.ElementTree(r).write('assets/extracted/shop-potion.xml')
