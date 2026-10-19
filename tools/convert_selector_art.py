from pathlib import Path
from PIL import Image
import json
R=Path(__file__).resolve().parents[1];ims=[]
for n in [9,2,1,11]:ims.append(Image.open(R/f'assets/item-selector/{n}.png').convert('RGBA').resize((48,48),Image.Resampling.LANCZOS))
for n in [*range(2,10),*range(11,19)]:ims.append(Image.open(R/f'assets/item-popups/{n}.png').convert('RGBA').resize((40,40),Image.Resampling.LANCZOS))
atlas=Image.new('RGB',(48,48*len(ims)))
for i,im in enumerate(ims):atlas.paste(im,(0,i*48),im)
q=atlas.quantize(colors=15,method=Image.Quantize.MEDIANCUT);p=q.getpalette();pal=[0]+[32768|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,45,3)]
data=[];meta=[];hashes=[]
for im in ims:
 a=im.getchannel('A').point(lambda v:255 if v>=60 else 0);x,y,r,b=a.getbbox();v=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);raw=[v.getpixel((xx,yy))+1 if a.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r)]
 start=len(data);i=0
 while i<len(raw):
  count=1
  while i+count<len(raw) and count<16 and raw[i+count]==raw[i]:count+=1
  data.append(((count-1)<<4)|raw[i]);i+=count
 meta.append((start,len(data)-start,x-im.width//2,y-im.height//2,r-x,b-y))
 hv=2166136261
 for z in raw:hv=((hv^pal[z])*16777619)&0xffffffff
 hashes.append(hv)
def arr(n,t,v):return 'const '+t+' '+n+'[]={'+','.join(map(str,v))+'};\n'
h="""#ifndef SELECTOR_ART_H
#define SELECTOR_ART_H
#include <stdint.h>
#define SELECTOR_FRAME_COUNT 20
#define SELECTOR_MAX_PIXELS 1600
typedef struct {uint16_t offset,size;int8_t x,y;uint8_t w,h;} SelectorFrame;
extern const SelectorFrame selector_frames[SELECTOR_FRAME_COUNT];
extern const uint16_t selector_palette[16];
int selector_decode(unsigned frame,uint8_t *out);
#endif
"""
c='#include "selector_art.h"\n'+arr('selector_palette','uint16_t',pal)+'static '+arr('selector_data','uint8_t',data)+'const SelectorFrame selector_frames[]={'+','.join('{'+','.join(map(str,f))+'}' for f in meta)+'};\n'
c+="""int selector_decode(unsigned id,uint8_t*out){
 if(id>=SELECTOR_FRAME_COUNT||!out)return 0;
 const SelectorFrame*f=&selector_frames[id];unsigned n=0,total=f->w*f->h;
 if(total>SELECTOR_MAX_PIXELS)return 0;
 for(unsigned i=0;i<f->size;i++){unsigned v=selector_data[f->offset+i],count=(v>>4)+1;if(n+count>total)return 0;while(count--)out[n++]=v&15;}
 return n==total;
}
"""
(R/'generated/selector_art.h').write_text(h);(R/'generated/selector_art.c').write_text(c)
(R/'generated/selector-art-audit.json').write_text(json.dumps(dict(source='PLAYER305 frames9,2,1,11 and PLAYER316/269 selection pulse',frames=meta,hashes=hashes,bytes=len(data)+32+len(meta)*8),indent=2))
atlas.save(R/'build/selector-art-source.png');print('Selector bytes',len(data)+32+len(meta)*8)
