"""Deer ride poses (LEVEL22) for the four knights: PLAYER deer_walk 1032, deerpoo_walk 1033, deertoss 1038, deerslam 1043,
runscared 1676, dockjump 1691, jumptohorse 1702, using the same costume freezing as rider_art.py.
python deer_art.py prepare [costume]   build assets/extracted/deer-<costume>.xml and export frames with FFDec (zoom 0.5)"""
from pathlib import Path
import xml.etree.ElementTree as ET,sys
import rider_art as ra
R=ra.R
CLIPS=[('walk',1032,11),('poo',1033,3),('toss',1038,24),('slam',1043,44),('scared',1676,10),('dock',1691,55),('mount',1702,14)]
def prepare(only=None):
    for name,variant in ra.COSTUMES:
        if only and name!=only:continue
        r,definitions=ra.costume_defs(name,variant);tags=r.find('tags');tags.clear();tags.extend(definitions);total=0
        for clip,sid,count in CLIPS:
            ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
            p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
            ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
            for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
            total+=count
        r.set('frameCount',str(total));rect=r.find('displayRect')
        for k,v in dict(Xmin=-5120,Xmax=5120,Ymin=-5120,Ymax=2560).items():rect.set(k,str(v))
        xml=R/f'assets/extracted/deer-{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
        swf=xml.with_suffix('.swf');out=R/f'assets/deer-{name}'
        ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
        print(name,len(list(out.glob('*.png'))),flush=True)
if __name__=='__main__':prepare(sys.argv[2] if len(sys.argv)>2 else None)
