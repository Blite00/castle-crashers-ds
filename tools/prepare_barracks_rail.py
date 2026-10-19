from pathlib import Path
import xml.etree.ElementTree as E
from copy import deepcopy
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level2.xml').getroot();ts=r.find('tags');scene=next(t for t in ts if t.get('spriteId')=='378');freeze(scene,1)
placements=[deepcopy(p) for p in scene.find('subTags') if p.get('characterId') in ('28','303','305')]
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
 elif t.get('spriteId'):freeze(t,1)
for k,v in [('Xmin',0),('Ymin',0),('Xmax',51200),('Ymax',17920)]:r.find('displayRect').set(k,str(v))
for p in placements:
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');p.set('depth','1');m=p.find('matrix');m.set('translateX',str(int(m.get('translateX','0'))+10000));m.set('translateY',str(int(m.get('translateY','0'))+14280));ts.append(p);E.SubElement(ts,'item',type='ShowFrameTag')
r.set('frameCount',str(len(placements)));E.ElementTree(r).write(R/'assets/extracted/barracks-rail.xml')
