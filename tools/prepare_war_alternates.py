"""Export original destroyed fence display lists at their authored placements."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level11-full.xml').getroot();ts=r.find('tags')
scene=next(t for t in ts if t.get('spriteId')=='376');freeze(scene,1)
# Sprite 161 frame 2 starts a 32-frame break animation (sprite 160): an explosion burst,
# flying planks, then nothing. Keep sample frames so the DS can play it and then clear it.
WALL_KEYFRAMES=(1,6,12,19)
originals={t.get('spriteId'):deepcopy(t) for t in ts if t.get('type')=='DefineSpriteTag' and t.get('spriteId') in ('130','133','160','161')}
gate_placement=next(deepcopy(t) for t in scene.find('subTags') if t.get('characterId')=='133')
placements=[deepcopy(t) for t in scene.find('subTags') if t.get('characterId') in ('161','172','303','316')]
for s in list(ts):
 if s.get('type')=='DefineSpriteTag':freeze(s,25 if s.get('spriteId')=='303' else 2 if s.get('spriteId') in ('161','171','316') else 1)
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for k in WALL_KEYFRAMES[1:]:
 s160=deepcopy(originals['160']);s160.set('spriteId',str(20000+k));freeze(s160,k)
 s161=deepcopy(originals['161']);s161.set('spriteId',str(21000+k))
 for t in s161.find('subTags'):
  if t.get('characterId')=='160':t.set('characterId',str(20000+k))
 freeze(s161,2);ts.append(s160);ts.append(s161)
# Original wood door sprite130 shows damage frames 2..5 and a destroyed frame 6 (f_HitDoor).
GATE_FRAMES=(2,3,4,5,6)
for k in GATE_FRAMES:
 d130=deepcopy(originals['130']);d130.set('spriteId',str(22000+k));freeze(d130,k)
 d133=deepcopy(originals['133']);d133.set('spriteId',str(23000+k))
 for t in d133.find('subTags'):
  if t.get('characterId')=='130':t.set('characterId',str(22000+k))
 freeze(d133,1);ts.append(d130);ts.append(d133)
info=[]
cinema=[deepcopy(t) for t in placements if (t.get('name') or '').startswith('cinema_wall')]
for p in placements:
 m=p.find('matrix');depth=int(p.get('depth'));y=int(m.get('translateY','0'))
 info.append(dict(source=int(p.get('characterId')),source_depth=depth,name=p.get('name'),depth=round((y/20+700)/2),keyframe=0))
 m.set('translateX',str(int(m.get('translateX','0'))+2112*20));m.set('translateY',str(y+700*20));p.set('depth','1');p.set('placeFlagMove','false')
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');ts.append(p);E.SubElement(ts,'item',type='ShowFrameTag')
for p in cinema:
 for n,k in enumerate(WALL_KEYFRAMES[1:],1):
  m=p.find('matrix');y=int(m.get('translateY','0'));q=deepcopy(p);q.set('characterId',str(21000+k))
  info.append(dict(source=161,source_depth=int(p.get('depth')),name=p.get('name'),depth=round((y/20+700)/2),keyframe=n,sample=k))
  mm=q.find('matrix');mm.set('translateX',str(int(mm.get('translateX','0'))+2112*20));mm.set('translateY',str(y+700*20));q.set('depth','1');q.set('placeFlagMove','false')
  E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');ts.append(q);E.SubElement(ts,'item',type='ShowFrameTag')
for k in GATE_FRAMES:
 q=deepcopy(gate_placement);m=q.find('matrix');y=int(m.get('translateY','0'));q.set('characterId',str(23000+k))
 info.append(dict(source=133,source_depth=int(q.get('depth')),name='gate',depth=round((y/20+700)/2),keyframe=k-1,sample=k))
 m.set('translateX',str(int(m.get('translateX','0'))+2112*20));m.set('translateY',str(y+700*20));q.set('depth','1');q.set('placeFlagMove','false')
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');ts.append(q);E.SubElement(ts,'item',type='ShowFrameTag')
for k,v in [('Xmin',0),('Ymin',0),('Xmax',4608*40),('Ymax',448*40)]:r.find('displayRect').set(k,str(v))
r.set('frameCount',str(len(info)));E.ElementTree(r).write(R/'assets/extracted/war-alternates.xml');(R/'assets/war-alternates.json').write_text(json.dumps(info,indent=2))
print('Original broken fences:',len(info))
