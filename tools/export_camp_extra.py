"""LEVEL20 extras read by tools/camp_actors.py, baked on a fixed canvas at zoom 0.5:
  assets/l20-244b   LEVEL20 sprite 244 (the story scene), 237 frames
  assets/fx-349     FX sprite 349 (the bears' wind ball)
python export_camp_extra.py"""
import xml.etree.ElementTree as ET
import rider_art as ra
R=ra.R
def bake(source,sid,count,rect,name):
    r=ET.parse(R/f'assets/extracted/{source}.xml').getroot();tags=r.find('tags')
    defs=[x for x in tags if x.get('type','').startswith('Define')]
    if count is None:count=int(next(x for x in defs if x.get('type')=='DefineSpriteTag' and x.get('spriteId')==str(sid)).get('frameCount'))
    tags.clear();tags.extend(defs)
    p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
    ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
    for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
    r.set('frameCount',str(count));box=r.find('displayRect')
    for k,v in zip(('Xmin','Xmax','Ymin','Ymax'),rect):box.set(k,str(v))
    xml=R/f'assets/extracted/{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
    swf=xml.with_suffix('.swf');out=R/f'assets/{name}'
    ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
    print(name,len(list(out.glob('*.png'))),flush=True)
if __name__=='__main__':
    bake('level20-full',244,237,(-600*20,600*20,-500*20,200*20),'l20-244b')
    bake('fx',349,None,(-8000,8000,-8000,8000),'fx-349')
