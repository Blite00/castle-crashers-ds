"""Small original level beam/title bank, independently bounded DEFLATE."""
from pathlib import Path
from PIL import Image
import zlib,json,struct,sys
R=Path(__file__).resolve().parents[1];ims=[];meta=[]
for n in [('level' if '--beam' in sys.argv else 'level-label'),'combo']:
 im=Image.open(R/f'assets/progression-{n}/1.png').convert('RGBA')
 if n=='level':im=im.resize((256,192),Image.Resampling.LANCZOS)
 a=im.getchannel('A').point(lambda v:255 if v>=35 else 0);b=a.getbbox();im=im.crop(b);ims.append(im)
 meta.append((b[0]-128,b[1]-96-130) if n=='level' else (0,0))
# The beam is intentionally translucent in source. Preserve palegray/white;
# runtime can use its existing sprite opacity envelope. Original NewCombo is black.
train=Image.new('RGB',(256,sum(im.height for im in ims)),(0,0,0));y=0
for im in ims:train.paste(im.convert('RGB'),(0,y));y+=im.height
q=train.quantize(colors=15,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette();palette=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,45,3)];blob=bytearray();frames=[];hashes=[]
for im,(x,y) in zip(ims,meta):
 a=im.getchannel('A').tobytes();v=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();raw=bytes(c+1 if aa>=35 else 0 for c,aa in zip(v,a));compressed=zlib.compress(raw,9)[2:-4];assert zlib.decompress(compressed,-15)==raw
 frames.append((len(blob),len(compressed),x,y,im.width,im.height));blob.extend(compressed);hv=2166136261
 for c in raw:hv=((hv^palette[c])*16777619)&0xffffffff
 hashes.append(hv)
def arr(n,t,a):return f'static const {t} {n}[{len(a)}]={{'+','.join(map(str,a))+'};\n'
h='''#ifndef PROGRESSION_ART_H
#define PROGRESSION_ART_H
#include <stdint.h>
#include "asset_inflate.h"
typedef struct {uint16_t offset,length;int16_t x,y;uint16_t w,h;} ProgressionArtFrame;
enum {PROGRESSION_LEVEL_LABEL=0,PROGRESSION_NEW_COMBO=1};
''';h+=f'#define PROGRESSION_ART_MAX_PIXELS {max(im.width*im.height for im in ims)}\n'+arr('progression_palette','uint16_t',palette)+arr('progression_data','uint8_t',blob)
h+='static const ProgressionArtFrame progression_frames[2]={'+','.join('{'+','.join(map(str,f))+'}' for f in frames)+'};\n'
h+='''static inline int progression_decode(unsigned frame,uint8_t*out){
 if(frame>=2||!out)return 0;
 const ProgressionArtFrame*f=&progression_frames[frame];
 return asset_inflate(progression_data+f->offset,f->length,out,f->w*f->h);
}
#endif
''';(R/'generated/progression_art.h').write_text(h);(R/'generated/progression-art-hashes.bin').write_bytes(struct.pack('<2I',*hashes));(R/'generated/progression-art-audit.json').write_text(json.dumps(dict(source=['PLAYER29 LEVEL label; originalfont24,numberdynamic','MAIN DefineEditText342,New Combo'],frames=frames,bytes=len(blob)+32+24),indent=2));print('Progression static',len(blob)+56,'frames',frames)
