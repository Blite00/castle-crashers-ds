"""Bake an enemy species (troll, Troll Mother, imp) onto the engine's 304-frame pose layout.

python species_art.py prepare NAME   build assets/extracted/species-NAME.xml (needs assets/extracted/<pak>.xml from FFDec swf2xml)
python species_art.py convert NAME   pack assets/species-NAME/*.png (FFDec frame export of that xml) into generated/NAME_art.h
Frame k of the export is engine pose frame k, so the runtime indexes it exactly like the thief and barbarian streams."""
from pathlib import Path
import sys,json,hashlib
import xml.etree.ElementTree as ET
from PIL import Image
from prepare_frames import POSES
from bake_sprite import Baker
ROOT=Path(__file__).resolve().parents[1]
# Engine hit-onset frames (arena.c hit_frame) for the poses that strike.
HIT={'attack':2,'light2':4,'light3':3,'finisher':9,'heavy':6,'uppercut':2,'cast':5,'splash':6,'airattack':2,'throw':6,'airspin':2,
     'airattack2':4,'airheavy':4,'heavy2':1,'bow':10,'castdown':3}
def spread(sprite,length,n,a=1,b=None,stride=1):
    b=b or length;out=[]
    for k in range(n):
        f=a+round(k*(b-a)/max(1,n-1)) if n>1 else a
        if k not in (0,n-1):f=(f-a)//stride*stride+a
        out.append((sprite,min(max(f,1),length)))
    return out
