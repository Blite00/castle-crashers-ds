"""Magic effects from the original FX clips, at full animation and with their impact bursts.

python magic_fx.py prepare   build assets/extracted/magic-fx.xml and export every frame with FFDec (zoom 0.5 = DS pixels)
python magic_fx.py convert   pack the chosen frames into generated/magic_fx.h

Projectile types follow the source projectile_type: 1 poison, 2 lightning, 3 ice, 4 fire. Bolts are the animated children of
general_projectile (sprite 484: poison 304, lightning 309, ice shapes 305 and 310, fire 315); impacts are the children of
impact_general (sprite 994: poison 197, lightning 887, ice 219, fire 899). Semi transparent glow pixels are dithered
(checkerboard) instead of blended, so every effect is drawn by the plain sprite blitter."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as ET,subprocess,os,sys
from PIL import Image
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
# name, parts (character ids placed together), frames played, freeze variant (None = let nested clips play), stored frame indices (0 based)
G=[
 ('bolt_poison',[304],4,None,[0,1,2,3]),
 ('bolt_lightning',[309],1,None,[0]),
 ('bolt_ice',[305,310],1,None,[0]),
 ('bolt_fire',[315],8,None,[0,1,2,3,4,5,6,7]),
 ('impact_poison',[197],18,None,[0,2,4,6,8,10,12,14,16]),
 ('impact_lightning',[887],8,None,[0,1,2,3,4,5,6,7]),
 ('impact_ice',[219],5,None,[0,1,2,3,4]),
 ('impact_fire',[899],21,None,[0,2,4,6,8,10,12,14,16,18,20]),
 ('splash_ice',[1310],34,None,[0,3,6,9,12,15,18,21,24,27,30,33]),
 ('splash_fire',[1402],16,None,[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]),
 ('splash_lightning',[1629],12,None,[0,1,2,3,4,5,6,7,8,9,10,11]),
 ('splash_poison',[1397],31,None,[0,3,6,9,12,15,18,21,24,27,30]),
 ('frozen',[1645],14,None,[0,2,4,6,8,10,12,13]),
 ('burn',[1637],14,None,[0,2,4,6,8,10,12,13]),
 ('ready_poison',[35],14,1,[0,3,6,9]),
 ('ready_lightning',[35],14,2,[0,3,6,9]),
 ('ready_ice',[35],14,3,[0,3,6,9]),
 ('ready_fire',[35],14,4,[0,3,6,9]),
 ('arrow_blue',[538],8,3,[0,1,2,3,4,5,6,7]),
 ('arrow_orange',[538],8,4,[0,1,2,3,4,5,6,7]),
]
BAYER=[0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5]
TINT={'ready_ice':(150,215,255),'ready_fire':(255,175,70),'ready_lightning':(255,245,130),'ready_poison':(140,255,110)}
def tint(im,c):
    px=im.load()
    for y in range(im.height):
        for x in range(im.width):
            r,g,b,a=px[x,y]
            if a:
                l=(r+g+b)/765;k=0.35+0.65*l;px[x,y]=(min(255,int(c[0]*k+l*l*60)),min(255,int(c[1]*k+l*l*60)),min(255,int(c[2]*k+l*l*60)),a)
    return im
SCALE={'frozen':.55,'burn':.55,'splash_lightning':.55,'ready_ice':.75,'ready_fire':.75,'ready_lightning':.75,'ready_poison':.75}
def shrink(im,k):
    small=im.resize((round(im.width*k),round(im.height*k)),Image.Resampling.LANCZOS);out=Image.new('RGBA',im.size)
    out.paste(small,(HALF-round(HALF*k),HALF-round(HALF*k)));return out
jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def ffdec(*args):
    env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
    p=subprocess.run(['java','-Xmx2g','-jar',jar]+list(args),env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
HALF=240 # canvas half size in DS pixels
def prepare():
    r=ET.parse(R/'assets/extracted/fx.xml').getroot();tags=r.find('tags')
    defs=[x for x in tags if x.get('type','').startswith('Define')];originals={x.get('spriteId') or x.get('shapeId'):x for x in defs if x.get('spriteId')}
    tags.clear();tags.extend(defs);total=0
    for index,(name,parts,count,variant,keep) in enumerate(G):
        ids=[]
        for part in parts:
            sid=part
            if variant:
                dup=deepcopy(originals[str(part)]);sid=3000+index;dup.set('spriteId',str(sid));freeze(dup,variant);tags.append(dup)
            ids.append(sid)
        ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false');ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='2',forceWriteAsLong='false')
        for depth,sid in enumerate(ids,1):
            p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth=str(depth),forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
            ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='-800' if name.startswith('ready') else '0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        total+=count
    r.set('frameCount',str(total));rect=r.find('displayRect')
    for k,v in dict(Xmin=-HALF*40,Xmax=HALF*40,Ymin=-HALF*40,Ymax=HALF*40).items():rect.set(k,str(v))
    xml=R/'assets/extracted/magic-fx.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
    swf=xml.with_suffix('.swf');out=R/'assets/magic-fx'
    ffdec('-xml2swf',str(xml),str(swf));ffdec('-zoom','0.5','-ignorebackground','-export','frame',str(out),str(swf))
    print('frames',len(list(out.glob('*.png'))),flush=True)
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
def arr(n,t,v):return f'static const {t} {n}[{len(v)}]={{\n'+''.join(','.join(map(str,v[i:i+24]))+',\n' for i in range(0,len(v),24))+'};\n'
def convert():
    first=1;pics=[];entries=[]
    for name,parts,count,variant,keep in G:
        ims=[Image.open(R/f'assets/magic-fx/{first+k}.png').convert('RGBA') for k in keep]
        if name in TINT:ims=[tint(i,TINT[name]) for i in ims]
        if name in SCALE:ims=[shrink(i,SCALE[name]) for i in ims]
        if name.startswith('arrow'):ims=[im.rotate(a,resample=Image.Resampling.BICUBIC,center=(HALF,HALF)) for im,a in zip(ims,[5,2,0,-4,-8,-12,-18,-26])]
        entries.append((name,ims));first+=count
    for name,images in list(entries): # downward bolts: the same clip turned toward the ground (the source rotates by the shot angle)
        if name.startswith('bolt_'):entries.append((name+'_down',[im.rotate(-35,resample=Image.Resampling.BICUBIC) for im in images]))
    pics=[(name,im) for name,images in entries for im in images]
    atlas=Image.new('RGB',(max(i.width for _,i in pics),sum(i.height for _,i in pics)),(0,0,0));y=0
    for _,im in pics:
        atlas.paste(im.convert('RGB'),(0,y),im.getchannel('A').point(lambda a:255 if a>=48 else 0));y+=im.height
    pal=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);prgb=pal.getpalette()[:765]
    ds=[0]+[0x8000|(prgb[i]>>3)|((prgb[i+1]>>3)<<5)|((prgb[i+2]>>3)<<10) for i in range(0,765,3)]
    data=bytearray();frames=[];seen={};groups=[];n=0
    for name,images in entries:
        soft=name.startswith('ready')
        groups.append((len(frames),len(images)));
        for _ in images:
            _,im=pics[n];n+=1
            al=im.getchannel('A');px=al.load();q=im.convert('RGB').quantize(palette=pal,dither=Image.Dither.NONE)
            mask=Image.new('L',im.size,0);mp=mask.load()
            for yy in range(im.height):
                for xx in range(im.width):
                    a=px[xx,yy]
                    if a>=170 or (soft and a>BAYER[(yy&3)*4+(xx&3)]*15+8):mp[xx,yy]=255
            box=mask.getbbox()
            if not box:frames.append((0,0,0,0,0));continue
            x,y,r,b=box
            block=bytes(q.getpixel((xx,yy))+1 if mp[xx,yy] else 0 for yy in range(y,b) for xx in range(x,r))
            key=(r-x,b-y,x,y,block)
            if key in seen:frames.append(frames[seen[key]]);continue
            seen[key]=len(frames);off=len(data);data.extend(pack(block));frames.append((off,x-HALF,y-HALF,r-x,b-y))
    out=['#ifndef MAGIC_FX_H\n#define MAGIC_FX_H\n#include <stdint.h>\n/* Generated by tools/magic_fx.py from the original FX clips. */\ntypedef struct {uint32_t offset;int16_t x,y;uint16_t w,h;} MagicFrame;\ntypedef struct {uint16_t first,count;} MagicGroup;\n']
    out.append('enum {'+','.join('MFX_'+e[0].upper() for e in entries)+',MFX_GROUPS};\n')
    out.append(f'#define MAGIC_FRAME_PIXELS_MAX {max(f[3]*f[4] for f in frames)}\n')
    out.append(arr('magic_palette','uint16_t',ds));out.append(arr('magic_pixels','uint8_t',list(data)))
    out.append('static const MagicFrame magic_frames[%d]={'%len(frames)+','.join('{%d,%d,%d,%d,%d}'%f for f in frames)+'};\n')
    out.append('static const MagicGroup magic_groups[%d]={'%len(groups)+','.join('{%d,%d}'%g for g in groups)+'};\n')
    out.append('''static inline int magic_decode(unsigned id,uint8_t*out){
 if(id>=%d||!out)return 0;
 const MagicFrame*f=&magic_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(magic_pixels))return 0;
  unsigned token=magic_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(magic_pixels))return 0;unsigned v=magic_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else{if(n>sizeof(magic_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=magic_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''%len(frames))
    (R/'generated/magic_fx.h').write_text(''.join(out))
    print('frames',len(frames),'distinct',len(seen),'pixel bytes',len(data),'max',max(f[3]*f[4] for f in frames))
    for (name,_),g in zip(entries,groups):print(' ',name,g)
if __name__=='__main__':{'prepare':prepare,'convert':convert}[sys.argv[1]]()
