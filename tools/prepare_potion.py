from pathlib import Path
import xml.etree.ElementTree as E
import sys
sys.path.insert(0,'tools')
from prepare_frames import freeze
r=E.parse('assets/extracted/player.xml').getroot();ts=r.find('tags');s=next(t for t in ts if t.get('spriteId')=='305');print('itemselector',s.get('frameCount'));freeze(s,9)
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for k,v in [('Xmin',-960),('Ymin',-960),('Xmax',960),('Ymax',960)]:r.find('displayRect').set(k,str(v))
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='305',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag');r.set('frameCount','1');E.ElementTree(r).write('assets/extracted/potion.xml')
