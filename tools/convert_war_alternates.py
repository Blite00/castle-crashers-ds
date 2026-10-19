"""Original fence frame2, palette-matched to the continuous War scene."""
from pathlib import Path
from PIL import Image
import json,re,zlib
R=Path(__file__).resolve().parents[1]
s=(R/'generated/war_art.h').read_text();pal=list(map(int,re.search(r'campaign_war_palette\[256\]=\{(.*?)\};',s,re.S)[1].replace('\n','').strip(',').split(',')))
q=Image.new('P',(1,1));rgb=[]
for c in pal:rgb.extend(((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31))
q.putpalette(rgb);blob=bytearray();records=[]
for i,m in enumerate(json.loads((R/'assets/war-alternates.json').read_text())):
 im=Image.open(R/f'assets/war-alternates/{i+1}.png').convert('RGBA').resize((4608,448),Image.Resampling.LANCZOS);box=im.getchannel('A').point(lambda a:255 if a>=96 else 0).getbbox()
 if not box:continue
 im=im.crop(box);w,h=im.size;alpha=im.getchannel('A').tobytes();data=bytes(v if a>=96 else 0 for v,a in zip(im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes(),alpha))
 for x in range(0,w,32):
  ww=min(32,w-x);raw=b''.join(data[y*w+x:y*w+x+ww] for y in range(h));enc=zlib.compress(raw,9)[2:-4];assert zlib.decompress(enc,-15)==raw
  records.append((box[0]+x,box[1],ww,h,m['depth'],len(blob),len(enc),m['source'],m['source_depth'],1+m.get('keyframe',0)));blob.extend(enc)
# Original FX door_shrapnel chips (four shapes) at DS scale, used by gate hit feedback.
for n in range(4):
 im=Image.open(R/f'assets/war-gate-chip/{n+1}.png').convert('RGBA');box=im.getchannel('A').point(lambda a:255 if a>=96 else 0).getbbox();im=im.crop(box)
 im=im.resize((max(1,round(im.size[0]*0.5)),max(1,round(im.size[1]*0.5))),Image.Resampling.LANCZOS);alpha=im.getchannel('A').tobytes();w,h=im.size
 raw=bytes(v if a>=96 else 0 for v,a in zip(im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes(),alpha));enc=zlib.compress(raw,9)[2:-4]
 assert zlib.decompress(enc,-15)==raw
 records.append((0,0,w,h,-1,len(blob),len(enc),1180,n,32+n));blob.extend(enc)
assert max(r[2]*r[3] for r in records)<=14336
h='#ifndef WAR_ALTERNATE_ART_H\n#define WAR_ALTERNATE_ART_H\n#include "route_art_types.h"\n#include "asset_inflate.h"\n#define WAR_ALTERNATE_COUNT '+str(len(records))+'u\n'
h+='static const CampaignRouteLayer war_alternate_layers[]={'+','.join('{'+','.join(map(str,r))+'}' for r in records)+'};\n'
h+='static const uint8_t war_alternate_data['+str(len(blob))+']={\n'+''.join(','.join(map(str,blob[i:i+32]))+',\n' for i in range(0,len(blob),32))+'};\n'
h+='''static inline int war_alternate_decode(unsigned index,uint8_t*out){
 if(index>=WAR_ALTERNATE_COUNT||!out)return 0;
 const CampaignRouteLayer *f=&war_alternate_layers[index];
 return asset_inflate(war_alternate_data+f->offset,f->length,out,(unsigned)f->w*f->h);
}
#endif
'''
(R/'generated/war_alternate_art.h').write_text(h);print('Broken fence slices',len(records),'bytes',len(blob))

if __name__ == "__main__" and "--prepare" not in __import__("sys").argv:
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
