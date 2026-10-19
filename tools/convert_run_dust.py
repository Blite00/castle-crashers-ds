"""Original FX sprite41 dust; exact DS colors, two-bit indices, three source poses."""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1];pal=[0];blob=bytearray();meta=[]
for n in (1,5,9):
 im=Image.open(R/f'assets/run-dust/{n}.png').convert('RGBA').resize((64,64),Image.Resampling.LANCZOS);a=im.getchannel('A').point(lambda a:255 if a>=100 else 0);x,y,r,b=a.getbbox();data=[]
 for rr,gg,bb,aa in im.crop((x,y,r,b)).getdata():
  if aa<100:data.append(0);continue
  c=0x8000|(rr>>3)|((gg>>3)<<5)|((bb>>3)<<10)
  if c not in pal:pal.append(c)
  data.append(pal.index(c))
 assert len(pal)<=4
 off=len(blob)
 for i in range(0,len(data),4):blob.append(sum(v<<(j*2) for j,v in enumerate(data[i:i+4])))
 assert [((blob[off+i//4]>>((i%4)*2))&3) for i in range(len(data))]==data
 meta.append((off,x-32,y-48,r-x,b-y))
s='''#ifndef RUN_DUST_ART_H
#define RUN_DUST_ART_H
#include <stdint.h>
typedef struct {uint16_t offset;int8_t x,y;uint8_t w,h;} RunDustFrame;
'''
s+=f'#define RUN_DUST_MAX_PIXELS {max(m[3]*m[4] for m in meta)}\n'
s+='static const uint16_t run_dust_palette[]={'+','.join(map(str,pal))+'};\n'
s+='static const uint8_t run_dust_pixels[]={'+','.join(map(str,blob))+'};\n'
s+='static const RunDustFrame run_dust_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n'
s+='''static inline int run_dust_decode(unsigned frame,uint8_t *out){
 if(frame>=3||!out)return 0;
 const RunDustFrame *f=&run_dust_frames[frame];
 for(unsigned i=0;i<(unsigned)f->w*f->h;i++)out[i]=(run_dust_pixels[f->offset+i/4]>>((i%4)*2))&3;
 return 1;
}
#endif
'''
(R/'generated/run_dust_art.h').write_text(s);print('Dust bytes',len(blob)+len(pal)*2+len(meta)*6,'meta',meta)
