"""LEVEL22 monster, arms, logs and log fronts baked on a canvas with a known origin (DS pixel 400,400 of an 800x560 image).
python deer_extra.py   -> assets/deer-extra-<name>/N.png (zoom 0.5)"""
from pathlib import Path
import xml.etree.ElementTree as ET
import rider_art as ra
R=ra.R
CLIPS=[('monster',188,316),('arm1',190,2),('arm2',194,2),('log',132,17),('logfront',140,14),('overlay',208,35)]
def prepare():
    for name,sid,count in CLIPS:
        r=ET.parse(R/'assets/extracted/level22-full.xml').getroot();tags=r.find('tags')
        defs=[x for x in tags if x.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        # a root with one place and N frames lets FFDec advance the clip's own timeline
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        r.set('frameCount',str(count));rect=r.find('displayRect')
        for k,v in dict(Xmin=-16000,Xmax=16000,Ymin=-16000,Ymax=6400).items():rect.set(k,str(v))
        xml=R/f'assets/extracted/deer-extra-{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
        swf=xml.with_suffix('.swf');out=R/f'assets/deer-extra-{name}'
        ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
        print(name,len(list(out.glob('*.png'))),flush=True)
if __name__=='__main__':prepare()
