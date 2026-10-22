"""LEVEL23 actors baked on a canvas with a known origin (DS pixel 400,400): fish, fins, rocks, logs, Catfish, king boat, bats, and the four
knights swimming. python flood_extra.py [name...]   -> assets/flood-extra-<name>/N.png (zoom 0.5)"""
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
import rider_art as ra
import deer_art
R=ra.R
LEVEL=[('fishidle',461,12),('fishhit',464,15),('fishjump',471,74),('fishcharge',475,89),('fishdie',485,9),('fin',489,1),('rock',458,1),('log',455,13),
       ('cat149',149,95),('cat218',218,38),('cat241',241,50),('cat248',248,114),('cat263',263,155),('cat280',280,31),('cat297',297,149),('boat',433,4),('catgo',313,49),('cannon',434,68),('angry',333,112)]
PLAYER=[('swimstand',1699,12),('swimwalk',1700,12),('bathead',1737,11),('logride',1693,16)]
def place(tags,sid,count):
    ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
    p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
    ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
    for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
def finish(r,total,xml,out):
    r.set('frameCount',str(total));rect=r.find('displayRect')
    for k,v in dict(Xmin=-16000,Xmax=16000,Ymin=-16000,Ymax=6400).items():rect.set(k,str(v))
    ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
    swf=xml.with_suffix('.swf');ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
def prepare(only):
    for name,sid,count in LEVEL:
        if only and name not in only:continue
        r=ET.parse(R/'assets/extracted/level23-full.xml').getroot();tags=r.find('tags')
        defs=[x for x in tags if x.get('type','').startswith('Define')];tags.clear();tags.extend(defs);place(tags,sid,count)
        out=R/f'assets/flood-extra-{name}';finish(r,count,R/f'assets/extracted/flood-extra-{name}.xml',out);print(name,len(list(out.glob('*.png'))),flush=True)
    for cname,variant in ra.COSTUMES[:4]:
        if only and cname not in only and 'player' not in only:continue
        r,defs=ra.costume_defs(cname,variant);tags=r.find('tags');tags.clear();tags.extend(defs);total=0
        for _,sid,count in PLAYER:place(tags,sid,count);total+=count
        out=R/f'assets/flood-{cname}';finish(r,total,R/f'assets/extracted/flood-{cname}.xml',out);print(cname,len(list(out.glob('*.png'))),flush=True)
    if not only or 'bat' in only:
        r=ET.parse(R/'assets/extracted/ebat.xml').getroot();tags=r.find('tags')
        defs=[x for x in tags if x.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
        place(tags,10,6);out=R/'assets/flood-extra-bat';finish(r,6,R/'assets/extracted/flood-extra-bat.xml',out);print('bat',len(list(out.glob('*.png'))))
if __name__=='__main__':prepare(sys.argv[1:])
