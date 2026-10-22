"""Original War/bridge scenery and source registered depth layers at half scale."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
import sys
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
CONFIG=[('war',11,376,(-2112,-700),(4608,448),{4,5,358,360,363,364}, {133,161,172,247,303,316}),('bridge',12,94,(-1024,-800),(1024,448),{4,5,80,82,85},{48,74,75}),('forest',18,244,(-256,-736),(2304,448),{1,5,40,187,211,222,224,229,231,234,237,239,241,242,243},set()),('thieves',4,393,(0,-1350),(6688,900),{4,165,371,376,46,138,235,251,300,302,310,312,313,317,324,332,180,363},set()),('deer',22,243,(-1400,-1450),(3900,400),{4,67,229,231,234,238,239,241,242,210,224,88,132,135,140,141,188,190,194,208},set()),('flood',23,548,(-600,-560),(1000,448),{298,313,333,433,434,455,459,486,490,70,1,539,540,126,543,544,545,547},set()),
         ('camp',20,386,(-4700,-800),(6450,448),{1,5,140,187,244,342,349,367,368,370,373,374,378,380,381,382,383,385},{199})]
if '--hubs' in sys.argv:
 CONFIG=[('blacksmith',14,112,(-512,-700),(768,448),{1,99,101,104},{73}),('frog',15,207,(-384,-700),(3200,448),{12,154,191,192,194,197},set()),('ark',16,98,(640,-760),(2816,448),{4,17,84,86,89},set())]
ONLY=[a for a in sys.argv[1:] if not a.startswith('--')]
for name,level,sid,origin,size,hidden,dynamic in CONFIG:
 if (ONLY and name not in ONLY) or (not ONLY and name in ('forest','thieves','camp')):continue
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
