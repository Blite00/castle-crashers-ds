"""Original cannonball and bomb, plus bounded tiles of the authored fire blast."""
from pathlib import Path
from PIL import Image
import json,zlib
from convert_barbarian import pack,array
R=Path(__file__).resolve().parents[1];ims=[]
for name,sid,count,keys in json.loads((R/'assets/war-cannon-spec.json').read_text()):
 for key in keys:ims.append(Image.open(R/f'assets/war-cannon-{name}/{key}.png').convert('RGBA').resize((512,320),Image.Resampling.LANCZOS))
atlas=Image.new('RGB',(512,320*len(ims)))
for i,im in enumerate(ims):atlas.paste(im,(0,i*320),im.getchannel('A'))
q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT);rgb=q.getpalette();pal=[0]+[0x8000|(rgb[i]>>3)|((rgb[i+1]>>3)<<5)|((rgb[i+2]>>3)<<10) for i in range(0,189,3)]
blob=bytearray();tiles=[];frames=[];preview=Image.new('RGBA',(512,320*len(ims)),(80,80,80,255))
for i,im in enumerate(ims):
 alpha=im.getchannel('A').point(lambda a:255 if a>=96 else 0);box=alpha.getbbox();first=len(tiles)
 if box:
  x,y,x2,y2=box;assert x>0 and y>0 and x2<512 and y2<320,('clipped',i,box)
  indexed=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
  for yy in range(y,y2,64):
   for xx in range(x,x2,32):
    b=(xx,yy,min(xx+32,x2),min(yy+64,y2));a=alpha.crop(b).tobytes();v=indexed.crop(b).tobytes();raw=bytes(p+1 if aa else 0 for p,aa in zip(v,a))
    if not any(raw):continue
    enc=pack(raw) if i<2 else zlib.compress(raw,9)[2:-4];off=len(blob);blob.extend(enc);tiles.append((xx-256,yy-224,b[2]-xx,b[3]-yy,off,len(enc)|(0x80000000 if i<2 else 0)))
 frames.append((first,len(tiles)-first));preview.alpha_composite(im,(0,i*320))
h='#ifndef WAR_CANNON_ART_H\n#define WAR_CANNON_ART_H\n#include <stdint.h>\n#include "asset_inflate.h"\n#define WAR_CANNON_FRAME_COUNT '+str(len(frames))+'u\n#define WAR_CANNON_PIXELS_MAX 2048u\n'
h+='typedef struct {int16_t x,y;uint16_t w,h;uint32_t offset,length;} WarCannonTile;\ntypedef struct {uint16_t first,count;} WarCannonFrame;\n'
h+=array('war_cannon_palette','uint16_t',pal)+array('war_cannon_data','uint8_t',blob)
h+='static const WarCannonTile war_cannon_tiles[]={'+','.join('{'+','.join(map(str,t))+'}' for t in tiles)+'};\n'
h+='static const WarCannonFrame war_cannon_frames[]={'+','.join('{%d,%d}'%f for f in frames)+'};\n'
h+='''static inline int war_cannon_decode_tile(unsigned id,uint8_t*out){
 if(id>=sizeof(war_cannon_tiles)/sizeof(*war_cannon_tiles)||!out)return 0;
 const WarCannonTile*t=&war_cannon_tiles[id];unsigned n=(unsigned)t->w*t->h;
 if(!(t->length&0x80000000u))return asset_inflate(war_cannon_data+t->offset,t->length,out,n);
 unsigned at=t->offset,end=at+(t->length&0x7fffffffu);
 while(n){if(at>=end)return 0;unsigned token=war_cannon_data[at++],run=(token&127)+1;if(run>n)return 0;
  if(token&128){if(at>=end)return 0;unsigned v=war_cannon_data[at++];for(unsigned j=0;j<run;j++)*out++=(uint8_t)v;}
  else {if(run>end-at)return 0;for(unsigned j=0;j<run;j++)*out++=war_cannon_data[at++];}n-=run;
 }return 1;
}
#endif
'''
(R/'generated/war_cannon_art.h').write_text(h);preview.save(R/'generated/war-cannon-preview.png');print('Original cannon bank',len(blob),'bytes',len(tiles),'tiles',frames)
if __name__=='__main__':
 from pack_chapter_assets import build
 build()
