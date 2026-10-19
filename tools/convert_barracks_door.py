"""OriginalLEVEL2 doorway317: closed1, then nested314's32-frame burst.
Run prepare_barracks_door.py and FFDec frame exports first. World1280x448,
origin(-500,-714),halfscale. No invented door animation or opacity effects.
"""
from pathlib import Path
import json,zlib,sys
from PIL import Image
from convert_barbarian import array
R=Path(__file__).resolve().parents[1]
paths=[R/'assets/barracks-door-closed/1.png']+[R/f'assets/barracks-door-open/{f}.png' for f in range(1,33,2)]
ims=[Image.open(p).convert('RGBA').resize((1280,448),Image.Resampling.LANCZOS) for p in paths]
# Allocate palette from just original opaque art; transparency staysindex0.
opaque=[p[:3] for im in ims for p in im.getdata() if p[3]>=96]
atlas=Image.new('RGB',(len(opaque),1));atlas.putdata(opaque)
q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
p=q.getpalette();palette=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,63*3,3)]
records=[];data=bytearray();max_pixels=0;hashes=[];preview=[]
for im in ims:
 alpha=im.getchannel('A').point(lambda x:255 if x>=96 else 0);box=alpha.getbbox()
 if box:
  x,y,r,b=box;crop=im.crop(box);colors=crop.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();mask=alpha.crop(box).tobytes();raw=bytes(c+1 if a else 0 for c,a in zip(colors,mask));w=r-x;h=b-y
 else:x,y,w,h,raw=1089,175,1,1,b'\0'
 z=zlib.compressobj(9,zlib.DEFLATED,-15);packed=z.compress(raw)+z.flush();assert zlib.decompress(packed,-15)==raw
 records.append((x,y,w,h,len(data),len(packed)));data.extend(packed);max_pixels=max(max_pixels,w*h)
 hval=2166136261
 for v in raw:hval=((hval^v)*16777619)&0xffffffff
 hashes.append(hval)
 display=Image.new('RGBA',(1280,448));tile=Image.new('RGBA',(w,h));tile.putdata([((palette[c]&31)*255//31,((palette[c]>>5)&31)*255//31,((palette[c]>>10)&31)*255//31,255 if c else 0) for c in raw]);display.alpha_composite(tile,(x,y));preview.append(display.crop((950,20,1150,220)))
assert max_pixels<=14336 and len(data)<65536
header='''#ifndef BARRACKS_DOOR_ART_H
#define BARRACKS_DOOR_ART_H
#include <stdint.h>
#include "asset_inflate.h"
typedef struct {int16_t x,y;uint16_t w,h,offset,length;} BarracksDoorFrame;
'''
header+='#define BARRACKS_DOOR_FRAME_TICKS 2u\n'
header+=f'#define BARRACKS_DOOR_FRAME_COUNT {len(records)}u\n#define BARRACKS_DOOR_MAX_PIXELS {max_pixels}u\n'
header+=array('barracks_door_palette','uint16_t',palette)+array('barracks_door_pixels','uint8_t',data)
header+='static const BarracksDoorFrame barracks_door_frames[] = {\n'+''.join(' {'+','.join(map(str,r))+'},\n' for r in records)+'};\n'
header+='''static inline int barracks_door_decode(unsigned frame,uint8_t *out){
 if(frame>=BARRACKS_DOOR_FRAME_COUNT)return 0;
 const BarracksDoorFrame *f=&barracks_door_frames[frame];
 return asset_inflate(barracks_door_pixels+f->offset,f->length,out,(unsigned)f->w*f->h);
}
#endif
'''
(R/'generated/barracks_door_art.h').write_text(header)
(R/'generated/barracks_door_hashes.h').write_text(array('barracks_door_hashes','uint32_t',hashes))
preview[0].save(R/'generated/barracks-door-original.gif',save_all=True,append_images=preview[1:],duration=[600]+[67]*16,loop=0,disposal=2)
audit=dict(source='LEVEL2scene378 doorway317 closedframe1/openlabel14; child31432frames independent of parentstop23',origin=[-500,-714],world_position=[1088.95,175.25],source_trigger='movie366frame161',frame_count=len(records),sample_ticks=2,max_pixels=max_pixels,encoded_bytes=len(data),total_bytes=len(data)+128+len(records)*12,frames=records)
(R/'generated/barracks-door-audit.json').write_text(json.dumps(audit,indent=2));print({k:v for k,v in audit.items() if k!='frames'})

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
