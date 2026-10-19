"""Bake LEVEL18 cutscene props (King, princess litter, kidnapping thieves) at their scene positions, one frozen key per output frame."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1];source=E.parse(R/'assets/extracted/level18-full.xml').getroot()
ORIGIN=(-256,-736);SIZE=(2304,448)
# name, sprite, name of its scene placement, sampled source frames (clip frame numbers)
SPEC=[('king',222,'boss',list(range(1,98,4))+[97]),
      ('litter',211,'boss_door',list(range(1,164,6))+[163]),
      ('thieves',187,'thieves',list(range(1,79,4))+[78])]
scene=next(t for t in source.find('tags') if t.get('type')=='DefineSpriteTag' and t.get('spriteId')=='244')
places={t.get('name'):t for t in scene.find('subTags') if t.get('type')=='PlaceObject2Tag' and t.get('name')}
out={}
for name,sid,place,samples in SPEC:
 r=deepcopy(source);ts=r.find('tags');defs={int(t.get('spriteId')):t for t in ts if t.get('type')=='DefineSpriteTag'}
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
 nextid=[600]
 def bake(ident,frame):
  if ident not in defs:return ident
  src=defs[ident];f=min(frame,int(src.get('frameCount')))
  births={};now=1
  for t in src.find('subTags'):
   if t.get('type')=='ShowFrameTag':
    if now==f:break
    now+=1
   elif t.get('type','').startswith('Place') and t.get('placeFlagHasCharacter')=='true':births[t.get('depth')]=now
   elif t.get('type','').startswith('Remove'):births.pop(t.get('depth'),None)
  clone=deepcopy(src);freeze(clone,f);new=nextid[0];nextid[0]+=1;clone.set('spriteId',str(new))
  for t in clone.find('subTags'):
   if t.get('characterId'):
    childframe=f-births.get(t.get('depth'),1)+1
    t.set('characterId',str(bake(int(t.get('characterId')),max(1,childframe))))
  ts.append(clone);return new
 for sample in samples:
  ident=bake(sid,sample)
  p=deepcopy(places[place]);p.set('characterId',str(ident));p.set('depth','1');p.set('placeFlagMove','false')
  m=p.find('matrix');m.set('translateX',str(int(m.get('translateX','0'))-ORIGIN[0]*20));m.set('translateY',str(int(m.get('translateY','0'))-ORIGIN[1]*20))
  E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');ts.append(p);E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount',str(len(samples)))
 for k,v in [('Xmin',0),('Xmax',SIZE[0]*40),('Ymin',0),('Ymax',SIZE[1]*40)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/forest-prop-{name}.xml');out[name]=samples
(R/'assets/forest-props.json').write_text(json.dumps(out))
print({k:len(v) for k,v in out.items()})
