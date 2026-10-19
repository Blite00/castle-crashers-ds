"""Export original MENU647 attribute icons, preserving source geometry."""
from pathlib import Path
import xml.etree.ElementTree as E
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/menu.xml').getroot();tags=r.find('tags')
for t in list(tags):
 if not t.get('type','').startswith('Define'):tags.remove(t)
for sid in [367,366,364,365]:
 E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1')
 p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount','4')
for k,v in [('Xmin',-1280),('Xmax',1280),('Ymin',-1280),('Ymax',1280)]:r.find('displayRect').set(k,str(v))
E.ElementTree(r).write(R/'assets/extracted/ui-attribute-icons.xml')