def loop(sprite,length,n,stride=1):
    return [(sprite,(k//stride*stride)%length+1) for k in range(n)]
def strike(sprite,length,hit_src,n,h,stride=1):
    """Windup frames 1..hit_src spread over [0,h]; recovery hit_src..length spread over (h,n)."""
    out=[]
    for k in range(n):
        if k<=h:f=1+round(k*(hit_src-1)/max(1,h))
        else:f=hit_src+round((k-h)*(length-hit_src)/max(1,n-1-h))
        if k not in (0,h,n-1):f=(f-1)//stride*stride+1
        out.append((sprite,min(max(f,1),length)))
    return out
def hold(sprite,frame,n):return [(sprite,frame)]*n
def seqlist(pairs,n):
    """Pick n entries spread over an explicit (sprite,frame) list."""
    return [pairs[round(k*(len(pairs)-1)/max(1,n-1))] for k in range(n)] if n>1 else [pairs[0]]

def plan_troll(name,n):
    stand,walk,fall,hit,stun,juggle,b1,b2,ground,die,punch=(13,16),(16,8),(36,1),(58,7),(63,45),(65,1),(66,1),(67,1),(69,18),(90,45),(97,20)
    st=death_stage(name,n,die[0],die[1])
    if st:return st
    if name in HIT:return strike(punch[0],punch[1],12,n,HIT[name],2)
    if name=='idle':return loop(stand[0],stand[1],n,2)
    if name=='walk':return loop(walk[0],walk[1],n,2)
    if name in('jump','airhurt'):return hold(*juggle,n) if name=='airhurt' else hold(*fall,n)
    if name=='block':return hold(stand[0],1,n)
    if name=='hurt':return spread(hit[0],hit[1],n,stride=2)
    if name=='knockdown':return seqlist([juggle,b1,b2,(ground[0],1),(ground[0],6),(ground[0],12)],n)
    if name=='land':return spread(ground[0],ground[1],n,a=12,stride=2)
    if name=='dead':return seqlist([(die[0],41),(die[0],45)],n)
    if name=='roll':return loop(walk[0],walk[1],n,2)
    if name=='clash':return spread(stun[0],stun[1],n,stride=8)
    return hold(stand[0],1,n)
def plan_mother(name,n):
    stand,walk,birth,hit,die=(9,1),(23,24),(28,8),(31,8),(81,77)
    if name in HIT:return spread(birth[0],birth[1],n,stride=2) # arms raised and brought down: readable as an attack
    if name=='walk':return spread(walk[0],walk[1],n,stride=3)
    if name=='idle':return hold(stand[0],1,n)
    if name in('hurt','airhurt','knockdown','land','roll','clash','jump','block'):return spread(hit[0],hit[1],n,stride=3)
    if name=='dead':return seqlist([(die[0],40),(die[0],56)],n)
    return hold(stand[0],1,n)
def plan_imp(name,n):
    walk,dodge,grab,bag,hit1,hit2,juggle,ground,die=(7,6),(14,11),(25,41),(26,7),(32,11),(34,11),(36,1),(39,18),(41,46)
    st=death_stage(name,n,die[0],die[1])
    if st:return st
    if name in HIT:return spread(dodge[0],dodge[1],n,stride=2)
    if name in('idle','walk','roll','block','jump'):return loop(walk[0],walk[1],n,1 if name=='walk' else 2)
    if name=='hurt':return spread(hit1[0],hit1[1],n,stride=3)
    if name in('airhurt','knockdown'):return seqlist([juggle,(ground[0],1),(ground[0],9),(ground[0],18)],n)
    if name=='land':return spread(ground[0],ground[1],n,a=10,stride=3)
    if name=='dead':return seqlist([(die[0],40),(die[0],46)],n)
    if name=='clash':return spread(hit2[0],hit2[1],n,stride=3)
    return loop(walk[0],walk[1],n,2)
# Troll and imp deaths play their whole source clip: twelve stages stored in the otherwise unused cast pose slot, held four ticks each
# by render.c (the mother keeps her two-frame fall; her clip is too large for the chapter bank).
def death_stage(name,n,sprite,length):
    if name!='cast':return None
    return [(sprite,1+round(min(i,11)*(length-1)/11)) for i in range(n)]
RECT=dict(Xmin=-3840,Xmax=3840,Ymin=-3520,Ymax=1280)
BIG=dict(Xmin=-6400,Xmax=6400,Ymin=-6400,Ymax=1600)
SPECIES={'troll':dict(pak='etroll',plan=plan_troll,rect=RECT),'mother':dict(pak='etrollx',plan=plan_mother,rect=BIG),'imp':dict(pak='eimp',plan=plan_imp,rect=RECT)}

def prepare(name):
    spec=SPECIES[name];r=ET.parse(ROOT/f"assets/extracted/{spec['pak']}.xml").getroot();tags=r.find('tags')
    defs=[t for t in tags if t.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
    baker=Baker(tags);frames=[]
    for pose,sid,count in POSES:frames.extend(spec['plan'](pose,count))
    for ident,f in frames:
        new=baker.bake(ident,f)
        ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(new),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
        ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
    r.set('frameCount',str(len(frames)))
    rect=r.find('displayRect')
    for k,v in spec['rect'].items():rect.set(k,str(v))
    ET.ElementTree(r).write(ROOT/f'assets/extracted/species-{name}.xml',encoding='utf-8',xml_declaration=True)
    print(name,len(frames),'engine frames,',len(set(frames)),'distinct source frames')

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
def array(name,typ,values):
    return f'static const {typ} {name}[{len(values)}]={{\n'+''.join(','.join(map(str,values[i:i+24]))+',\n' for i in range(0,len(values),24))+'};\n'
def convert(name):
    count=sum(p[2] for p in POSES);U=name.upper()
    frames=[Image.open(ROOT/f'assets/species-{name}/{n+1}.png').convert('RGBA') for n in range(count)]
    R=SPECIES[name]['rect'];W2=(R['Xmax']-R['Xmin'])//40;H2=(R['Ymax']-R['Ymin'])//40;OX=-R['Xmin']//40;OY=-R['Ymin']//40
    assert frames[0].size==(W2*2,H2*2),frames[0].size
    frames=[im.resize((W2,H2),Image.Resampling.LANCZOS) for im in frames]
    # One palette from the distinct frames only.
    seen={};distinct=[]
    for im in frames:
        key=hashlib.sha1(im.tobytes()).digest()
        if key not in seen:seen[key]=len(distinct);distinct.append(im)
    atlas=Image.new('RGB',(W2,H2*len(distinct)))
    for n,im in enumerate(distinct):atlas.paste(im,(0,n*H2),im.getchannel('A'))
    palette=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);pal=palette.getpalette()[:765]
    ds_pal=[0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
    data=bytearray();meta=[];unique={};clipped=0
    for im in frames:
        q=im.convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE);alpha=im.getchannel('A').point(lambda a:255 if a>=100 else 0)
        box=alpha.getbbox()
        if not box:meta.append((0,0,0,1,1));continue
        x,y,r,b=box
        if x<=0 or y<=0 or r>=W2 or b>=H2:clipped+=1;x,y,r,b=max(x,1),max(y,1),min(r,W2-1),min(b,H2-1)
        block=bytes(q.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
        key=(r-x,b-y,block)
        if key not in unique:unique[key]=len(data);data.extend(pack(block))
        meta.append((unique[key],x-OX,y-OY,r-x,b-y))
    h=f'#ifndef {U}_ART_H\n#define {U}_ART_H\n#include "castle_art.h"\n/* Generated by species_art.py from the original {SPECIES[name]["pak"]} clip set. */\n'
    h+=f'#define {U}_FRAME_PIXELS_MAX {max(m[3]*m[4] for m in meta)}\n#define {U}_FRAME_COUNT {count}\n'
    h+=array(f'{name}_palette','uint16_t',ds_pal)+array(f'{name}_pixels','uint8_t',list(data))
    h+=f'static const ArtFrame {name}_frames[{count}]={{'+','.join('{%d,%d,%d,%d,%d}'%m for m in meta)+'};\n'
    h+=f'''static inline int {name}_decode_frame(unsigned id,uint8_t*out){{
 if(id>={U}_FRAME_COUNT||!out)return 0;
 const ArtFrame*f=&{name}_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){{
  if(at>=sizeof({name}_pixels))return 0;
  unsigned token={name}_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){{if(at>=sizeof({name}_pixels))return 0;unsigned v={name}_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}}
  else {{if(n>sizeof({name}_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++={name}_pixels[at++];}}
  left-=n;
 }}return 1;
}}
#endif
'''
    (ROOT/f'generated/{name}_art.h').write_text(h)
    print(name,'distinct frames',len(distinct),'pixel bytes',len(data),'max',max(m[3]*m[4] for m in meta),'clipped',clipped)
if __name__=='__main__':
    {'prepare':prepare,'convert':convert}[sys.argv[1]](sys.argv[2])
