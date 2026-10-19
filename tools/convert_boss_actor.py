"""Original EBARBOSS and LEVEL7 keys, with bounded independently decoded tiles."""
from pathlib import Path
from PIL import Image
import json,zlib,hashlib,struct
R=Path(__file__).resolve().parents[1]
spec=json.loads((R/'assets/boss-actor-spec.json').read_text())
ims=[];source=[];starts=[];counts=[]
for pose,(name,root,count,frames) in enumerate(spec):
 starts.append(len(ims));counts.append(len(frames))
 for n in frames:ims.append(Image.open(R/f'assets/boss-actor-{name}/{n}.png').convert('RGBA').resize((512,384),Image.Resampling.LANCZOS));source.append(n)
atlas=Image.new('RGB',(512,384*len(ims)),(0,0,0))
for i,im in enumerate(ims):atlas.paste(im,(0,i*384),im.getchannel('A'))
# Reserve the original four knight helmet colors and highlights. A global
# median-cut palette otherwise spends every entry on the much larger boss.
knight_colors=[(102,153,255),(255,153,0),(255,0,0),(102,204,102),
               (154,192,254),(254,185,101),(254,122,122),(155,227,149)]
q=atlas.quantize(colors=55,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
p=q.getpalette()[:165]+[v for color in knight_colors for v in color]
q.putpalette(p+[0]*(768-len(p)))
rgb555=[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,189,3)];unique=list(dict.fromkeys(rgb555));remap=[unique.index(v)+1 for v in rgb555];palette=[0]+unique+[0]*(63-len(unique));blob=bytearray();tiles=[];frames=[];seen={};hashes=[];maximum=0
for frame_index,im in enumerate(ims):
 a=im.getchannel('A').point(lambda a:255 if a>=100 else 0);bbox=a.getbbox();first=len(tiles)
 if bbox:
  alpha=a.tobytes();qi=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();raw=bytes(remap[v] if aa else 0 for v,aa in zip(qi,alpha))
  for yy in range(0,384,64):
   for xx in range(0,512,32):
    ww,hh=32,64;part=b''.join(raw[row*512+xx:row*512+xx+ww] for row in range(yy,yy+hh))
    if not any(part):continue
    key=(ww,hh,part)
    if key not in seen:
     if False:
      from convert_barbarian import pack,unpack
      encoded=bytes(pack(part));assert unpack(encoded,0,len(part))==part;flag=0x80000000
     else:
      encoded=zlib.compress(part,9)[2:-4];assert zlib.decompress(encoded,-15)==part;flag=0
     seen[key]=(len(blob),len(encoded)|flag);blob.extend(encoded)
    off,length=seen[key];tiles.append((xx-256,yy-320,ww,hh,off,length));maximum=max(maximum,len(part));hv=2166136261
    for v in part:hv=((hv^v)*16777619)&0xffffffff
    hashes.append(hv)
 frames.append((first,len(tiles)-first))
