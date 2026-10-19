"""Replace incorrect FXfood10 with original PLAYER item305 frame9 health potion."""
from pathlib import Path
import re
from PIL import Image
R=Path(__file__).resolve().parents[1];p=R/'generated/loot_art.c';s=p.read_text()
def vals(name):return list(map(int,re.findall(r'-?\d+',re.search(name+r'\[\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
pal=vals('loot_palette');data=vals('loot_pixels');meta=[tuple(map(int,x.split(','))) for x in re.findall(r'\{([^}]+)\}',re.search(r'loot_frames\[\]\s*=\s*\{(.*?)\};',s,re.S)[1])]
base=data[:meta[5][0]];used=set(base);free=[i for i in range(1,64) if i not in used];assert len(free)>=4
im=Image.open(R/'assets/potion/1.png').convert('RGBA').resize((48,48),Image.Resampling.LANCZOS);mask=im.getchannel('A').point(lambda a:255 if a>=100 else 0);x,y,r,b=mask.getbbox();im=im.crop((x,y,r,b));q=im.convert('RGB').quantize(colors=len(free),method=Image.Quantize.MEDIANCUT);rgb=q.getpalette()
for i,index in enumerate(free):pal[index]=32768|(rgb[i*3]>>3)|((rgb[i*3+1]>>3)<<5)|((rgb[i*3+2]>>3)<<10)
new=[free[c] if a>=100 else 0 for c,a in zip(q.tobytes(),im.getchannel('A').tobytes())];meta[5]=(len(base),x-24,y-24,r-x,b-y);data=base+new
s='#include "loot_art.h"\nconst uint16_t loot_palette[]={'+','.join(map(str,pal))+'};\nconst uint8_t loot_pixels[]={'+','.join(map(str,data))+'};\nconst ArtFrame loot_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n';p.write_text(s)
im.resize((im.width*5,im.height*5),Image.Resampling.NEAREST).save(R/'generated/potion-source-preview.png')
print('Correct original potion',meta[5],'palette slots',len(free),'total pixels',len(data))

preview=Image.new('RGB',(48*6,48))
for i,(offset,xx,yy,w,h) in enumerate(meta):
 for row in range(h):
  for col in range(w):
   index=data[offset+row*w+col]
   if index:
    c=pal[index];preview.putpixel((i*48+24+xx+col,24+yy+row),((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31))
preview.save(R/'generated/loot-preview.png')
