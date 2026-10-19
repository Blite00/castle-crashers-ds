"""Original LEVEL2 sprite45 front jamb/lintel mask; colors come from streamed terrain."""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
p=R/'assets/barracks-doorframe/1.png'
if not p.exists():p=R/'assets/door-frame-audit/7.png'
im=Image.open(p).convert('RGBA').resize((1280,448),Image.Resampling.LANCZOS)
a=im.getchannel('A').point(lambda x:255 if x>=96 else 0);box=a.getbbox();x,y,r,b=box;w=r-x;h=b-y
bits=bytearray((w*h+7)//8)
for i,v in enumerate(a.crop(box).getdata()):
 if v:bits[i//8]|=1<<(i%8)
s='''#ifndef BARRACKS_DOORFRAME_ART_H
#define BARRACKS_DOORFRAME_ART_H
#include <stdint.h>
/* Genuine source sprite45. Transparent opening stays unmasked.
   Redraw terrain at these world pixels after a hero enters the upstairs doorway. */
'''
s+=f'#define BARRACKS_DOORFRAME_X {x}\n#define BARRACKS_DOORFRAME_Y {y}\n#define BARRACKS_DOORFRAME_W {w}\n#define BARRACKS_DOORFRAME_H {h}\n'
s+='static const uint8_t barracks_doorframe_mask[]={'+','.join(map(str,bits))+'};\n'
s+='''static inline int barracks_doorframe_pixel(int x,int y){
 x-=BARRACKS_DOORFRAME_X;y-=BARRACKS_DOORFRAME_Y;
 if((unsigned)x>=BARRACKS_DOORFRAME_W||(unsigned)y>=BARRACKS_DOORFRAME_H)return 0;
 unsigned i=(unsigned)y*BARRACKS_DOORFRAME_W+(unsigned)x;
 return (barracks_doorframe_mask[i>>3]>>(i&7))&1;
}
#endif
'''
# Source stone front pillar sprites42, plus the adjacent source30 stone edges.
from PIL import ImageChops
wall=Image.new('L',(1280,448))
for number in (6,9,10,11,12,13):
 q=R/f'assets/door-frame-audit/{number}.png'
 aa=Image.open(q).convert('RGBA').resize((1280,448),Image.Resampling.LANCZOS).getchannel('A').point(lambda v:255 if v>=96 else 0)
 wall=ImageChops.lighter(wall,aa)
clipped=Image.new('L',wall.size);clipped.paste(wall.crop((1111,0,1180,183)),(1111,0));wall=clipped
wx,wy,wr,wb=wall.getbbox();ww=wr-wx;wh=wb-wy;wbts=bytearray((ww*wh+7)//8)
for i,v in enumerate(wall.crop((wx,wy,wr,wb)).getdata()):
 if v:wbts[i//8]|=1<<(i%8)
s=s.replace('#endif','')
s+=f'#define BARRACKS_DOORWALL_X {wx}\n#define BARRACKS_DOORWALL_Y {wy}\n#define BARRACKS_DOORWALL_W {ww}\n#define BARRACKS_DOORWALL_H {wh}\n'
s+='static const uint8_t barracks_doorwall_mask[]={'+','.join(map(str,wbts))+'};\n'
s+='''static inline int barracks_doorwall_pixel(int x,int y){
 x-=BARRACKS_DOORWALL_X;y-=BARRACKS_DOORWALL_Y;
 if((unsigned)x>=BARRACKS_DOORWALL_W||(unsigned)y>=BARRACKS_DOORWALL_H)return 0;
 unsigned i=(unsigned)y*BARRACKS_DOORWALL_W+(unsigned)x;
 return (barracks_doorwall_mask[i>>3]>>(i&7))&1;
}
#endif
'''
print('Source nearwall',wx,wy,ww,wh,'bytes',len(wbts))
(R/'generated/barracks_doorframe.h').write_text(s)
assert all(bool((bits[i//8]>>(i%8))&1)==bool(v) for i,v in enumerate(a.crop(box).getdata()))
print('Doorframe',box,'bytes',len(bits),'opening center1088,130=',a.getpixel((1088,130)))
