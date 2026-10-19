"""Sharp half-scale continuous Keep strips with separately depth-sorted original props."""
from pathlib import Path
from collections import defaultdict,deque
from PIL import Image
import json,hashlib
R=Path(__file__).resolve().parents[1]
W,H,SW=2448,448,32
# Bounded raw DEFLATE: each strip is independent and uses the caller's output as its window.
def pack(data):
 from asset_codec import pack as encode
 return encode(data)

def unpack(data):
 from asset_codec import unpack as decode
 return decode(data)

def arr(name,typ,data):return f'static const {typ} {name}[{len(data)}]={{\n'+''.join(','.join(map(str,data[i:i+32]))+',\n' for i in range(0,len(data),32))+'};\n'
fg=Image.open(R/'assets/campaign-world/1.png').convert('RGBA').resize((W,H),Image.Resampling.LANCZOS)
px=fg.load()
for x in range(W):
 y=H-1
 while y>=0 and px[x,y][3]<250:y-=1
 if y>=H//2:
  edge=px[x,y]
  for k in range(y+1,H):px[x,k]=edge
sky=Image.open(R/'assets/campaign-sky-images/1.png').convert('RGBA').resize((W,W//2),Image.Resampling.LANCZOS)
bg=sky.crop((0,0,W,H));bg.alpha_composite(fg)
q=bg.convert('RGB').quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
p=q.getpalette();rgb555=[0x8000|(p[j]>>3)|((p[j+1]>>3)<<5)|((p[j+2]>>3)<<10) for j in range(0,384,3)]
# Merge only palette entries already identical on DS; visible pixels are unchanged.
unique=list(dict.fromkeys(rgb555));remap=[unique.index(v)+1 for v in rgb555];pal=[0]+unique+[0]*(255-len(unique))
q.convert('RGB').save(R/'generated/campaign-world.png')
pixels=bytes(remap[v] for v in q.getdata());(R/'assets/campaign-world-reference.bin').write_bytes(pixels)
blocks=[];blob=bytearray();offsets=[0]
for x in range(0,W,SW):
 chunk=bytearray()
 for y in range(H):
  line=pixels[y*W+x:y*W+min(W,x+SW)];chunk.extend(line);chunk.extend([line[-1]]*(SW-len(line)))
 b=pack(chunk);assert unpack(b)==chunk;blob.extend(b);offsets.append(len(blob))
# Props remain in the composed terrain and redraw above actors according to their foot depth.
meta=json.loads((R/'assets/campaign-world-layers.json').read_text());layers=[]
for i,m in enumerate(meta):
 im=Image.open(R/f'assets/campaign-world-fg/{i+1}.png').convert('RGBA').resize((W,H),Image.Resampling.LANCZOS)
 if m['shape'] in (68,79):
  im.paste((0,0,0,0),(0,0,W,307)) # side faces only; never redraw the walkable floor over fighters
 bbox=im.getchannel('A').point(lambda a:255 if a>=96 else 0).getbbox()
 if not bbox:continue
 x,y,x2,y2=bbox;im=im.crop(bbox);qp=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
 alpha=im.getchannel('A').tobytes();data=bytes(remap[v] if a>=96 else 0 for v,a in zip(qp.tobytes(),alpha));b=pack(data);assert unpack(b)==data
 depth=y2-3
 if m['shape'] in (68,79):depth=307
 if m['shape'] in (91,101,102,103,384):depth=round(m['placement_y'])
 layers.append((depth,x,y,x2-x,y2-y,b,data,m['shape']))
# Stacked column pieces share the bottom piece's depth, so a jumping actor cannot
# punch through an upper segment of the same pillar.
layers=[(max(b[0] for b in layers if b[7]==57 and abs(b[1]-a[1])<=8),*a[1:]) if a[7]==57 else a for a in layers]
# Bound runtime scratch memory by slicing large assemblies into narrow strips.
sliced=[]
for depth,x,y,w,h,b,data,sid in layers:
 if w*h<=14336:sliced.append((depth,x,y,w,h,b,data,sid));continue
 for xx in range(0,w,32):
  ww=min(32,w-xx);part=b''.join(data[row*w+xx:row*w+xx+ww] for row in range(h))
  sliced.append((depth,x+xx,y,ww,h,pack(part),part,sid))
layers=sliced;layers.sort();fgblob=bytearray();records=[]
for depth,x,y,w,h,b,data,sid in layers:
 records.append((x,y,w,h,depth,len(fgblob),len(b)));fgblob.extend(b)
header='''/* Original LEVEL13 + SKY1. Sharp half-source-resolution terrain, independently compressed strips. */
#ifndef CAMPAIGN_WORLD_H
#define CAMPAIGN_WORLD_H
#include <stdint.h>
#define CAMPAIGN_WORLD_WIDTH 2448
#define CAMPAIGN_WORLD_HEIGHT 448
#define CAMPAIGN_WORLD_STRIP_WIDTH 32
#define CAMPAIGN_WORLD_STRIP_COUNT 77
#define CAMPAIGN_WORLD_STRIP_PIXELS (32*448)
'''
header+=arr('campaign_world_palette','uint16_t',pal)+arr('campaign_world_strip_offsets','uint32_t',offsets)+arr('campaign_world_strip_data','uint8_t',blob)
header+='typedef struct {int16_t x,y,w,h,depth;uint32_t offset,length;} CampaignWorldLayer;\n'
header+=f'#define CAMPAIGN_WORLD_FG_COUNT {len(records)}\n#define CAMPAIGN_WORLD_FG_MAX_PIXELS {max(a[2]*a[3] for a in records)}\n'
header+='static const CampaignWorldLayer campaign_world_fg_layers[]={'+','.join('{'+','.join(map(str,a))+'}' for a in records)+'};\n'+arr('campaign_world_fg_data','uint8_t',fgblob)
header+='''#include "asset_inflate.h"
static inline int campaign_world_decode_bytes(const uint8_t *p,unsigned length,uint8_t *out,unsigned size){
 return asset_inflate(p,length,out,size);
}
static inline int campaign_world_decode_strip(unsigned strip,uint8_t *out){
 if(strip>=CAMPAIGN_WORLD_STRIP_COUNT)return 0;
 unsigned offset=campaign_world_strip_offsets[strip];return campaign_world_decode_bytes(campaign_world_strip_data+offset,campaign_world_strip_offsets[strip+1]-offset,out,CAMPAIGN_WORLD_STRIP_PIXELS);
}
static inline int campaign_world_decode_fg(unsigned layer,uint8_t *out){
 if(layer>=CAMPAIGN_WORLD_FG_COUNT)return 0;
 const CampaignWorldLayer *l=&campaign_world_fg_layers[layer];
 return campaign_world_decode_bytes(campaign_world_fg_data+l->offset,l->length,out,l->w*l->h);
}
#endif
'''
(R/'generated/campaign_world.h').write_text(header)
(R/'assets/campaign-world-metadata.json').write_text(json.dumps({'width':W,'height':H,'strip_width':SW,'palette_colors':128,'terrain_bytes':len(blob),'foreground_bytes':len(fgblob),'layers':[{'shape':a[7],'x':a[1],'y':a[2],'w':a[3],'h':a[4],'depth':a[0]} for a in layers],'terrain_sha256':hashlib.sha256(pixels).hexdigest()},indent=2))
print('Terrain',len(blob),'FG',len(fgblob),'layers',len(layers),'largestFG',max(a[2]*a[3] for a in records),'total',len(blob)+len(fgblob)+len(pal)*2)


from pack_campaign_rows import repack
repack("world")

# Static foreground exactly matches its depth-layer redraw colors.
import runpy
runpy.run_path(str(R/'tools/bake_campaign_foreground.py'))

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
