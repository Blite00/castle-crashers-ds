"""Six original Castle Keep panels, original SKY1 and allocation-free LZ storage."""
from pathlib import Path
from collections import defaultdict, deque
from PIL import Image,ImageDraw
import struct, json, math, sys
R=Path(__file__).resolve().parents[1]
ORIGINS=[(-200,-810),(600,-550),(1400,-460),(2200,-500),(3000,-810),(3800,-810)]
def array(name,typ,data):
 return f'static const {typ} {name}[{len(data)}]={{\n'+''.join(','.join(map(str,data[i:i+32]))+',\n' for i in range(0,len(data),32))+'};\n'
def compress(data):
 table=defaultdict(lambda:deque(maxlen=32));out=bytearray();lit=bytearray();i=0
 def flush():
  if lit:out.append(len(lit)-1);out.extend(lit);lit.clear()
 while i<len(data):
  key=data[i:i+3];best=0;dist=0
  if len(key)==3:
   for p in reversed(table[key]):
    if i-p>65535:break
    n=3
    while n<130 and i+n<len(data) and data[p+n]==data[i+n]:n+=1
    if n>best:best=n;dist=i-p
  if best>=4:
   flush();out.extend((128+best-3,dist&255,dist>>8))
   for j in range(best):table[data[i+j:i+j+3]].append(i+j)
   i+=best
  else:
   table[key].append(i);lit.append(data[i]);i+=1
   if len(lit)==128:flush()
 flush();return out
def decode(data):
 out=bytearray();i=0
 while i<len(data):
  t=data[i];i+=1
  if t<128:out.extend(data[i:i+t+1]);i+=t+1
  else:
   d=data[i]|data[i+1]<<8;i+=2
   for j in range((t&127)+3):out.append(out[-d])
 return out
