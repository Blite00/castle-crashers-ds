"""Bake the original MOUNT2 crocodile (body sprite 9 + head/bite sprite 29) into one frame per bite key on a fixed canvas.
Canvas origin (ground contact under the mount) sits at source pixel (CANVAS_ORIGIN)."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
CANVAS=(400,300);ORIGIN=(200,250);BODY_Y=-34 # source px; body centre, with the feet (sprite 5; the original mount has just the two front legs) under it as in the original saddled mount
HEAD_DX=42 # the head clip sits at the body's right shoulder
FEET=[(18,-12,4),(40,-12,5)] # x,y from the ground contact, depth (body is depth 3, head 6)
src=E.parse(R/'assets/extracted/mount2.xml').getroot()
r=deepcopy(src);tags=r.find('tags')
defs=[t for t in tags if t.get('type','').startswith('Define')]
sprites={t.get('spriteId'):t for t in defs if t.get('type')=='DefineSpriteTag'}
tags.clear();tags.extend(defs)
head=sprites['29'];frames=int(head.get('frameCount'))
def place(char,x,y,depth):
    p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(char),depth=str(depth),forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
    E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX=str(int(x*20)),translateY=str(int(y*20)),rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='20',nRotateBits='0',nScaleBits='0')
nid=700
for k in range(1,frames+1):
    c=deepcopy(head);freeze(c,k);c.set('spriteId',str(nid));tags.append(c)
    for d in range(1,7):E.SubElement(tags,'item',type='RemoveObject2Tag',depth=str(d),forceWriteAsLong='false')
    for fx,fy,d in FEET:place(5,ORIGIN[0]+fx,ORIGIN[1]+fy,d)
    place(9,ORIGIN[0],ORIGIN[1]+BODY_Y,3);place(nid,ORIGIN[0]+HEAD_DX,ORIGIN[1]+BODY_Y,6)
    E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false');nid+=1
r.set('frameCount',str(frames))
for kk,v in [('Xmin',0),('Xmax',CANVAS[0]*20),('Ymin',0),('Ymax',CANVAS[1]*20)]:r.find('displayRect').set(kk,str(v))
E.ElementTree(r).write(R/'assets/extracted/forest-croc.xml',encoding='utf-8',xml_declaration=True)
print('croc frames',frames)
