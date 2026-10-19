"""Original hay-cart intact/hit/death frames plus five original hay fragments."""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
ims=[]
for i in range(14):
 path=R/f'assets/campaign-breakables/{i+1}.png' if i<9 else R/f'assets/campaign-hay/{i-8}.png'
 im=Image.open(path).convert('RGBA');im=im.resize((im.width//2,im.height//2),Image.Resampling.LANCZOS);ims.append(im)
sheet=Image.new('RGB',(160*14,160))
for i,im in enumerate(ims):sheet.paste(im.convert('RGB'),(i*160,0))
q=sheet.quantize(colors=128,dither=Image.Dither.NONE);p=q.getpalette();rawpal=[0x8000|(p[j]>>3)|((p[j+1]>>3)<<5)|((p[j+2]>>3)<<10) for j in range(0,384,3)];unique=list(dict.fromkeys(rawpal));remap=[unique.index(v)+1 for v in rawpal];pal=[0]+unique+[0]*(255-len(unique))
def rle(data):
 out=bytearray();i=0
 while i<len(data):
  run=1
  while i+run<len(data) and data[i+run]==data[i] and run<128:run+=1
  if run>=3:out.extend((128+run-1,data[i]));i+=run
  else:
   start=i;i+=run
   while i<len(data) and i-start<128:
    if i+2<len(data) and data[i]==data[i+1]==data[i+2]:break
    i+=1
   out.append(i-start-1);out.extend(data[start:i])
 return out
blob=bytearray();frames=[]
for im in ims:
 box=im.getchannel('A').point(lambda a:255 if a>=96 else 0).getbbox();assert box
 x,y,x2,y2=box;crop=im.crop(box);qi=crop.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
 data=bytes(remap[v] if a>=96 else 0 for v,a in zip(qi.tobytes(),crop.getchannel('A').tobytes()));packed=rle(data)
 frames.append((x-im.width//2,y-im.height//2,x2-x,y2-y,len(blob),len(packed)));blob.extend(packed)
def arr(name,typ,data):return f'static const {typ} {name}[{len(data)}]={{'+','.join(map(str,data))+'};\n'
h='''#ifndef CAMPAIGN_BREAKABLES_H
#define CAMPAIGN_BREAKABLES_H
#include <stdint.h>
/* Original LEVEL13 hay cart, at its original world position; original HP=4. */
#define CAMPAIGN_BREAKABLE_COUNT 1
#define CAMPAIGN_BREAKABLE_FRAME_COUNT 14
#define CAMPAIGN_BREAKABLE_INTACT_FRAME 0
#define CAMPAIGN_BREAKABLE_HIT_FRAME 1
#define CAMPAIGN_BREAKABLE_BREAK_FIRST 2
#define CAMPAIGN_BREAKABLE_BREAK_COUNT 7
#define CAMPAIGN_BREAKABLE_DEBRIS_FIRST 9
#define CAMPAIGN_BREAKABLE_DEBRIS_COUNT 5
typedef struct {int16_t x,y;uint8_t hp,radius_x,radius_y;} CampaignBreakable;
static const CampaignBreakable campaign_breakable_objects[1]={{917,333,4,25,8}};
typedef struct {int16_t x,y,w,h;uint32_t offset,length;} CampaignBreakableFrame;
'''
h+=f'#define CAMPAIGN_BREAKABLE_MAX_PIXELS {max(a[2]*a[3] for a in frames)}\n'+arr('campaign_breakable_palette','uint16_t',pal)+arr('campaign_breakable_data','uint8_t',blob)
h+='static const CampaignBreakableFrame campaign_breakable_frames[]={'+','.join('{'+','.join(map(str,a))+'}' for a in frames)+'};\n'
h+='''static inline int campaign_breakable_decode(unsigned frame,uint8_t *out){
 if(frame>=CAMPAIGN_BREAKABLE_FRAME_COUNT||!out)return 0;
 const CampaignBreakableFrame *f=&campaign_breakable_frames[frame];const uint8_t *p=campaign_breakable_data+f->offset,*end=p+f->length;unsigned n=0,size=f->w*f->h;
 while(p<end&&n<size){unsigned t=*p++,count=(t&127)+1;if(count>size-n)return 0;
  if(t&128){if(p==end)return 0;uint8_t v=*p++;while(count--)out[n++]=v;}
  else{if((unsigned)(end-p)<count)return 0;while(count--)out[n++]=*p++;}
 }return n==size&&p==end;
}
#endif
'''
(R/'generated/campaign_breakables.h').write_text(h);print('Breakable art',len(blob)+512,'bytes; maxdecode',max(a[2]*a[3] for a in frames),'pixels')

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
