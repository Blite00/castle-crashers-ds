"""LEVEL20 actor art beyond the bear pose layout: the propeller entry (PLAYER sprite 1389), the boss dance (sprite 1839) in the bear costume, and the bats.
python camp_actors.py prepare   build and export assets/camp-bear-extra
python camp_actors.py           pack generated/camp_actor_art.h (bank pixels, palette and frame table)"""
import sys,struct
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image
import rider_art as ra
from prepare_frames import freeze
R=ra.R
NL=chr(10)
EXTRA=[('propeller',1389,9),('dance',1839,78),('tornado',1769,2)]
def prepare():
    r=ET.parse(R/'assets/extracted/player.xml').getroot();definitions=[]
    for x in r.find('tags'):
        if not x.get('type','').startswith('Define'):continue
        definitions.append(x)
        if x.get('type')!='DefineSpriteTag':continue
        actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in x.find('subTags') if a.get('type')=='DoActionTag')
        if any(('f_'+s+'\0').encode() in actions for s in ['Helmet','Belt','Emblem','Shield']):freeze(x,20)
        elif b'f_Weapon\0' in actions:freeze(x,29)
        elif any(('f_'+s+'\0').encode() in actions for s in ['Wings','Flag']):freeze(x,1)
        elif x.get('spriteId')=='371':freeze(x,2)
        elif x.get('spriteId')=='289':freeze(x,4)
        elif x.get('spriteId')=='438':freeze(x,1)
        elif x.get('spriteId') in ('436','780'):
            t=x.find('subTags');t.clear();ET.SubElement(t,'item',type='ShowFrameTag',forceWriteAsLong='false');x.set('frameCount','1')
    tags=r.find('tags');tags.clear();tags.extend(definitions);total=0
    for name,sid,count in EXTRA:
        ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        total+=count
    r.set('frameCount',str(total));rect=r.find('displayRect')
    for k,v in dict(Xmin=-3840,Xmax=3840,Ymin=-3520,Ymax=1280).items():rect.set(k,str(v))
    xml=R/'assets/extracted/camp-bear-extra.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
    swf=xml.with_suffix('.swf');ra.ffdec('-xml2swf',str(xml),str(swf));ra.ffdec('-export','frame',str(R/'assets/camp-bear-extra'),str(swf))
def groups():
    g=[]
    g.append(('PROPELLER',[('bear',1+i,96,88) for i in range(0,9)]))
    g.append(('DANCE',[('bear',10+i,96,88) for i in range(0,78,8)]))
    g.append(('BAT',[('bat',f,400,400) for f in range(1,7)]))
    g.append(('TORNADO',[('bear',88+i,96,88) for i in range(0,2)]))
    g.append(('WIND',[('wind',f,200,200) for f in range(1,6)]))
    g.append(('STORY',[('story',17+i,300,250) for i in range(0,221,12)]))
    return g
def load(kind,f):
    if kind=='bear':return Image.open(R/f'assets/camp-bear-extra/{f}.png').convert('RGBA').resize((192,120),Image.Resampling.LANCZOS)
    if kind=='story':return Image.open(R/f'assets/l20-244b/{f}.png').convert('RGBA')
    if kind=='wind':return Image.open(R/f'assets/fx-349/{f}.png').convert('RGBA')
    return Image.open(R/f'assets/flood-extra-bat/{f}.png').convert('RGBA')
def pack(block):
    out=bytearray();i=0
    while i<len(block):
        run=1
        while run<128 and i+run<len(block) and block[i+run]==block[i]:run+=1
        if run>=3:out.extend([127+run,block[i]]);i+=run
        else:
            start=i;i+=run
            while i<len(block) and i-start<128:
                if i+2<len(block) and block[i]==block[i+1]==block[i+2]:break
                i+=1
            out.append(i-start-1);out.extend(block[start:i])
    return out
def bank(name,ctype,blob):
    return f'static const uint8_t {name}_data[{len(blob)}] __attribute__((aligned(4)))={{'+NL+''.join(','.join(map(str,blob[j:j+32]))+','+NL for j in range(0,len(blob),32))+'};'+NL+f'#define {name} ((const {ctype}*)({name}_data))'+NL
DECODER='''static inline int camp_actor_decode(unsigned id,uint8_t*out){
 if(id>=@N@||!out)return 0;
 const ArtFrame*f=&camp_actor_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(camp_actor_pixels))return 0;
  unsigned token=camp_actor_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(camp_actor_pixels))return 0;unsigned v=camp_actor_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else{if(n>sizeof(camp_actor_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=camp_actor_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''
def convert():
    G=groups();images=[]
    for name,items in G:
        for kind,f,ox,oy in items:images.append((load(kind,f),ox,oy))
    atlas=Image.new('RGB',(max(i.width for i,_,_ in images),sum(i.height for i,_,_ in images)));y=0
    for im,_,_ in images:atlas.paste(im.convert('RGB'),(0,y),im.getchannel('A').point(lambda a:255 if a>=100 else 0));y+=im.height
    pal=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);prgb=pal.getpalette()[:765]
    ds=[0]+[0x8000|(prgb[i]>>3)|((prgb[i+1]>>3)<<5)|((prgb[i+2]>>3)<<10) for i in range(0,765,3)]
    data=bytearray();frames=[];seen={}
    for im,ox,oy in images:
        a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);box=a.getbbox()
        if not box:frames.append((0,0,0,0,0));continue
        q=im.convert('RGB').quantize(palette=pal,dither=Image.Dither.NONE);x,y,r,b=box
        block=bytes(q.getpixel((xx,yy))+1 if a.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
        key=(r-x,b-y,x-ox,y-oy,block)
        if key in seen:frames.append(frames[seen[key]]);continue
        seen[key]=len(frames);off=len(data);data.extend(pack(block));frames.append((off,x-ox,y-oy,r-x,b-y))
    h='#ifndef CAMP_ACTOR_ART_H'+NL+'#define CAMP_ACTOR_ART_H'+NL+'#include <stdint.h>'+NL+'#include "castle_art.h"'+NL+'/* Generated by tools/camp_actors.py. */'+NL
    h+=f'#define CAMP_ACTOR_PIXELS_MAX {max(f[3]*f[4] for f in frames)}'+NL
    first=0;names=[];table=[]
    for name,items in G:names.append(f'CA_{name}={len(table)}');table.append((first,len(items)));first+=len(items)
    h+='enum {'+','.join(names)+f',CA_GROUPS={len(table)}}};'+NL+'typedef struct {uint16_t first,count;} CampGroup;'+NL
    h+=f'static const CampGroup camp_groups[{len(table)}]={{'+','.join('{%d,%d}'%t for t in table)+'};'+NL
    h+=bank('camp_actor_palette','uint16_t',b''.join(struct.pack('<H',v) for v in ds))
    h+=f'static const uint8_t camp_actor_pixels[{len(data)}] __attribute__((aligned(4)))={{'+NL+''.join(','.join(map(str,data[j:j+32]))+','+NL for j in range(0,len(data),32))+'};'+NL
    h+=bank('camp_actor_frames','ArtFrame',b''.join(struct.pack('<IhhHH',*f) for f in frames))
    h+=DECODER.replace('@N@',str(len(frames)))
    (R/'generated/camp_actor_art.h').write_text(h)
    print('frames',len(frames),'distinct',len(seen),'pixel bytes',len(data),'max',max(f[3]*f[4] for f in frames))
if __name__=='__main__':
    if 'prepare' in sys.argv:prepare()
    else:convert()
