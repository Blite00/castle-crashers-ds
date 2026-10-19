from pathlib import Path
import xml.etree.ElementTree as E
from copy import deepcopy
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level2.xml').getroot();ts=r.find('tags');scene=next(t for t in ts if t.get('spriteId')=='378');freeze(scene,1)
placements=[deepcopy(p) for p in scene.find('subTags') if p.get('characterId') in ('53','60','73','117','136')]
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
 elif t.get('spriteId'):freeze(t,1)
for k,v in [('Xmin',0),('Ymin',0),('Xmax',51200),('Ymax',17920)]:r.find('displayRect').set(k,str(v))
for p in placements:
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');p.set('depth','1');m=p.find('matrix');m.set('translateX',str(int(m.get('translateX','0'))+10000));m.set('translateY',str(int(m.get('translateY','0'))+14280));ts.append(p);E.SubElement(ts,'item',type='ShowFrameTag')
r.set('frameCount',str(len(placements)));E.ElementTree(r).write(R/'assets/extracted/barracks-furniture.xml')
# Remove original debug impact shapes from the standalone wounded movie.
f=R/'assets/extracted/barracks-wounded.xml';r=E.parse(f).getroot();m=next(t for t in r.find('tags') if t.get('spriteId')=='366').find('subTags')
for p in list(m):
 if p.get('characterId') in ('307','308','310','311','312','313'):m.remove(p)
E.ElementTree(r).write(f)
import json
(R/'assets/barracks-furniture-depths.json').write_text(json.dumps([round((int(p.find('matrix').get('translateY')))/40) for p in placements]))
print('Furniture layers',len(placements))
