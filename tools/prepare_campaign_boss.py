"""Original LEVEL7 scenery and source registered depth layers at half scale."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
import sys
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
CONFIG=[('boss',7,290,(-768,-700),(1024,448),{13,17,111,120,126,202,227,238,245,251,270,275,277,280,281},set())]
for name,level,sid,origin,size,hidden,dynamic in CONFIG:
 source='audit' if level in (14,15) else 'full'
 r=E.parse(R/f'assets/extracted/level{level}-{source}.xml').getroot();ts=r.find('tags');registered=set()
 for s in ts:
  if s.get('type')!='DefineSpriteTag':continue
  ident=int(s.get('spriteId'));actions=b''.join(bytes.fromhex(t.get('actionBytes','')) for t in s.find('subTags') if t.get('type')=='DoActionTag')
  if any(k in actions for k in (b'f_AddNeutral\0',b'f_AddObject\0',b'f_RegisterGrass\0')):registered.add(ident)
  if ident in hidden:
   s.find('subTags').clear();E.SubElement(s.find('subTags'),'item',type='ShowFrameTag');s.set('frameCount','1')
  else:freeze(s,1)
 scene=next(s for s in ts if s.get('spriteId')==str(sid));placements=[deepcopy(s) for s in scene.find('subTags') if s.get('characterId')]
 layers=[]
 for p in list(scene.find('subTags')):
  ident=int(p.get('characterId','0'))
  if ident in dynamic:scene.find('subTags').remove(p)
 for p in placements:
  ident=int(p.get('characterId'));m=p.find('matrix')
  if ident in hidden or m is None:continue
  if ident in registered or ident in dynamic:
   x=int(m.get('translateX','0'))/20;y=int(m.get('translateY','0'))/20
   layers.append(dict(source=ident,name=p.get('name'),source_depth=int(p.get('depth')),depth=round((y-origin[1])/2),x=(x-origin[0])/2,y=(y-origin[1])/2,dynamic=ident in dynamic,placement=p))
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
 rect=r.find('displayRect')
 for k,v in [('Xmin',0),('Ymin',0),('Xmax',size[0]*40),('Ymax',size[1]*40)]:rect.set(k,str(v))
 bg=deepcopy(r);tags=bg.find('tags');p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX=str(-origin[0]*20),translateY=str(-origin[1]*20));E.SubElement(tags,'item',type='ShowFrameTag');bg.set('frameCount','1');E.ElementTree(bg).write(R/f'assets/extracted/campaign-{name}.xml')
 fg=deepcopy(r);tags=fg.find('tags')
 for layer in layers:
  p=layer.pop('placement');m=p.find('matrix');m.set('translateX',str(int(m.get('translateX','0'))-origin[0]*20));m.set('translateY',str(int(m.get('translateY','0'))-origin[1]*20));p.set('depth','1');p.set('placeFlagMove','false');E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1');tags.append(p);E.SubElement(tags,'item',type='ShowFrameTag')
 fg.set('frameCount',str(len(layers)));E.ElementTree(fg).write(R/f'assets/extracted/campaign-{name}-fg.xml');(R/f'assets/campaign-{name}-layers.json').write_text(json.dumps(dict(level=level,origin=origin,size=size,layers=layers),indent=2));print(name,'registered',len(layers),'dynamic',sum(x['dynamic'] for x in layers))