def arr(n,t,b):return f'static const {t} {n}[{len(b)}]={{'+','.join(map(str,b))+'};\n'
h='''#ifndef BOSS_ACTOR_ART_H
#define BOSS_ACTOR_ART_H
#include <stdint.h>
#include "asset_inflate.h"
typedef struct {int16_t x,y;uint16_t w,h;uint32_t offset,length;} BossActorTile;
typedef struct {uint16_t first,count;} BossActorFrame;
enum BossActorPose {BOSS_ACTOR_IDLE=0,BOSS_ACTOR_WALK=1,BOSS_ACTOR_PUNCH=2,BOSS_ACTOR_SPIKES=3,BOSS_ACTOR_SLAM=4,BOSS_ACTOR_FIRE=5,BOSS_ACTOR_DIE=6,BOSS_ACTOR_CHEST=7,BOSS_ACTOR_CHEST_OPEN=8,BOSS_ACTOR_PRINCESS=9,BOSS_ACTOR_KISS=10,BOSS_ACTOR_CARRIED=11,BOSS_ACTOR_POST=12,BOSS_ACTOR_KNIGHT_BLUE=13,BOSS_ACTOR_KNIGHT_ORANGE=14,BOSS_ACTOR_KNIGHT_RED=15,BOSS_ACTOR_KNIGHT_GREEN=16};
'''
h+=f'#define BOSS_ACTOR_FRAME_COUNT {len(frames)}\n#define BOSS_ACTOR_TILE_COUNT {len(tiles)}\n#define BOSS_ACTOR_PIXELS_MAX {maximum}\n'+arr('boss_actor_palette','uint16_t',palette)+arr('boss_actor_data','uint8_t',blob)+arr('boss_actor_pose_start','uint8_t',starts)+arr('boss_actor_pose_count','uint8_t',counts)+arr('boss_actor_source_frame','uint16_t',source)
h+='static const BossActorTile boss_actor_tiles[]={'+','.join('{'+','.join(map(str,r))+'}' for r in tiles)+'};\n'
h+='static const BossActorFrame boss_actor_frames[]={'+','.join('{%d,%d}'%r for r in frames)+'};\n'
h+='''/* Authored 30 Hz source timelines, with bounded sampled keys. */
static inline unsigned boss_actor_frame_for(unsigned pose,unsigned timer){
 if(pose>=17)pose=0;
 unsigned first=boss_actor_pose_start[pose],count=boss_actor_pose_count[pose],chosen=first;
 if(pose==BOSS_ACTOR_WALK)timer%=36;
 /* Source chest226 removes its final display objects at frame143. */
 if(pose==BOSS_ACTOR_CHEST_OPEN&&timer>=142&&timer<168)timer=168;
 unsigned source=timer<65535?timer+1:65535;
 for(unsigned i=1;i<count;i++)if(boss_actor_source_frame[first+i]<=source)chosen=first+i;
 return chosen;
}
static inline int boss_actor_decode_tile(unsigned tile,uint8_t*out){
 if(tile>=BOSS_ACTOR_TILE_COUNT||!out)return 0;
 const BossActorTile*t=&boss_actor_tiles[tile];unsigned size=t->w*t->h;
 unsigned length=t->length&0x7fffffffu;
 if(size>BOSS_ACTOR_PIXELS_MAX||t->offset+length>sizeof(boss_actor_data))return 0;
 if(t->length&0x80000000u){
  const uint8_t*p=boss_actor_data+t->offset,*end=p+length;unsigned done=0;
  while(done<size){if(p==end)return 0;unsigned token=*p++,n=(token&127)+1;if(n>size-done)return 0;
   if(token&128){if(p==end)return 0;memset(out+done,*p++,n);}else{if((unsigned)(end-p)<n)return 0;memcpy(out+done,p,n);p+=n;}done+=n;
  }return p==end;
 }
 return asset_inflate(boss_actor_data+t->offset,length,out,size);
}
#endif
'''
(R/'generated/boss_actor_art.h').write_text(h);(R/'generated/boss-actor-hashes.bin').write_bytes(struct.pack('<'+str(len(hashes))+'I',*hashes));(R/'generated/boss-actor-audit.json').write_text(json.dumps(dict(source='EBARBOSS.swf',poses=spec,frames=len(frames),tiles=len(tiles),unique_tiles=len(seen),payload_bytes=len(blob),max_tile_pixels=maximum,palette_colors=len(unique),source_sha256=hashlib.sha256((R/'assets/extracted/EBARBOSS.swf').read_bytes()).hexdigest(),level7_sha256=hashlib.sha256((R/'assets/extracted/LEVEL7-full.swf').read_bytes()).hexdigest(),palette_limit=63,tile_codec='raw-deflate',source_time_hz=30,origin=[-512,-640],scale=0.5),indent=2));print('Machine payload',len(blob),'frames',len(frames),'tiles',len(tiles),'unique',len(seen),'maxpixels',maximum)
