"""Thieves Forest cutscene clips (LEVEL4): intro carriers (sprite 300), owl (235), deer (251).

python thieves_scenes.py prepare   build assets/extracted/scene-<clip>.xml and export frames with FFDec (zoom 0.5 = DS pixels)
python thieves_scenes.py convert   pack the frames into generated/thieves_scene_art.h (palette, deduplicated RLE frames, anchors)
Clip frame k is source clip frame k. The exported canvas origin is the clip origin, so a frame offset is its crop offset
plus half the display rectangle minimum (source pixels to DS pixels)."""
from pathlib import Path
import xml.etree.ElementTree as ET,subprocess,os,sys,json
from PIL import Image
R=Path(__file__).resolve().parents[1]
# name, sprite id, frame count, display rect in source pixels (xmin,ymin,xmax,ymax), playback stride
CLIPS=[('intro',300,154,(-230,-180,225,10),6),('owl',235,431,(-70,-120,105,145),9),('deer',251,202,(-1365,-100,140,35),6)]
# Placement in sprite 393 of level 4: twips/20 source pixels. World x = source x / 2, world y = (source y + 1350) / 2 before the shear.
ANCHOR={'intro':(10823/20,-14506/20),'owl':(36701/20,-21761/20),'deer':(92051/20,-15071/20)}
jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def ffdec(*args):
    env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
    p=subprocess.run(['java','-Xmx2g','-jar',jar]+list(args),env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
def prepare():
    for name,sid,count,rect,stride in CLIPS:
        r=ET.parse(R/'assets/extracted/level4-full.xml').getroot();tags=r.find('tags')
        defs=[t for t in tags if t.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
        r.set('frameCount',str(count));d=r.find('displayRect')
        d.set('Xmin',str(rect[0]*20));d.set('Ymin',str(rect[1]*20));d.set('Xmax',str(rect[2]*20));d.set('Ymax',str(rect[3]*20))
        xml=R/f'assets/extracted/scene-{name}.xml';ET.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
        swf=xml.with_suffix('.swf');out=R/f'assets/scene-{name}'
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
    clips=[];sample=[]
    for name,sid,count,rect,stride in CLIPS:
        ims=[Image.open(R/f'assets/scene-{name}/{k+1}.png').convert('RGBA') for k in range(count)]
        keep=sorted(set(list(range(0,count,stride))+[count-1]));clips.append((name,keep,ims,rect))
        sample+= [ims[k] for k in keep]
    atlas=Image.new('RGB',(max(i.width for i in sample),sum(i.height for i in sample)),(0,0,0));y=0
    for im in sample:atlas.paste(im.convert('RGB'),(0,y),im.getchannel('A').point(lambda a:255 if a>=100 else 0));y+=im.height
    pal=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT);prgb=pal.getpalette()[:189]
    ds=[0]+[0x8000|(prgb[i]>>3)|((prgb[i+1]>>3)<<5)|((prgb[i+2]>>3)<<10) for i in range(0,189,3)]
    shear=json.loads((R/'assets/thieves-shear.json').read_text())['dy']
    def dy(x):return round(shear[min(max(int(x),0),len(shear)-1)])
    data=bytearray();frames=[(0,0,0,0,0)];uniq={('empty',):0};keys=[];clipdefs=[]
    for (name,keep,ims,rect),(nm,sid,count,_,stride) in zip(clips,CLIPS):
        base=len(keys)
        for k in keep:
            im=ims[k];a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);box=a.getbbox()
            if not box:key=('empty',);block=b''
            else:
                q=im.convert('RGB').quantize(palette=pal,dither=Image.Dither.NONE);x,y,r,b=box
                block=bytes(q.getpixel((xx,yy))+1 if a.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r));key=(r-x,b-y,x,y,block)
            if key not in uniq:
                uniq[key]=len(frames);off=len(data);data.extend(pack(block))
                cx=round(ANCHOR[name][0]/2)+x+rect[0]//2+(r-x)//2
                frames.append((off,x+rect[0]//2,y+rect[1]//2-dy(cx),r-x,b-y))
            keys.append((k,uniq[key]))
        ax=ANCHOR[name][0]/2;ay=(ANCHOR[name][1]+1350)/2
        clipdefs.append((round(ax),round(ay),base,len(keep),count))
    out=['#ifndef THIEVES_SCENE_ART_H\n#define THIEVES_SCENE_ART_H\n#include <stdint.h>\n#include "castle_art.h"\n/* Generated by tools/thieves_scenes.py from the original LEVEL4 intro, owl and deer clips. */\n']
    out.append(arr('thieves_scene_palette','uint16_t',ds));out.append(arr('thieves_scene_pixels','uint8_t',list(data)))
    out.append('static const ArtFrame thieves_scene_frames[%d]={'%len(frames)+','.join('{%d,%d,%d,%d,%d}'%m for m in frames)+'};\n')
    out.append('typedef struct {uint16_t source_frame,frame;} ThievesSceneKey;\n')
    out.append('static const ThievesSceneKey thieves_scene_keys[%d]={'%len(keys)+','.join('{%d,%d}'%k for k in keys)+'};\n')
    out.append('typedef struct {int16_t anchor_x,anchor_y;uint16_t first,count,length;} ThievesSceneClip;\n')
    out.append('static const ThievesSceneClip thieves_scene_clips[%d]={'%len(clipdefs)+','.join('{%d,%d,%d,%d,%d}'%c for c in clipdefs)+'};\n')
    out.append('#define THIEVES_SCENE_FRAME_PIXELS_MAX %d\n'%max(m[3]*m[4] for m in frames))
    out.append('''static inline int thieves_scene_decode(unsigned id,uint8_t*out){
 if(id>=%d||!out)return 0;
 const ArtFrame*f=&thieves_scene_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(thieves_scene_pixels))return 0;
  unsigned token=thieves_scene_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(thieves_scene_pixels))return 0;unsigned v=thieves_scene_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else{if(n>sizeof(thieves_scene_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=thieves_scene_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''%len(frames))
    (R/'generated/thieves_scene_art.h').write_text(''.join(out))
    print('frames',len(frames),'pixel bytes',len(data),'keys',len(keys))
if __name__=='__main__':{'prepare':prepare,'convert':convert}[sys.argv[1]]()
