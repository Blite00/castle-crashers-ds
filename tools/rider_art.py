"""Original mounted-rider poses (PLAYER horse_idle, horse_walk, horse_punch1_1/2, horse_bow, horsejump) for the four knights and the thief.

python rider_art.py prepare   build assets/extracted/rider-<costume>.xml and export frames with FFDec (zoom 0.5 = DS pixels)
python rider_art.py convert   pack them into generated/rider_art.h (the pixel stream goes into the forest bank)

Costume freezing follows prepare_frames.py (knights, starter sword) and convert_thief.py (thief). Frame layout per costume:
idle 1, walk 8, punch A 6 keys, punch B 6 keys, bow 5 keys, jump 6 keys = 32 frames."""
from pathlib import Path
import xml.etree.ElementTree as ET,subprocess,os,sys
from PIL import Image
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
COSTUMES=[('blue',3),('orange',5),('red',4),('green',2),('thief',8)]
CLIPS=[('idle',1025,1),('walk',1026,8),('punchA',1028,11),('punchB',1029,11),('bow',1030,20),('jump',1034,23)]
KEYS={'idle':[1],'walk':list(range(1,9)),'punchA':[1,3,5,7,9,11],'punchB':[1,3,5,7,9,11],'bow':[1,5,10,15,20],'jump':[1,5,9,14,19,23]}
FRAMES_PER_COSTUME=sum(len(v) for v in KEYS.values())
jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def ffdec(*args):
    env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
    p=subprocess.run(['java','-Xmx2g','-jar',jar]+list(args),env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
def costume_defs(name,variant):
    r=ET.parse(R/'assets/extracted/player.xml').getroot();definitions=[]
    for x in r.find('tags'):
        if not x.get('type','').startswith('Define'):continue
        definitions.append(x)
        if x.get('type')!='DefineSpriteTag':continue
        actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in x.find('subTags') if a.get('type')=='DoActionTag')
        thief=name=='thief'
        if any(('f_'+s+'\0').encode() in actions for s in (['Helmet','Belt','Emblem','Shield'] if thief else ['Helmet','Belt','Shield'])):freeze(x,variant)
        elif b'f_Weapon\0' in actions:freeze(x,10 if thief else 1)
        elif any(('f_'+s+'\0').encode() in actions for s in ['Wings','Flag']):freeze(x,1)
        elif x.get('spriteId')=='371':freeze(x,2)
        elif x.get('spriteId')=='289':freeze(x,4 if thief or name!='blue' else 3)
        elif x.get('spriteId')=='438':freeze(x,1)
        elif x.get('spriteId') in ('436','780'):
            t=x.find('subTags');t.clear();ET.SubElement(t,'item',type='ShowFrameTag',forceWriteAsLong='false');x.set('frameCount','1')
    return r,definitions
def prepare():
    for name,variant in COSTUMES:
        r,definitions=costume_defs(name,variant);tags=r.find('tags');tags.clear();tags.extend(definitions)
        total=0
        for clip,sid,count in CLIPS:
            ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
            p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
            ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
            for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
            total+=count
        r.set('frameCount',str(total));rect=r.find('displayRect')
        for k,v in dict(Xmin=-3840,Xmax=3840,Ymin=-3520,Ymax=1280).items():rect.set(k,str(v))
        xml=R/f'assets/extracted/rider-{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
        swf=xml.with_suffix('.swf');out=R/f'assets/rider-{name}'
        ffdec('-xml2swf',str(xml),str(swf));ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
        print(name,len(list(out.glob('*.png'))),flush=True)
def pack(block):
    result=bytearray();i=0
    while i<len(block):
        run=1
        while run<128 and i+run<len(block) and block[i+run]==block[i]:run+=1
        if run>=3:result.extend([127+run,block[i]]);i+=run
        else:
            start=i;i+=run
            while i<len(block) and i-start<128:
                if i+2<len(block) and block[i]==block[i+1]==block[i+2]:break
                i+=1
            result.append(i-start-1);result.extend(block[start:i])
    return result
def arr(n,t,v):
    return f'static const {t} {n}[{len(v)}]={{\n'+''.join(','.join(map(str,v[i:i+24]))+',\n' for i in range(0,len(v),24))+'};\n'
def convert():
    starts={};n=1
    for clip,sid,count in CLIPS:starts[clip]=n;n+=count
    images=[]
    for name,variant in COSTUMES:
        for clip,sid,count in CLIPS:
            for k in KEYS[clip]:images.append(Image.open(R/f'assets/rider-{name}/{starts[clip]+k-1}.png').convert('RGBA'))
    atlas=Image.new('RGB',(max(i.width for i in images),sum(i.height for i in images)),(0,0,0));y=0
    for im in images:atlas.paste(im.convert('RGB'),(0,y),im.getchannel('A').point(lambda a:255 if a>=100 else 0));y+=im.height
    pal=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);prgb=pal.getpalette()[:765]
    ds=[0]+[0x8000|(prgb[i]>>3)|((prgb[i+1]>>3)<<5)|((prgb[i+2]>>3)<<10) for i in range(0,765,3)]
    data=bytearray();frames=[];seen={}
    W2,H2=192,60 # export canvas in DS pixels: Xmin -192, Ymin -176 (display rect -3840..-3520 twips halved)
    for im in images:
        a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);box=a.getbbox()
        if not box:frames.append((0,0,0,0,0));continue
        q=im.convert('RGB').quantize(palette=pal,dither=Image.Dither.NONE);x,y,r,b=box
        block=bytes(q.getpixel((xx,yy))+1 if a.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
        key=(r-x,b-y,x,y,block)
        if key not in seen:
            seen[key]=len(frames);off=len(data);data.extend(pack(block));frames.append((off,x-96,y-88,r-x,b-y))
        else:frames.append(frames[seen[key]])
    out=['#ifndef RIDER_ART_H\n#define RIDER_ART_H\n#include <stdint.h>\n#include "castle_art.h"\n/* Generated by tools/rider_art.py from PLAYER horse_* clips: knights blue, orange, red, green and the thief, in that order. */\n']
    out.append(f'#define RIDER_FRAMES_PER_COSTUME {FRAMES_PER_COSTUME}\n#define RIDER_COSTUMES {len(COSTUMES)}\n#define RIDER_FRAME_PIXELS_MAX {max(f[3]*f[4] for f in frames)}\n')
    off=0;names=[]
    for clip,sid,count in CLIPS:names.append(f'RIDER_{clip.upper()}={off}');off+=len(KEYS[clip])
    out.append('enum {'+','.join(names)+'};\n')
    out.append(arr('rider_palette','uint16_t',ds));out.append(arr('rider_pixels','uint8_t',list(data)))
    out.append('static const ArtFrame rider_frames[%d]={'%len(frames)+','.join('{%d,%d,%d,%d,%d}'%f for f in frames)+'};\n')
    out.append('''static inline int rider_decode(unsigned id,uint8_t*out){
 if(id>=%d||!out)return 0;
 const ArtFrame*f=&rider_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(rider_pixels))return 0;
  unsigned token=rider_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(rider_pixels))return 0;unsigned v=rider_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else{if(n>sizeof(rider_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=rider_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''%len(frames))
    (R/'generated/rider_art.h').write_text(''.join(out))
    print('frames',len(frames),'pixel bytes',len(data),'max',max(f[3]*f[4] for f in frames))
if __name__=='__main__':{'prepare':prepare,'convert':convert}[sys.argv[1]]()
