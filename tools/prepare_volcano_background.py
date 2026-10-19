"""Export original level47 scenery in its background/foreground draw layers.

Runtime collision helpers are hidden by the game, but FFDec's static renderer
shows their authoring geometry. Clear those definitions before rasterizing.
The root's bg0_c layer is a real foreground; never bake it beneath fighters.
"""
from pathlib import Path
from copy import deepcopy
import json
import xml.etree.ElementTree as ET
from prepare_frames import freeze
ROOT=Path(__file__).resolve().parents[1]
r=ET.parse(ROOT/'assets/extracted/level47.xml').getroot()
# The original loader selects arena_mode * 2 + 1; mode 1 is PvP.
freeze(next(s for s in r.find('tags') if s.get('spriteId')=='164'),3)
game=next(s for s in r.find('tags') if s.get('spriteId')=='164')
props=[deepcopy(t) for t in game.find('subTags') if t.get('characterId') in ('148', '150', '152', '155', '157', '159')]
for t in list(game.find('subTags')):
    if t.get('characterId') in ('148', '150', '152', '155', '157', '159'):game.find('subTags').remove(t)
for s in r.find('tags'):
    if s.get('spriteId') in ('106', '135', '138', '140', '141', '144', '160', '163'):
        # Hitzones, exit portals, guide, bottom marker, navigation and spawn guides.
        tags=s.find('subTags');tags.clear()
        ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        s.set('frameCount','1')

for tag in r.find('tags'):
    if tag.get('name')=='game':
        tag.find('matrix').set('translateX','33824');tag.find('matrix').set('translateY','10401')
rect=r.find('displayRect');rect.set('Xmax','16960');rect.set('Ymax','12720')
for foreground,name in [(False,'arena-volcano'),(True,'arena-volcano-foreground')]:
    scene=deepcopy(r); tags=scene.find('tags')
    for tag in list(tags):
        kind=tag.get('type','')
        if kind.startswith('PlaceObject'):
            is_foreground=tag.get('name')=='bg0_c'
            if is_foreground!=foreground:tags.remove(tag)
        elif foreground and kind=='SetBackgroundColorTag':tags.remove(tag)
    ET.ElementTree(scene).write(ROOT/f'assets/extracted/{name}.xml',encoding='utf-8',xml_declaration=True)
# Export each original neutral object in stage coordinates for depth sorting.
scene=deepcopy(r); tags=scene.find('tags')
definitions=[t for t in tags if t.get('type','').startswith('Define')]
tags.clear();tags.extend(definitions);metadata=[]
for index,prop in enumerate(props):
    ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
    prop.set('depth','1');prop.set('placeFlagMove','false')
    m=prop.find('matrix');x=int(m.get('translateX'))+13078;y=int(m.get('translateY'))+12300
    m.set('translateX',str(x));m.set('translateY',str(y));tags.append(prop)
    ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
    metadata.append(dict(frame=index+1,sprite=int(prop.get('characterId')),origin_x=x/40,depth_y=y/40))
scene.set('frameCount',str(len(props)))
ET.ElementTree(scene).write(ROOT/'assets/extracted/arena-volcano-props.xml',encoding='utf-8',xml_declaration=True)
(ROOT/'assets/arena-volcano-props.json').write_text(json.dumps(metadata,indent=2))
