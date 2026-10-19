"""Original EWARMACH keyframes, cropped into bounded independently decoded tiles."""
from pathlib import Path
from PIL import Image
import json,zlib,hashlib,struct
R=Path(__file__).resolve().parents[1]
spec=json.loads((R/'assets/war-machine-spec.json').read_text());spec[2][3][-1]=120
ims=[];source=[];starts=[];counts=[]
for pose,(name,root,count,frames) in enumerate(spec):
 starts.append(len(ims));counts.append(len(frames))
 for n in frames:ims.append(Image.open(R/f'assets/war-machine-{name}/{n}.png').convert('RGBA').resize((384,320),Image.Resampling.LANCZOS));source.append(n)
atlas=Image.new('RGB',(384,320*len(ims)),(0,0,0))
for i,im in enumerate(ims):atlas.paste(im,(0,i*320),im.getchannel('A'))
q=atlas.quantize(colors=127,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette();rgb555=[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,381,3)];unique=list(dict.fromkeys(rgb555));remap=[unique.index(v)+1 for v in rgb555];palette=[0]+unique+[0]*(127-len(unique));blob=bytearray();tiles=[];frames=[];seen={};hashes=[];maximum=0
for frame_index,im in enumerate(ims):
 a=im.getchannel('A').point(lambda a:255 if a>=100 else 0);bbox=a.getbbox();first=len(tiles)
 if bbox:
  alpha=a.tobytes();qi=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();raw=bytes(remap[v] if aa else 0 for v,aa in zip(qi,alpha))
  for yy in range(0,320,64):
   for xx in range(0,384,32):
    ww,hh=32,64;part=b''.join(raw[row*384+xx:row*384+xx+ww] for row in range(yy,yy+hh))
    if not any(part):continue
    key=(ww,hh,part)
    if key not in seen:
     if frame_index<starts[2]:
      from convert_barbarian import pack,unpack
      encoded=bytes(pack(part));assert unpack(encoded,0,len(part))==part;flag=0x80000000
     else:
      encoded=zlib.compress(part,9)[2:-4];assert zlib.decompress(encoded,-15)==part;flag=0
     seen[key]=(len(blob),len(encoded)|flag);blob.extend(encoded)
    off,length=seen[key];tiles.append((xx-192,yy-280,ww,hh,off,length));maximum=max(maximum,len(part));hv=2166136261
    for v in part:hv=((hv^v)*16777619)&0xffffffff
    hashes.append(hv)
 frames.append((first,len(tiles)-first))
def arr(n,t,b):return f'static const {t} {n}[{len(b)}]={{'+','.join(map(str,b))+'};\n'
h='''#ifndef WAR_MACHINE_ART_H
#define WAR_MACHINE_ART_H
#include <stdint.h>
#include "asset_inflate.h"
typedef struct {int16_t x,y;uint16_t w,h;uint32_t offset,length;} WarMachineTile;
typedef struct {uint16_t first,count;} WarMachineFrame;
enum WarMachinePose {WAR_MACHINE_IDLE=0,WAR_MACHINE_WALK=1,WAR_MACHINE_JACK=2,WAR_MACHINE_DIE=3};
'''
h+=f'#define WAR_MACHINE_FRAME_COUNT {len(frames)}\n#define WAR_MACHINE_TILE_COUNT {len(tiles)}\n#define WAR_MACHINE_PIXELS_MAX {maximum}\n'+arr('war_machine_palette','uint16_t',palette)+arr('war_machine_data','uint8_t',blob)+arr('war_machine_pose_start','uint8_t',starts)+arr('war_machine_pose_count','uint8_t',counts)+arr('war_machine_source_frame','uint8_t',source)
h+='static const WarMachineTile war_machine_tiles[]={'+','.join('{'+','.join(map(str,r))+'}' for r in tiles)+'};\n'
h+='static const WarMachineFrame war_machine_frames[]={'+','.join('{%d,%d}'%r for r in frames)+'};\n'
h+='''/* Timer is zero-based 30 Hz source time. Frame121 of jack returns to idle
 * in source code; sample120 retains the complete recovered body. */
static inline unsigned war_machine_frame_for(unsigned pose,unsigned timer){
 if(pose>=4)pose=0;
 unsigned first=war_machine_pose_start[pose],count=war_machine_pose_count[pose],chosen=first;
 if(pose==WAR_MACHINE_WALK)timer%=16;
 unsigned source=timer<65535?timer+1:65535;
 for(unsigned i=1;i<count;i++)if(war_machine_source_frame[first+i]<=source)chosen=first+i;
 return chosen;
}
static inline int war_machine_decode_tile(unsigned tile,uint8_t*out){
 if(tile>=WAR_MACHINE_TILE_COUNT||!out)return 0;
 const WarMachineTile*t=&war_machine_tiles[tile];unsigned size=t->w*t->h;
 unsigned length=t->length&0x7fffffffu;
 if(size>WAR_MACHINE_PIXELS_MAX||t->offset+length>sizeof(war_machine_data))return 0;
 if(t->length&0x80000000u){
  const uint8_t*p=war_machine_data+t->offset,*end=p+length;unsigned done=0;
  while(done<size){if(p==end)return 0;unsigned token=*p++,n=(token&127)+1;if(n>size-done)return 0;
   if(token&128){if(p==end)return 0;memset(out+done,*p++,n);}else{if((unsigned)(end-p)<n)return 0;memcpy(out+done,p,n);p+=n;}done+=n;
  }return p==end;
 }
 return asset_inflate(war_machine_data+t->offset,length,out,size);
}
#endif
'''
(R/'generated/war_machine_art.h').write_text(h);(R/'generated/war-machine-hashes.bin').write_bytes(struct.pack('<'+str(len(hashes))+'I',*hashes));(R/'generated/war-machine-audit.json').write_text(json.dumps(dict(source='EWARMACH.swf',poses=spec,frames=len(frames),tiles=len(tiles),unique_tiles=len(seen),payload_bytes=len(blob),max_tile_pixels=maximum,palette_colors=len(unique),source_sha256=hashlib.sha256((R/'assets/extracted/EWARMACH.swf').read_bytes()).hexdigest()),indent=2));print('Machine payload',len(blob),'frames',len(frames),'tiles',len(tiles),'unique',len(seen),'maxpixels',maximum)

if __name__ == "__main__":
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
