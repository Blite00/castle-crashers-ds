"""Eight original PLAYER grey-knight frames for friendly castle guards."""
from pathlib import Path
import xml.etree.ElementTree as E
import argparse
from PIL import Image
from prepare_frames import freeze
from convert_barbarian import pack,unpack,array
R=Path(__file__).resolve().parents[1]
FRAMES=[1,7,14,22,30,36,43,46]
def prepare():
    r=E.parse(R/'assets/extracted/player.xml').getroot();tags=r.find('tags')
    for t in list(tags):
        if not t.get('type','').startswith('Define'):tags.remove(t);continue
        if t.get('type')!='DefineSpriteTag':continue
        actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in t.find('subTags') if a.get('type')=='DoActionTag')
        if any(('f_'+n+'\0').encode() in actions for n in ['Helmet','Belt','Emblem','Shield']):freeze(t,6)
        elif any(('f_'+n+'\0').encode() in actions for n in ['Weapon','Wings','Flag']):freeze(t,1)
        elif t.get('spriteId')=='371':freeze(t,2)
        elif t.get('spriteId')=='289':freeze(t,4)
    for sid,count in [(244,13),(335,16),(496,13),(373,6)]:
        E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1')
        p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
        E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
        for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag')
    r.set('frameCount','48');rect=r.find('displayRect')
    for k,v in dict(Xmin=-3840,Xmax=3840,Ymin=-3520,Ymax=1280).items():rect.set(k,str(v))
    E.ElementTree(r).write(R/'assets/extracted/gray-npc.xml',encoding='utf-8',xml_declaration=True)
def convert():
    frames=[Image.open(R/f'assets/gray-npc/{f}.png').convert('RGBA').resize((192,120),Image.Resampling.LANCZOS) for f in FRAMES]
    atlas=Image.new('RGB',(192,120*8))
    for i,im in enumerate(frames):atlas.paste(im,(0,i*120),im)
    q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette()
    pal=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,189,3)]
    data=bytearray();meta=[]
    for i,im in enumerate(frames):
        color=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);alpha=im.getchannel('A').point(lambda a:255 if a>=100 else 0)
        x,y,r,b=alpha.getbbox();block=bytes(color.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
        meta.append((len(data),x-96,y-88,r-x,b-y));data.extend(pack(block));assert unpack(data,meta[-1][0],len(block))==block
    h='#ifndef GRAY_NPC_ART_H\n#define GRAY_NPC_ART_H\n#include "castle_art.h"\n#define GRAY_NPC_FRAME_COUNT 8\n'
    h+=f'#define GRAY_NPC_FRAME_PIXELS_MAX {max(m[3]*m[4] for m in meta)}\n'
    h+=array('gray_npc_palette','uint16_t',pal)+array('gray_npc_pixels','uint8_t',data)
    h+='static const ArtFrame gray_npc_frames[8]={\n'+''.join('{'+','.join(map(str,m))+'},\n' for m in meta)+'};\n#endif\n'
    (R/'generated/gray_npc_art.h').write_text(h);atlas.save(R/'generated/gray-npc-preview.png')
    print('Grey NPC: 8 frames (idle,walk,attack,hurt pairs),',len(data)+128+8*12,'bytes')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');a=p.parse_args();prepare() if a.prepare else convert()
