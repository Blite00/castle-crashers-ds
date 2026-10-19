"""Export sharp continuous LEVEL13 plus original depth-sorted architectural props."""
from pathlib import Path
import xml.etree.ElementTree as E
from copy import deepcopy
from prepare_frames import freeze
import json,math
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level13.xml').getroot();tags=r.find('tags')
for s in tags:
 sid=int(s.get('spriteId','0'))
 if s.get('type')=='DefineSpriteTag':
  if sid in (4,5,268,389,391,392,264,360,275,278,284) or 394<=sid<=406:
   s.find('subTags').clear();E.SubElement(s.find('subTags'),'item',type='ShowFrameTag');s.set('frameCount','1')
  else:freeze(s,1)
scene=next(s for s in tags if s.get('spriteId')=='407')
placements=[deepcopy(s) for s in scene.find('subTags') if s.get('characterId')]
# The hay cart is now a separately rendered destructible object, not baked scenery.
for placement in list(scene.find('subTags')):
 if placement.get('characterId')=='367':scene.find('subTags').remove(placement)
for t in list(tags):
 if not t.get('type','').startswith('Define'):tags.remove(t)
r.set('frameCount','1');rect=r.find('displayRect')
for k,v in [('Xmin',0),('Ymin',0),('Xmax',97920),('Ymax',17920)]:rect.set(k,str(v))
def place(root,char,x=4000,y=16200):
 ts=root.find('tags');p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(char),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX=str(x),translateY=str(y));E.SubElement(ts,'item',type='ShowFrameTag')
world=deepcopy(r);place(world,407);E.ElementTree(world).write(R/'assets/extracted/campaign-world.xml')
# Repeat frame indices each contain only one original placement. Layer bounds/depth are extracted below.
fg=deepcopy(r);ts=fg.find('tags');meta=[]
for p in placements:
 sid=int(p.get('characterId'));m=p.find('matrix')
 if sid not in (57,68,69,71,79,91,101,102,103,384):continue
 # Keep even tall offscreen placements; cropping skips wholly invisible exports.
 m.set('translateX',str(int(m.get('translateX','0'))+4000));m.set('translateY',str(int(m.get('translateY','0'))+16200))
 p.set('depth','1');p.set('placeFlagMove','true' if meta else 'false');ts.append(p);E.SubElement(ts,'item',type='ShowFrameTag')
 meta.append({'shape':sid,'placement_y':int(m.get('translateY'))/40,'source_depth':int(p.get('depth'))})
fg.set('frameCount',str(len(meta)));E.ElementTree(fg).write(R/'assets/extracted/campaign-world-fg.xml');(R/'assets/campaign-world-layers.json').write_text(json.dumps(meta,indent=2))
print('Continuous world and',len(meta),'foreground source placements prepared')
