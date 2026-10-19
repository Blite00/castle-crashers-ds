"""Compact original horse/rider/sleep keys, fast per-frame RLE, bank resident."""
from pathlib import Path
from PIL import Image
import json,hashlib
from convert_barbarian import pack,unpack,array
R=Path(__file__).resolve().parents[1];spec=[1,4,7,10,13,16,19,22,25];ims=[]
for f in spec:
 im=Image.open(R/f'assets/hub-ambient/{f}.png').convert('RGBA');ims.append(im.resize((im.width//2,im.height//2),Image.Resampling.LANCZOS))
atlas=Image.new('RGB',(192,sum(im.height for im in ims)));y=0
for im in ims:atlas.paste(im,(0,y),im.getchannel('A'));y+=im.height
q=atlas.quantize(colors=127,method=Image.Quantize.MEDIANCUT);rgb=q.getpalette();pal=[0]+[0x8000|(rgb[i]>>3)|((rgb[i+1]>>3)<<5)|((rgb[i+2]>>3)<<10) for i in range(0,381,3)]
blob=bytearray();meta=[];seen={};hashes=[];preview=Image.new('RGBA',(192*3,128*3),(80,80,80,255))
for i,im in enumerate(ims):
 a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);box=a.getbbox();assert box
 x,y,x2,y2=box;assert x>0 and y>0 and x2<im.width and y2<im.height,('clipped',i,box,im.size)
 w,h=x2-x,y2-y;v=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).crop(box).tobytes();alpha=a.crop(box).tobytes();raw=bytes(p+1 if aa else 0 for p,aa in zip(v,alpha));key=(w,h,raw)
 if key not in seen:seen[key]=len(blob);blob.extend(pack(raw))
 off=seen[key];assert unpack(blob,off,len(raw))==raw;meta.append((off,x-96,y-96,w,h));hashes.append(hashlib.sha256(raw).hexdigest());preview.alpha_composite(im,(i%3*192,i//3*128))
h='#ifndef HUB_AMBIENT_ART_H\n#define HUB_AMBIENT_ART_H\n#include "castle_art.h"\n#define HUB_AMBIENT_FRAME_COUNT 9u\n#define HUB_AMBIENT_PIXELS_MAX '+str(max(f[3]*f[4] for f in meta))+'u\n'
h+='/*Source smith73 loop72: frame=(tick%27)/3. World foot381,300.*/\n'+array('hub_ambient_palette','uint16_t',pal)+array('hub_ambient_pixels','uint8_t',blob)
h+='static const ArtFrame hub_ambient_frames[]={'+','.join('{'+','.join(map(str,f))+'}' for f in meta)+'};\n'
h+='''static inline int hub_ambient_decode(unsigned id,uint8_t*out){
 if(id>=HUB_AMBIENT_FRAME_COUNT||!out)return 0;
 const ArtFrame*f=&hub_ambient_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){if(at>=sizeof(hub_ambient_pixels))return 0;unsigned t=hub_ambient_pixels[at++],n=(t&127)+1;if(n>left)return 0;
  if(t&128){if(at>=sizeof(hub_ambient_pixels))return 0;unsigned v=hub_ambient_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else {if(n>sizeof(hub_ambient_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=hub_ambient_pixels[at++];}left-=n;
 }return 1;
}
#endif
'''
(R/'generated/hub_ambient_art.h').write_text(h);preview.save(R/'generated/hub-ambient-preview.png');(R/'generated/hub-ambient-audit.json').write_text(json.dumps(dict(spec=spec,bytes=len(blob),frames=meta,hashes=hashes),indent=2));print('Hub ambient smith',len(blob),'bytes maxpixels',max(f[3]*f[4] for f in meta))

