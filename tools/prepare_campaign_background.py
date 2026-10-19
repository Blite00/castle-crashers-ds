"""Prepare Castle Keep (LEVEL13) art; original scene at a bounded room crop."""
from pathlib import Path
import xml.etree.ElementTree as E
from copy import deepcopy
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/level13.xml').getroot()
tags=r.find('tags')
# Hide original runtime-only editor navigation/collision/portal guides and movies.
for s in tags:
    sid=int(s.get('spriteId','0'))
    if sid in (4,5,268,389,391,392,264,360) or 394<=sid<=406:
        if s.get('type')=='DefineSpriteTag':
            s.find('subTags').clear();E.SubElement(s.find('subTags'),'item',type='ShowFrameTag');s.set('frameCount','1')
    if sid==407:freeze(s,1)
# Keep definitions and place the real playable scenery directly; this removes
# runtime parallax/cutscene camera offsets. All geometry remains original.
for t in list(tags):
    if not t.get('type','').startswith('Define'):tags.remove(t)
# Six overlapping sections retain original half-scale geometry across the full Keep.
ORIGINS=[(-200,-810),(600,-550),(1400,-460),(2200,-500),(3000,-810),(3800,-810)]
for i,(x,y) in enumerate(ORIGINS):
    p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId='407',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='true' if i else 'false')
    E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX=str(-x*20),translateY=str(-y*20))
    E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount',str(len(ORIGINS)))
rect=r.find('displayRect');rect.set('Xmin','0');rect.set('Ymin','0');rect.set('Xmax','16960');rect.set('Ymax','12720')
E.ElementTree(r).write(R/'assets/extracted/campaign.xml',encoding='utf-8',xml_declaration=True)
