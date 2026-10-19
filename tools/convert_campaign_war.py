"""Convert original LEVEL11/12 continuous scenery and registered depth layers."""
from pathlib import Path
from PIL import Image
import json,zlib,hashlib,sys
R=Path(__file__).resolve().parents[1]
def pack(b):return zlib.compress(b,9)[2:-4]
def arr(n,t,b):return f'static const {t} {n}[{len(b)}]={{\n'+''.join(','.join(map(str,b[i:i+32]))+',\n' for i in range(0,len(b),32))+'};\n'
def forest_backdrop(W,H):
 """LEVEL18 has no sky image: dusk gradient, original mountain/pine layer faded into the haze, original tree line standing on a dark forest floor."""
 bg=Image.new('RGBA',(W,H));px=bg.load();top=(92,124,146);horizon=(170,198,176)
 for y in range(H):
  t=min(1.0,y/190.0);c=tuple(round(top[i]+(horizon[i]-top[i])*t) for i in range(3))
  for x in range(W):px[x,y]=c+(255,)
 for sid,base,scale,fade in ((19,135,0.5,50),(23,198,0.36,0)):
  layer=Image.open(R/f'build/l18bg/DefineSprite_{sid}/1.png').convert('RGBA');layer=layer.crop(layer.getchannel('A').getbbox())
  layer=layer.resize((round(layer.width*scale),round(layer.height*scale)),Image.Resampling.LANCZOS)
  if fade:
   al=layer.getchannel('A');lp=al.load()
   for y in range(max(0,layer.height-fade),layer.height):
    k=(layer.height-1-y)/fade
    for x in range(layer.width):lp[x,y]=int(lp[x,y]*k)
   layer.putalpha(al)
  x=-40
  while x<W:
   y0=base-layer.height
   if y0>=0:bg.alpha_composite(layer,(x,y0))
   else:bg.alpha_composite(layer.crop((0,-y0,layer.width,layer.height)),(x,0))
   x+=layer.width-30
  if sid==23:
   floor=Image.new('RGBA',(W,H-base));fp=floor.load()
   for yy in range(H-base):
    t=min(1.0,yy/40.0);c=(round(36+30*t),round(66+4*t),round(48-2*t),255)
    for xx in range(W):fp[xx,yy]=c
   bg.alpha_composite(floor,(0,base))
 return bg
def flood_backdrop(W,H):
 """LEVEL23: dark woods above the river, deep water below the skewed river plane."""
 bg=Image.new('RGBA',(W,H));px=bg.load()
 for y in range(H):
  t=y/H;c=(round(10+14*t),round(42+18*t),round(44+26*t),255)
  for x in range(W):px[x,y]=c
 return bg
def deer_backdrop(W,H):
 """LEVEL22: dark woods behind the mill's open wall slats (original bgwoods colour)."""
 bg=Image.new('RGBA',(W,H));px=bg.load()
 for y in range(H):
  t=y/H;c=(round(8+6*t),round(38+8*t),round(40+4*t),255)
  for x in range(W):px[x,y]=c
 return bg
def thieves_shear(im,dy,H):
 """Shift every column up by dy so the sloped corridor sits in the 448-row world (see thieves_shear.py)."""
 import numpy as np
 a=np.array(im.convert('RGBA'));raw_h=a.shape[0];out=np.zeros((H,a.shape[1],4),np.uint8)
 for x in range(a.shape[1]):
  d=dy[min(x,len(dy)-1)];lo=max(0,d);hi=min(raw_h,H+d)
  if hi>lo:out[lo-d:hi-d,x]=a[lo:hi,x]
 return Image.fromarray(out,'RGBA').copy()
def thieves_backdrop(W,H):
 """LEVEL4 sits in a dark canopy: the original deep background colour with a faint vertical gradient."""
 bg=Image.new('RGBA',(W,H));px=bg.load()
 for y in range(H):
  t=y/H;c=(round(16+10*t),round(40+6*t),round(38+2*t),255)
  for x in range(W):px[x,y]=c
 return bg
