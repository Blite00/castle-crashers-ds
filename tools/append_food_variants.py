"""Original five barrel foods (FX834 frames2..6) and shop potion (LEVEL19 sprite102)."""
from pathlib import Path
from PIL import Image
import re
R=Path(__file__).resolve().parents[1];p=R/'generated/loot_art.c';s=p.read_text()
def arr(name):return list(map(int,re.findall(r'-?\d+',re.search(name+r'\[\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
oldpal=arr('loot_palette')[:64];old=arr('loot_pixels');oldmeta=[tuple(map(int,v.split(','))) for v in re.findall(r'\{([^}]+)\}',re.search(r'loot_frames\[\]\s*=\s*\{(.*?)\};',s,re.S)[1])]
ims=[Image.open(R/f'assets/food-variants/{i}.png').convert('RGBA').resize((48,80),Image.Resampling.LANCZOS) for i in range(1,6)]
potion=Image.open(R/'assets/shop-potion/1.png').convert('RGBA');potion=potion.crop(potion.getbbox());potion.thumbnail((24,30),Image.Resampling.LANCZOS);canvas=Image.new('RGBA',(48,80));canvas.alpha_composite(potion,((48-potion.width)//2,48-potion.height//2));ims.append(canvas)
atlas=Image.new('RGB',(48*6,80))
for i,im in enumerate(ims):atlas.paste(im,(i*48,0),im)
q=atlas.quantize(colors=63,method=Image.Quantize.MEDIANCUT);rgb=q.getpalette();pal=oldpal+[32768|(rgb[i]>>3)|((rgb[i+1]>>3)<<5)|((rgb[i+2]>>3)<<10) for i in range(0,189,3)]+[0]
data=[];meta=[]
for frame in range(10):
 index={2:0,6:1,7:2,8:3,9:4,5:5}.get(frame)
 if index is None:
  off,x,y,w,h=oldmeta[frame];raw=old[off:off+w*h]
 else:
  im=ims[index];mask=im.getchannel('A').point(lambda a:255 if a>=100 else 0);x,y,r,b=mask.getbbox();pic=im.crop((x,y,r,b));color=pic.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);raw=[v+64 if a>=100 else 0 for v,a in zip(color.tobytes(),pic.getchannel('A').tobytes())];w=r-x;h=b-y;x-=24;y-=48
 meta.append((len(data),x,y,w,h));data.extend(raw)
s='#include "loot_art.h"\nconst uint16_t loot_palette[]={'+','.join(map(str,pal))+'};\nconst uint8_t loot_pixels[]={'+','.join(map(str,data))+'};\nconst ArtFrame loot_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n';p.write_text(s)
h=(R/'generated/loot_art.h').read_text();h=h.replace('LOOT_FRAME_COUNT 6','LOOT_FRAME_COUNT 10').replace('loot_palette[64]','loot_palette[128]').replace('loot_frames[6]','loot_frames[10]')
if 'loot_food_frame' not in h:h=h.replace('#endif','static inline int loot_food_frame(unsigned value){unsigned variant=value>>5;return variant==0||variant>4?2:5+(int)variant;}\n#endif')
(R/'generated/loot_art.h').write_text(h)
atlas.save(R/'generated/source-food-potion-preview.png')
print('Loot with five sourcefoods+shopbottle',len(data),'pixels; totalgrowth',len(data)-len(old)+128+48)

hashes=[]
for off,x,y,w,h in meta:
 hash=2166136261
 for value in data[off:off+w*h]:hash=((hash^value)*16777619)&0xffffffff
 hashes.append(hash)
(R/'generated/loot_hashes.h').write_text('static const uint32_t loot_hashes[]={'+','.join(str(h)+'u' for h in hashes)+'};\n')
