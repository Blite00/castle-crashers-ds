"""Pack the baked LEVEL18 cutscene props into 32x64 independently inflated tiles at absolute world positions."""
from pathlib import Path
from PIL import Image
import json,zlib,hashlib
R=Path(__file__).resolve().parents[1]
spec=json.loads((R/'assets/forest-props.json').read_text())
names=list(spec)
ims=[];index=[]
for pi,name in enumerate(names):
    for si,frame in enumerate(spec[name]):
        im=Image.open(R/f'assets/forest-prop-{name}/{si+1}.png').convert('RGBA');im=im.resize((im.width//2,im.height//2),Image.Resampling.LANCZOS)
        ims.append(im);index.append((pi,frame))
# One shared palette for all props.
atlas=Image.new('RGB',(512,sum(max(1,(im.getchannel('A').point(lambda a:255 if a>=100 else 0).getbbox() or (0,0,1,1))[3]-(im.getchannel('A').point(lambda a:255 if a>=100 else 0).getbbox() or (0,0,1,1))[1]) for im in ims)),(0,0,0))
y=0
for im in ims:
    a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);b=a.getbbox()
    if not b:continue
    c=im.crop(b);c.thumbnail((512,10000));atlas.paste(c,(0,y),a.crop(b).resize(c.size));y+=c.height
q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);pal=q.getpalette()[:189]
rgb555=[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,189,3)];unique=list(dict.fromkeys(rgb555));remap=[unique.index(v)+1 for v in rgb555]
palette=[0]+unique+[0]*(64-1-len(unique))
blob=bytearray();tiles=[];frames=[];seen={};maximum=0
for im,(pi,frame) in zip(ims,index):
    a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);b=a.getbbox();first=len(tiles)
    if b:
        crop=im.crop(b);alpha=a.crop(b).tobytes();qi=crop.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes()
        w,h=crop.size;raw=bytes(remap[v] if aa else 0 for v,aa in zip(qi,alpha))
        for yy in range(0,h,64):
            for xx in range(0,w,32):
                ww,hh=min(32,w-xx),min(64,h-yy)
                part=b''.join(raw[(yy+r)*w+xx:(yy+r)*w+xx+ww] for r in range(hh))
                if not any(part):continue
                key=(ww,hh,part)
                if key not in seen:
                    enc=zlib.compress(part,9)[2:-4];assert zlib.decompress(enc,-15)==part;seen[key]=(len(blob),len(enc));blob.extend(enc)
                off,length=seen[key];tiles.append((b[0]+xx,b[1]+yy,ww,hh,off,length));maximum=max(maximum,len(part))
    frames.append((pi,frame,first,len(tiles)-first))
def arr(n,t,v):return f'static const {t} {n}[{len(v)}]={{\n'+''.join(','.join(map(str,v[i:i+32]))+',\n' for i in range(0,len(v),32))+'};\n'
h='#ifndef FOREST_PROPS_ART_H\n#define FOREST_PROPS_ART_H\n#include <stdint.h>\n#include "asset_inflate.h"\n'
h+='typedef struct {int16_t x,y;uint16_t w,h;uint32_t offset,length;} ForestPropTile;\ntypedef struct {uint8_t prop,frame;uint16_t first,count;} ForestPropFrame;\n'
h+='enum {FOREST_PROP_KING,FOREST_PROP_LITTER,FOREST_PROP_THIEVES,FOREST_PROP_COUNT};\n'
h+=f'#define FOREST_PROP_FRAME_COUNT {len(frames)}\n#define FOREST_PROP_TILE_COUNT {len(tiles)}\n#define FOREST_PROP_PIXELS_MAX {maximum}\n'
import struct
def bank_table(n,ctype,fmt,rows):
    data=b''.join(struct.pack(fmt,*r) if isinstance(r,(tuple,list)) else struct.pack(fmt,r) for r in rows)
    return f'static const uint8_t {n}_data[{len(data)}] __attribute__((aligned(4)))={{\n'+''.join(','.join(map(str,data[k:k+32]))+',\n' for k in range(0,len(data),32))+f'}};\n#define {n} ((const {ctype}*)({n}_data))\n'
h+=bank_table('forest_prop_palette','uint16_t','<H',palette)+arr('forest_prop_data','uint8_t',blob)
h+=bank_table('forest_prop_tiles','ForestPropTile','<hhHHII',tiles)+bank_table('forest_prop_frames','ForestPropFrame','<BBHH',frames)
h+='''/* Last baked key at or before the requested clip frame (1-based) for one prop; -1 when none. */
static inline int forest_prop_frame_for(unsigned prop,unsigned clip_frame){
 int best=-1;
 for(unsigned i=0;i<FOREST_PROP_FRAME_COUNT;i++)if(forest_prop_frames[i].prop==prop&&forest_prop_frames[i].frame<=clip_frame)best=(int)i;
 return best;
}
static inline int forest_prop_decode_tile(unsigned id,uint8_t*out){
 if(id>=FOREST_PROP_TILE_COUNT||!out)return 0;
 const ForestPropTile*t=&forest_prop_tiles[id];
 return asset_inflate(forest_prop_data+t->offset,t->length,out,(unsigned)t->w*t->h);
}
#endif
'''
(R/'generated/forest_props_art.h').write_text(h)
print('forest props',len(frames),'frames',len(tiles),'tiles',len(blob),'bytes max tile',maximum)