def convert(name):
 info=json.loads((R/f'assets/campaign-{name}-layers.json').read_text());W,H=info['size'];prefix='campaign_'+name
 shear=None
 if name=='thieves':shear=json.loads((R/'assets/thieves-shear.json').read_text())['dy'];HR=H;H=448
 im=Image.open(R/f'assets/campaign-{name}/1.png').convert('RGBA')
 if shear:im=thieves_shear(im.resize((W,HR),Image.Resampling.LANCZOS),shear,H)
 else:im=im.resize((W,H),Image.Resampling.LANCZOS)
 p=im.load()
 # Preserve original edge colors below the authored floor, matching existing route overscan.
 for x in range(W):
  y=H-1
  while y>=0 and p[x,y][3]<250:y-=1
  if y>=0:
   for yy in range(y+1,H):p[x,yy]=p[x,y]
 bg=flood_backdrop(W,H) if name=='flood' else deer_backdrop(W,H) if name=='deer' else thieves_backdrop(W,H) if name=='thieves' else forest_backdrop(W,H) if name=='forest' else Image.open(R/'assets/campaign-sky-images/1.png').convert('RGBA').resize((W,W//2),Image.Resampling.LANCZOS).crop((0,0,W,H));bg.alpha_composite(im)
 layers=[]
 for i,m in enumerate(info['layers']):
  im=Image.open(R/f'assets/campaign-{name}-fg/{i+1}.png').convert('RGBA')
  im=thieves_shear(im.resize((W,HR),Image.Resampling.LANCZOS),shear,H) if shear else im.resize((W,H),Image.Resampling.LANCZOS)
  if shear:m=dict(m);m['depth']=m['depth']-shear[min(max(int(m['x']),0),W-1)]
  bbox=im.getchannel('A').point(lambda a:255 if a>=96 else 0).getbbox()
  if bbox:layers.append((m,bbox,im.crop(bbox),i))
 # Include original prop colors in the global scene palette without allocating whole-frame copies.
 extra=sum(im.height for _,_,im,_ in layers);training=Image.new('RGB',(W,H+extra));training.paste(bg.convert('RGB'),(0,0));yy=H
 for _,_,im,_ in layers:training.paste(im.convert('RGB'),(0,yy));yy+=im.height
 q=training.quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);pal888=q.getpalette();colors=[0x8000|(pal888[i]>>3)|((pal888[i+1]>>3)<<5)|((pal888[i+2]>>3)<<10) for i in range(0,384,3)];unique=list(dict.fromkeys(colors));remap=[unique.index(v)+1 for v in colors];pal=[0]+unique+[0]*(255-len(unique));pixels=bytearray(remap[v] for v in bg.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes());records=[];fg=bytearray();audit=[];hashes=[]
 layers.sort(key=lambda t:t[0]['depth'])
 for m,(x,y,x2,y2),im,placement in layers:
  w,h=x2-x,y2-y;alpha=im.getchannel('A').tobytes();indexed=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();data=bytes(remap[v] if a>=96 else 0 for v,a in zip(indexed,alpha))
  if not m['dynamic']:
   for yy in range(h):
    for xx in range(w):
     v=data[yy*w+xx]
     if v:pixels[(yy+y)*W+xx+x]=v
  step=w if w*h<=14336 else 32
  for xx in range(0,w,step):
   ww=min(step,w-xx);part=b''.join(data[yy*w+xx:yy*w+xx+ww] for yy in range(h));encoded=pack(part);assert zlib.decompress(encoded,-15)==part
   records.append((x+xx,y,ww,h,m['depth'],len(fg),len(encoded),m['source'],placement,int(m['dynamic'])));fg.extend(encoded)
   hv=2166136261
   for v in part:hv=((hv^v)*16777619)&0xffffffff
   hashes.append(hv)
  audit.append(dict(source=m['source'],placement=placement,name=m['name'],x=x,y=y,w=w,h=h,depth=m['depth'],dynamic=m['dynamic']))
 if not records: # A level without foreground layers keeps one transparent 1x1 record so the typed tables are never empty.
  encoded=pack(bytes(1));records.append((0,0,1,1,0,len(fg),len(encoded),0,0,0));fg.extend(encoded);hashes.append(0)
 rowdata=bytearray();offsets=[0]
 for strip in range(W//32):
  x=strip*32
  for y in range(0,H,64):
   block=b''.join(pixels[yy*W+x:yy*W+x+32] for yy in range(y,min(y+64,H)));encoded=pack(block);assert zlib.decompress(encoded,-15)==block;rowdata.extend(encoded);offsets.append(len(rowdata))
 guard=name.upper()+'_ART_H';h=f'#ifndef {guard}\n#define {guard}\n#include <stdint.h>\n#include <string.h>\n#include "asset_inflate.h"\n#include "route_art_types.h"\n#define {prefix.upper()}_WIDTH {W}\n#define {prefix.upper()}_HEIGHT {H}\n#define {prefix.upper()}_FG_COUNT {len(records)}\n'
 if name in ('forest','thieves','deer','flood'):
  # Chapter-bank tables: byte images with a typed view, so the shared ARM9 image stays small.
  import struct
  def bank_table(n,ctype,fmt,rows):
   data=b''.join(struct.pack(fmt,*r) if isinstance(r,(tuple,list)) else struct.pack(fmt,r) for r in rows)
   return f'static const uint8_t {n}_data[{len(data)}] __attribute__((aligned(4)))={{\n'+''.join(','.join(map(str,data[i:i+32]))+',\n' for i in range(0,len(data),32))+f'}};\n#define {n} ((const {ctype}*)({n}_data))\n'
  h+=bank_table(prefix+'_palette','uint16_t','<H',pal)+bank_table(prefix+'_row_offsets','uint32_t','<I',offsets)+arr(prefix+'_row_data','uint8_t',rowdata)+arr(prefix+'_fg_data','uint8_t',fg)
  h+='_Static_assert(sizeof(CampaignRouteLayer)==28,"Bank layer image layout");\n'+bank_table(prefix+'_fg_layers','CampaignRouteLayer','<hhhhh2xIIHHB3x',records)
 else:
  h+=arr(prefix+'_palette','uint16_t',pal)+arr(prefix+'_row_offsets','uint32_t',offsets)+arr(prefix+'_row_data','uint8_t',rowdata)+arr(prefix+'_fg_data','uint8_t',fg)
  h+=f'static const CampaignRouteLayer {prefix}_fg_layers[]={{'+','.join('{'+','.join(map(str,r))+'}' for r in records)+'};\n'
 h+=f'''static inline int {prefix}_decode_rows(unsigned strip,unsigned first,unsigned count,uint8_t*out){{
 if(strip>={W//32}||first>{H}||count>{H}-first||!out)return 0;
 uint8_t chunk[2048];unsigned done=0;
 while(done<count){{unsigned y=first+done,index=strip*7+y/64,begin=y%64,n=64-begin;if(n>count-done)n=count-done;
  unsigned off={prefix}_row_offsets[index],end={prefix}_row_offsets[index+1];
  if(!asset_inflate({prefix}_row_data+off,end-off,chunk,2048))return 0;
  memcpy(out+done*32,chunk+begin*32,n*32);done+=n;
 }}return 1;
}}
static inline int {prefix}_decode_strip(unsigned strip,uint8_t*out){{return {prefix}_decode_rows(strip,0,{H},out);}}
static inline int {prefix}_decode_fg(unsigned index,uint8_t*out){{
 if(index>={len(records)}||!out)return 0;
 const CampaignRouteLayer*l=&{prefix}_fg_layers[index];
 if(l->w*l->h>14336)return 0;
 return asset_inflate({prefix}_fg_data+l->offset,l->length,out,l->w*l->h);
}}
#endif
'''
 (R/f'generated/{name}_art.h').write_text(h);(R/f'assets/campaign-{name}-reference.bin').write_bytes(pixels)
 (R/f'generated/{name}-fg-hashes.bin').write_bytes(__import__('struct').pack('<'+str(len(hashes))+'I',*hashes))
 preview=Image.frombytes('P',(W,H),bytes(pixels));rgb=[]
 for c in pal:rgb.extend(((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31))
 preview.putpalette(rgb);preview.convert('RGB').save(R/f'generated/campaign-{name}-preview.png')
 report=dict(level=info['level'],origin=info['origin'],size=[W,H],palette_colors=len(unique),terrain_bytes=len(rowdata),foreground_bytes=len(fg),layers=audit,layer_records=len(records),max_pixels=max(r[2]*r[3] for r in records),pixel_sha256=hashlib.sha256(pixels).hexdigest())
 (R/f'generated/{name}-art-audit.json').write_text(json.dumps(report,indent=2));print(name,'terrain',len(rowdata),'fg',len(fg),'total',len(rowdata)+len(fg),'layers',len(records),'max',report['max_pixels'])
if __name__=='__main__':
 (R/'generated/route_art_types.h').write_text('#ifndef ROUTE_ART_TYPES_H\n#define ROUTE_ART_TYPES_H\n#include <stdint.h>\ntypedef struct {int16_t x,y,w,h,depth;uint32_t offset,length;uint16_t source,placement;uint8_t dynamic;} CampaignRouteLayer;\n#endif\n')
 for name in (('blacksmith','frog','ark') if '--hubs' in sys.argv else ('forest',) if '--forest' in sys.argv else ('thieves',) if '--thieves' in sys.argv else ('deer',) if '--deer' in sys.argv else ('flood',) if '--flood' in sys.argv else ('war','bridge')):convert(name)

if __name__ == "__main__" and "--prepare" not in sys.argv:
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
