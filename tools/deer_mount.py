"""Original MOUNT3 deer (LEVEL22) baked one frame per pose key. python deer_mount.py prepare
Clips: idle 9, walk 20 (11 frames), poop 29 (3), jump 30 (24), dockjump 31 (74), scared 35, eat 39; shadow 2 is drawn by the game."""
from pathlib import Path
import xml.etree.ElementTree as ET,sys
import rider_art as ra
R=ra.R
CLIPS=[('idle',[9],1),('walk',[20],11),('poop',[29],3),('jump',[30],24),('dock',[31],74),('scared',[35],1),('eat',[39],1)]
def prepare():
    r=ET.parse(R/'assets/extracted/mount3.xml').getroot();tags=r.find('tags')
    defs=[x for x in tags if x.get('type','').startswith('Define')];tags.clear();tags.extend(defs);total=0
    for clip,ids,count in CLIPS:
        ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
        for sid in ids:
            p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
            ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        total+=count
    r.set('frameCount',str(total));rect=r.find('displayRect')
    for k,v in dict(Xmin=-5120,Xmax=5120,Ymin=-5120,Ymax=2560).items():rect.set(k,str(v))
    xml=R/'assets/extracted/deer-mount.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
    swf=xml.with_suffix('.swf');out=R/'assets/deer-mount'
    ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
    print(len(list(out.glob('*.png'))))
if __name__=='__main__':prepare()
