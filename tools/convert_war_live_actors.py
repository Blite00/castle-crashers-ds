"""Compact original horse/rider/sleep keys, fast per-frame RLE, bank resident."""
from pathlib import Path
from PIL import Image
import json,hashlib
from convert_barbarian import pack,unpack,array
R=Path(__file__).resolve().parents[1];spec=json.loads((R/'assets/war-live-spec.json').read_text());ims=[]
for s in spec:
 for f in s['samples']:
  im=Image.open(R/f"assets/war-live-{s['name']}/{f}.png").convert('RGBA');ims.append(im.resize((im.width//2,im.height//2),Image.Resampling.LANCZOS))
atlas=Image.new('RGB',(192,sum(im.height for im in ims)));y=0
for im in ims:atlas.paste(im,(0,y),im.getchannel('A'));y+=im.height
q=atlas.quantize(colors=127,method=Image.Quantize.MEDIANCUT);rgb=q.getpalette();pal=[0]+[0x8000|(rgb[i]>>3)|((rgb[i+1]>>3)<<5)|((rgb[i+2]>>3)<<10) for i in range(0,381,3)]
blob=bytearray();meta=[];seen={};hashes=[];preview=Image.new('RGBA',(192*4,168*6),(80,80,80,255))
for i,im in enumerate(ims):
 a=im.getchannel('A').point(lambda v:255 if v>=100 else 0);box=a.getbbox();assert box
 x,y,x2,y2=box;assert x>0 and y>0 and x2<im.width and y2<im.height,('clipped',i,box,im.size)
 w,h=x2-x,y2-y;v=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).crop(box).tobytes();alpha=a.crop(box).tobytes();raw=bytes(p+1 if aa else 0 for p,aa in zip(v,alpha));key=(w,h,raw)
 if key not in seen:seen[key]=len(blob);blob.extend(pack(raw))
 off=seen[key];assert unpack(blob,off,len(raw))==raw;meta.append((off,x-96,y-88,w,h));hashes.append(hashlib.sha256(raw).hexdigest());preview.alpha_composite(im,(i%4*192,i//4*168))
h='#ifndef WAR_LIVE_ART_H\n#define WAR_LIVE_ART_H\n#include "castle_art.h"\n#define WAR_LIVE_FRAME_COUNT 24u\n#define WAR_LIVE_PIXELS_MAX '+str(max(f[3]*f[4] for f in meta))+'u\n'
h+='/*0..3horse,4..7blue,8..11orange,12..15red,16..19green,20..23sleep.*/\n'+array('war_live_palette','uint16_t',pal)+array('war_live_pixels','uint8_t',blob)
h+='static const ArtFrame war_live_frames[]={'+','.join('{'+','.join(map(str,f))+'}' for f in meta)+'};\n'
h+='''static inline int war_live_decode(unsigned id,uint8_t*out){
 if(id>=WAR_LIVE_FRAME_COUNT||!out)return 0;
 const ArtFrame*f=&war_live_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){if(at>=sizeof(war_live_pixels))return 0;unsigned t=war_live_pixels[at++],n=(t&127)+1;if(n>left)return 0;
  if(t&128){if(at>=sizeof(war_live_pixels))return 0;unsigned v=war_live_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else {if(n>sizeof(war_live_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=war_live_pixels[at++];}left-=n;
 }return 1;
}
#endif
'''
(R/'generated/war_live_art.h').write_text(h);preview.save(R/'generated/war-live-preview.png');(R/'generated/war-live-audit.json').write_text(json.dumps(dict(spec=spec,bytes=len(blob),frames=meta,hashes=hashes),indent=2));print('War live actors',len(blob),'bytes maxpixels',max(f[3]*f[4] for f in meta))

if __name__ == "__main__" and "--prepare" not in __import__("sys").argv:
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