sky=Image.open(R/'assets/campaign-sky-images/1.png').convert('RGBA').resize((848,424),Image.Resampling.LANCZOS)
header='/* Original LEVEL13/SKY1 art, six panels; quarter-scale storage expanded2x. Palette64 + lossless LZ. */\n#ifndef CAMPAIGN_BACKGROUND_H\n#define CAMPAIGN_BACKGROUND_H\n#include <stdint.h>\n#define CAMPAIGN_SCENE_COUNT 6\n#define CAMPAIGN_SCENE_PIXELS (424*318)\n'
palettes=[];blocks=[];montage=Image.new('RGB',(1272,636));total=0
for i in range(6):
 fg=Image.open(R/f'assets/campaign/{i+1}.png').convert('RGBA')
 # Extend the original bottom edge below the visible floor to avoid blank crop seams.
 px=fg.load()
 for xx in range(fg.width):
  yy=fg.height-1
  while yy>=0 and px[xx,yy][3]<250:yy-=1
  if yy>=fg.height//2:
   edge=px[xx,yy]
   for fill_y in range(yy+1,fg.height):px[xx,fill_y]=edge
 bg=Image.new('RGBA',fg.size,(78,94,101,255));bg.paste(sky,(0,0));bg.alpha_composite(fg)
 q=bg.convert('RGB').resize((212,159),Image.Resampling.LANCZOS).quantize(colors=64,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
 p=q.getpalette();pal=[0]+[0x8000|(p[j]>>3)|((p[j+1]>>3)<<5)|((p[j+2]>>3)<<10) for j in range(0,192,3)]+[0]*191
 pixels=bytes(v+1 for v in q.getdata());packed=compress(pixels);assert decode(packed)==pixels
 header+=array(f'campaign_scene_{i}_lz','uint8_t',packed);palettes.append(pal);blocks.append(len(packed));total+=len(packed)
 q.convert('RGB').save(R/f'generated/campaign-background-{i}.png');montage.paste(q.convert('RGB').resize((424,318),Image.Resampling.NEAREST),(i%3*424,i//3*318))
header+='static const uint16_t campaign_palettes[6][256]={'+','.join('{'+','.join(map(str,p))+'}' for p in palettes)+'};\n'
header+='static const uint8_t *const campaign_scene_lz[6]={'+','.join(f'campaign_scene_{i}_lz' for i in range(6))+'};\n'+array('campaign_scene_lengths','uint32_t',blocks)
header+='''/* Caller supplies a 33708-byte indexed scene buffer. No allocation. */
static inline int campaign_scene_decode_small(unsigned scene,uint8_t *out) {
 if(scene>=CAMPAIGN_SCENE_COUNT||!out)return 0;
 const uint8_t *p=campaign_scene_lz[scene],*end=p+campaign_scene_lengths[scene];
 unsigned n=0;
 while(p<end&&n<(212*159)){
  unsigned t=*p++,count=(t&127)+(t<128?1:3);
  if(count>(212*159)-n)return 0;
  if(t<128){if((unsigned)(end-p)<count)return 0;while(count--)out[n++]=*p++;}
  else{if(end-p<2)return 0;unsigned d=p[0]|((unsigned)p[1]<<8);p+=2;if(!d||d>n)return 0;while(count--){out[n]=out[n-d];n++;}}
 }
 return n==(212*159)&&p==end;
}
/* Compatibility expansion; caller supplies 134832 bytes. */
static inline int campaign_scene_decode(unsigned scene,uint8_t *out) {
 if(!campaign_scene_decode_small(scene,out))return 0;
 for(int y=158;y>=0;y--)for(int x=211;x>=0;x--){uint8_t v=out[y*212+x];unsigned d=y*2*424+x*2;out[d]=out[d+1]=out[d+424]=out[d+425]=v;}
 return 1;
}
#endif
'''
(R/'generated/campaign_background.h').write_text(header);montage.save(R/'generated/campaign-background.png')
# Bound the walkable corridor by original BSP types1/2, excluding slope helperplanes.
if len(sys.argv)>1:
 sys.path.insert(0,str(R/'tools/vendor'));import decrypt_asset as decrypt
 name,encrypted=decrypt.unzip_to_cok6_nrec(Path(sys.argv[1]));r=decrypt.blowfish_decrypt(name,encrypted)
 (R/'assets/extracted/bsp13.bin').write_bytes(r)
else:r=(R/'assets/extracted/bsp13.bin').read_bytes()
n=struct.unpack_from('<I',r,16)[0]//8
edges=[struct.unpack_from('<8f',r,20+i*32)[1:6] for i in range(n)]
profiles=[]
for ox,oy in ORIGINS:
 limits=[]
 for x in range(424):
  wx=ox+x*2;ys=[]
  for x1,y1,x2,y2,kind in edges:
   if kind in (1,2) and min(x1,x2)<=wx<max(x1,x2):ys.append(y1+(y2-y1)*(wx-x1)/(x2-x1))
  if not ys:ys=[-420,-205] if wx<0 else [-435,-267]
  lo=max(8,min(305,math.ceil((min(ys)-oy)/2)+5));hi=max(lo,min(310,math.floor((max(ys)-oy)/2)-5))
  limits.append((lo,hi))
 profiles.append(limits)
g='#ifndef CAMPAIGN_GEOMETRY_H\n#define CAMPAIGN_GEOMETRY_H\n#include <stdint.h>\n/* BSP-derived outer corridor. Slopes follow artwork; interior prop footprints simplified. */\n'
g+='static const int16_t campaign_origins[6][2]={'+','.join('{%d,%d}'%p for p in ORIGINS)+'};\n'
for bound in range(2):
 g+='static const uint16_t campaign_floor_'+('min' if bound==0 else 'max')+'[6][424]={'+','.join('{'+','.join(str(p[bound]) for p in a)+'}' for a in profiles)+'};\n'
g+='#endif\n';(R/'generated/campaign_geometry.h').write_text(g)
(R/'assets/campaign-sections.json').write_text(json.dumps({'origins':ORIGINS,'pixel_scale':0.5,'compressed_bytes':blocks,'palette_colors':64,'geometry':'BSP outer corridor envelope; interior props simplified'},indent=2))
print('Six Castle Keep scenes',blocks,'total',total+3072,'compressed bytes + palettes')
