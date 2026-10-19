"""LEVEL22 monster arms: sprites 189 (monster_arm, at (-168,-477) inside 190) and 193 (monster_arm2, at (-317,-310) inside 194), 47 frame loops,
baked on the 190/194 clip origins (DS pixel 400,400 of an 800x560 image). python deer_arms.py"""
from pathlib import Path
import xml.etree.ElementTree as ET
import rider_art as ra
R=ra.R
CLIPS=[('armloop1',189,-168,-477,47),('armloop2',193,-317,-310,47)]
def prepare():
    for name,sid,tx,ty,count in CLIPS:
        r=ET.parse(R/'assets/extracted/level22-full.xml').getroot();tags=r.find('tags')
        defs=[x for x in tags if x.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX=str(tx*20),translateY=str(ty*20),rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='20',nRotateBits='0',nScaleBits='0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        r.set('frameCount',str(count));rect=r.find('displayRect')
        for k,v in dict(Xmin=-16000,Xmax=16000,Ymin=-16000,Ymax=6400).items():rect.set(k,str(v))
        xml=R/f'assets/extracted/deer-extra-{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
        swf=xml.with_suffix('.swf');out=R/f'assets/deer-extra-{name}'
        ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
        print(name,len(list(out.glob('*.png'))),flush=True)
if __name__=='__main__':prepare()
