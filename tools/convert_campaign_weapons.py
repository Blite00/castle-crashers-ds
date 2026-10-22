"""Original PLAYER weapon selector243, source frames1/2/9/19, as HUD icons."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import sys,json
from PIL import Image
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
IDS=[1,2,9,19,10,38,30,37,42,54,55,62]
if '--prepare' in sys.argv:
 r=E.parse(R/'assets/extracted/player.xml').getroot();defs=[x for x in r.find('tags') if x.get('type','').startswith('Define')]
 selector=next(x for x in defs if x.get('spriteId')=='243')
 for i,frame in enumerate(IDS):
  x=deepcopy(selector);x.set('spriteId',str(5000+i));freeze(x,frame);defs.append(x)
 tags=r.find('tags');tags.clear();tags.extend(defs)
 for i in range(len(IDS)):
  E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
  t=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(5000+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
  E.SubElement(t,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',scaleX='1.0',scaleY='1.0')
  E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
 r.set("frameCount",str(len(IDS)));rect=r.find('displayRect')
 for k,v in [('Xmin',-2400),('Xmax',2400),('Ymin',-2400),('Ymax',2400)]:rect.set(k,str(v))
 E.ElementTree(r).write(R/'assets/extracted/campaign-weapons.xml',encoding='utf-8',xml_declaration=True)
else:
 print('Icons are packed into nitrofiles/weapons/icons.bin by build_weapon_packs.py from assets/campaign-weapons/*.png')
