"""Original classic MAP terrain and named node coordinates, without UI chrome."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/map-full.xml').getroot();ts=r.find('tags');nodes=[]
scene=next(t for t in ts if t.get('spriteId')=='201')
for t in list(scene.find('subTags')):
 if t.get('name') in ('insanetext','insaneflame'):scene.find('subTags').remove(t)
for t in scene.find('subTags'):
 if t.get('name','').startswith('node'):
  m=t.find('matrix');nodes.append(dict(id=int(t.get('name')[4:]),x=round((int(m.get('translateX','0'))/20+1024)/2),y=round((int(m.get('translateY','0'))/20+2048)/2)))
# MAP sprite201 is a single-frame parchment illustration in the source, not
# a full-color timeline. Apply explicit material colors to its vector artwork
# before rasterization; coordinates, paths and source contours stay untouched.
# This is an adapted color map, not a claim of a hidden original color frame.
terrain_colors = {
 (125,103,66):(46,66,43), (115,94,60):(43,53,37),
 (147,120,77):(144,107,67), (153,153,102):(85,157,61),
 (140,140,94):(58,120,48), (140,153,102):(81,153,57),
 (158,158,112):(183,207,113), (139,139,95):(71,139,62),
 (152,139,103):(194,163,101), (136,147,119):(117,177,83),
}
# Castle, smithy, arena and barbarian fort use different materials even
# though their source fills share the same olive ink.
materials = {
 '32': {(153,153,102):(158,168,177)},
 '102': {(153,153,102):(168,180,194)},
 '151': {(153,153,102):(185,199,214),(134,134,89):(126,148,173)},
 '75': {(153,153,102):(166,119,76),(112,91,58):(101,72,44)},
}
for definition in ts:
 overrides=materials.get(definition.get('shapeId'), {})
 for color in definition.iter():
  if color.get('type') in ('RGB','RGBA'):
   old=tuple(int(color.get(k,'0')) for k in ('red','green','blue'))
   new=overrides.get(old,terrain_colors.get(old))
   if new:
    for key,value in zip(('red','green','blue'),new):color.set(key,str(value))
for s in ts:
 if s.get('type')=='DefineSpriteTag':freeze(s,1)
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='201',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX=str(1024*20),translateY=str(2048*20));E.SubElement(ts,'item',type='ShowFrameTag')
for k,v in [('Xmin',0),('Ymin',0),('Xmax',3584*20),('Ymax',2560*20)]:r.find('displayRect').set(k,str(v))
r.set('frameCount','1');E.ElementTree(r).write(R/'assets/extracted/original-worldmap.xml');(R/'assets/worldmap-nodes.json').write_text(json.dumps(nodes,indent=2));print('Original named map nodes',len(nodes))
