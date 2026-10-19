from pathlib import Path
from PIL import Image
import zlib,json
R=Path(__file__).resolve().parents[1];ims=[Image.open(R/f'assets/shop-boards/{i}.png').convert('RGBA').resize((100,100),Image.Resampling.LANCZOS) for i in range(1,11)];atlas=Image.new('RGB',(1000,100))
for i,im in enumerate(ims):atlas.paste(im,(i*100,0),im)
q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT);p=q.getpalette();pal=[0]+[32768|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,189,3)]
unique=list(dict.fromkeys(pal[1:]));remap=[0]+[unique.index(v)+1 for v in pal[1:]];pal=[0]+unique+[0]*(63-len(unique))
blob=bytearray();meta=[];seen={};hashes=[]
for im in ims:
 b=im.getchannel('A').point(lambda a:255 if a>=100 else 0).getbbox();x,y,r,bb=b;im=im.crop(b);c=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);raw=bytes(remap[v+1] if a>=100 else 0 for v,a in zip(c.tobytes(),im.getchannel('A').tobytes()));compressed=zlib.compress(raw,9)[2:-4]
 if raw not in seen:seen[raw]=len(blob);blob.extend(compressed)
 meta.append((seen[raw],len(compressed),x-40,y-80,r-x,bb-y));assert zlib.decompress(compressed,-15)==raw
 h=2166136261
 for v in raw:h=((h^v)*16777619)&0xffffffff
 hashes.append(h)
s='''#ifndef SHOP_BOARD_ART_H
#define SHOP_BOARD_ART_H
#include <stdint.h>
#include "asset_inflate.h"
/* LEVEL19 shop140 modes2/3/5/6/7; loot106 potion9; source PLAYER skinny2.
   Original controller glyph selector129 frame3 supplies the DS A prompt. */
typedef struct {uint16_t offset,length;int8_t x,y;uint8_t w,h;} ShopBoardFrame;
'''
s+=f'#define SHOP_BOARD_MAX_PIXELS {max(m[4]*m[5] for m in meta)}\n'
s+='static const uint16_t shop_board_palette[]={'+','.join(map(str,pal))+'};\nstatic const uint8_t shop_board_pixels[]={'+','.join(map(str,blob))+'};\nstatic const ShopBoardFrame shop_board_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n'
s+='''static inline int shop_board_decode(unsigned frame,uint8_t *out){
 if(frame>=10||!out)return 0;
 const ShopBoardFrame *f=&shop_board_frames[frame];
 return asset_inflate(shop_board_pixels+f->offset,f->length,out,f->w*f->h);
}
#endif
''';(R/'generated/shop_board_art.h').write_text(s);(R/'generated/shop_board_hashes.h').write_text('static const uint32_t shop_board_hashes[]={'+','.join(str(v)+'u' for v in hashes)+'};\n');atlas.save(R/'generated/shop-boards-preview.png');print('Boards total',len(blob)+128+len(meta)*8,'pixelsmax',max(m[4]*m[5] for m in meta))

from pack_shop_board_deltas import optimize
optimize()

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
