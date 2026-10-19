"""Original MAIN.swf boss health meter (sprite 312 frame 25): ornate frame, face medallion and red bar."""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
SCALE=0.6
im=Image.open(R/'assets/boss-meter/DefineSprite_312/25.png').convert('RGBA')
im=im.crop(im.getchannel('A').getbbox())
w,h=round(im.width*SCALE),round(im.height*SCALE)
im=im.resize((w,h),Image.Resampling.LANCZOS)
alpha=im.getchannel('A').point(lambda a:255 if a>=100 else 0)
rgb=Image.new('RGB',im.size,(0,0,0));rgb.paste(im.convert('RGB'),mask=alpha)
q=rgb.quantize(colors=15,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
pal=q.getpalette()[:45];px=list(q.getdata());a=list(alpha.getdata())
idx=[(p+1) if aa else 0 for p,aa in zip(px,a)]
colors=[0]+[0x8000|(pal[i*3]>>3)|((pal[i*3+1]>>3)<<5)|((pal[i*3+2]>>3)<<10) for i in range(15)]
# Fill shades: strongly red palette entries are the part of the bar that depletes.
fill=[i+1 for i in range(15) if pal[i*3]>120 and pal[i*3]>pal[i*3+1]*1.7 and pal[i*3]>pal[i*3+2]*1.7]
data=bytearray();i=0
while i<len(idx):
    n=1
    while i+n<len(idx) and idx[i+n]==idx[i] and n<16:n+=1
    data.append(idx[i]|((n-1)<<4));i+=n
fillmask=sum(1<<f for f in fill)
# Left edge (in art pixels) of the bar interior so partial fill can be mapped onto the red area.
# The depleting bar is the longest contiguous red run on the middle row (the medallion is red too).
row=h//2;best=(0,0,0);x=0
while x<w:
    if idx[row*w+x] in fill:
        e=x
        while e<w and idx[row*w+e] in fill:e+=1
        if e-x>best[0]:best=(e-x,x,e-1)
        x=e
    else:x+=1
lo,hi=best[1],best[2]
(R/'generated/boss_meter_art.h').write_text('#ifndef BOSS_METER_ART_H\n#define BOSS_METER_ART_H\n#include <stdint.h>\n'
 f'#define BOSS_METER_W {w}\n#define BOSS_METER_H {h}\n#define BOSS_METER_FILL_LO {lo}\n#define BOSS_METER_FILL_HI {hi}\n#define BOSS_METER_FILL_MASK 0x{fillmask:x}u\n'
 'static const uint16_t boss_meter_palette[16]={'+','.join(map(str,colors))+'};\n'
 f'static const uint8_t boss_meter_runs[{len(data)}]={{'+','.join(map(str,data))+'};\n#endif\n')
prev=Image.new('RGBA',(w*3,h*3),(236,214,160,255))
big=Image.new('RGBA',(w,h),(0,0,0,0))
big.putdata([(0,0,0,0) if v==0 else ((colors[v]&31)*255//31,((colors[v]>>5)&31)*255//31,((colors[v]>>10)&31)*255//31,255) for v in idx])
prev.alpha_composite(big.resize((w*3,h*3),Image.NEAREST));prev.save(R/'generated/boss-meter-preview.png')
print('boss meter',w,h,'runs',len(data),'fill colors',fill,'fill x',lo,hi)
