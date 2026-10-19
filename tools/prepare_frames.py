"""Bake the original Flash pose timelines into a bounded DS sprite canvas.

Run from the project root after exporting PLAYER.swf to player.xml with FFDec.
Selector clips are frozen instead of allowing all character variants to cycle.
"""
from pathlib import Path
from copy import deepcopy
import json
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
POSES=[('idle',244,13),('walk',335,16),('attack',496,13),('jump',356,12),('block',438,3),('hurt',373,6),
       ('light2',501,13),('light3',506,14),('finisher',581,18),('heavy',584,16),
       ('uppercut',708,15),('cast',767,15),('splash',781,12),('airattack',690,9),
       # Nested sprite388 stops at frame6; exporting beyond it causes FFDec to
       # loop the fall again because the static exporter does not run actions.
       ('knockdown',389,6),('land',396,6),('dead',394,2),('airhurt',379,1),('throw',839,17),
       # The first topspin sequence returns to jump at frame13. Later frames
       # are alternate entries, not part of this attack's initial playback.
       ('roll',1675,8),('airspin',700,13),('clash',738,5),('heavy2',588,12),('bow',795,20),('airattack2',691,12),('airheavy',714,14),('castdown',785,13)]

def freeze(sprite,frame):
    display={}
    current=1
    for x in sprite.find('subTags'):
        kind=x.get('type','')
        if kind=='ShowFrameTag':
            if current==frame: break
            current+=1
        elif kind.startswith('PlaceObject'):
            depth=x.get('depth')
            if x.get('placeFlagMove')=='true' and depth in display:
                old=display[depth]
                for flag,attr in [('placeFlagHasCharacter','characterId'),('placeFlagHasRatio','ratio'),('placeFlagHasName','name'),('placeFlagHasClipDepth','clipDepth')]:
                    if x.get(flag)=='true':old.set(flag,'true');old.set(attr,x.get(attr))
                for child in x:
                    prev=old.find(child.tag)
                    if prev is not None:old.remove(prev)
                    old.append(deepcopy(child))
                    if child.tag=='matrix':old.set('placeFlagHasMatrix','true')
                    if child.tag=='colorTransform':old.set('placeFlagHasColorTransform','true')
            else: display[depth]=deepcopy(x)
        elif kind.startswith('RemoveObject'):display.pop(x.get('depth'),None)
    tags=sprite.find('subTags');tags.clear()
    for depth,x in sorted(display.items(),key=lambda p:int(p[0])):
        x.set('placeFlagMove','false');tags.append(x)
    ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
    sprite.set('frameCount','1')

def main():
    metadata=[]
    for color,variant in [('blue',3),('orange',5),('red',4),('green',2)]:
        r=ET.parse(ROOT/'assets/extracted/player.xml').getroot()
        definitions=[]
        for x in r.find('tags'):
            if x.get('type','').startswith('Define'):
                definitions.append(x)
                if x.get('type')=='DefineSpriteTag':
                    actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in x.find('subTags') if a.get('type')=='DoActionTag')
                    if any(('f_'+s+'\0').encode() in actions for s in ['Helmet','Belt','Shield']):freeze(x,variant)
                    elif any(('f_'+s+'\0').encode() in actions for s in ['Weapon','Wings','Flag']):freeze(x,1)
                    # This optional face overlay is selected by runtime code.
                    # Its empty second frame preserves the selected knight helmet.
                    elif x.get('spriteId')=='371':freeze(x,2)
                    # Held arrow uses f_ArrowColor, a runtime selector.
                    elif x.get('spriteId')=='289':freeze(x,3 if color=='blue' else 4)
                    elif x.get('spriteId')=='438':freeze(x,1)
                    elif x.get('spriteId') in ('436','780'):
                        # Runtime-only alien shield and invisible spell hit-range guides.
                        t=x.find('subTags');t.clear()
                        ET.SubElement(t,'item',type='ShowFrameTag',forceWriteAsLong='false')
                        x.set('frameCount','1')
        tags=r.find('tags');tags.clear();tags.extend(definitions)
        frame=1
        for name,sid,count in POSES:
            ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
            place=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false',ratio='0',clipDepth='0')
            ET.SubElement(place,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
            for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
            # Jump stops at frame12, hurt at6, land returns to idle at6, and
            # arena death explicitly stops at2. Only idle/walk should loop.
            metadata.append(dict(color=color,pose=name,start=frame,count=count,
                                 source_sprite=sid,playback='loop' if name in ('idle','walk','clash') else 'once_hold'))
            frame+=count
        r.set('frameCount',str(frame-1))
        # Wide canvas keeps original sword trails intact; transparent margins are cropped later.
        rect=r.find('displayRect');rect.set('Xmin','-3840');rect.set('Xmax','3840');rect.set('Ymin','-3520');rect.set('Ymax','1280')
        ET.ElementTree(r).write(ROOT/f'assets/extracted/{color}.xml',encoding='utf-8',xml_declaration=True)
    (ROOT/'assets/poses.json').write_text(json.dumps(metadata,indent=2))

if __name__=='__main__':main()
