"""Bank adapted-color source map as 32x64 independently compressed tiles."""
from pathlib import Path
from PIL import Image
import json,zlib
R=Path(__file__).resolve().parents[1]
im=Image.open(R/'assets/original-worldmap/1.png').convert('RGBA').resize((1792,1280),Image.Resampling.LANCZOS).crop((128,736,800,1184))
bg=Image.new('RGB',im.size,(199,220,150));bg.paste(im,mask=im.getchannel('A'));q=bg.quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
rgb=q.getpalette();pal=[0x8000|(rgb[i]>>3)|((rgb[i+1]>>3)<<5)|((rgb[i+2]>>3)<<10) for i in range(0,384,3)];raw=q.tobytes();blob=bytearray();offsets=[]
for x in range(0,672,32):
 for y in range(0,448,64):
  b=b''.join(raw[yy*672+x:yy*672+x+32] for yy in range(y,y+64));v=zlib.compress(b,9)[2:-4];assert zlib.decompress(v,-15)==b;offsets.append(len(blob));blob.extend(v)
offsets.append(len(blob))
def arr(name,kind,b):return f'static const {kind} {name}[{len(b)}]={{\n'+''.join(','.join(map(str,b[i:i+32]))+',\n' for i in range(0,len(b),32))+'};\n'
h='#ifndef ORIGINAL_WORLDMAP_H\n#define ORIGINAL_WORLDMAP_H\n#include <stdint.h>\n#include <string.h>\n#include "asset_inflate.h"\n#define ORIGINAL_WORLDMAP_WIDTH 672u\n#define ORIGINAL_WORLDMAP_HEIGHT 448u\n#define ORIGINAL_WORLDMAP_ORIGIN_X 128\n#define ORIGINAL_WORLDMAP_ORIGIN_Y 864\n'
h+=arr('original_worldmap_palette','uint16_t',pal)+arr('original_worldmap_offsets','uint32_t',offsets)+arr('original_worldmap_data','uint8_t',blob)
h+='''static inline int original_worldmap_decode_rows(unsigned strip,unsigned first,unsigned count,uint8_t*out){
 if(!out||strip>=21||first>448||count>448-first)return 0;
 uint8_t tile[2048];unsigned end=first+count;
 while(first<end){unsigned c=first/64,within=first%64,n=64-within;
  if(n>end-first)n=end-first;
  unsigned id=strip*7+c,off=original_worldmap_offsets[id];
  if(!asset_inflate(original_worldmap_data+off,original_worldmap_offsets[id+1]-off,tile,2048))return 0;
  memcpy(out,tile+within*32,n*32);out+=n*32;first+=n;
 }return 1;
}
#endif
'''
(R/'generated/original_worldmap.h').write_text(h);bg.save(R/'generated/original-worldmap-preview.png');(R/'assets/worldmap-reference.bin').write_bytes(raw)
print('Original western map:',len(blob),'bytes, nodes crop origin128,736 (rows 128 above the old crop reach the Flooded Castle node)')

if __name__ == "__main__" and "--prepare" not in __import__("sys").argv:
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
