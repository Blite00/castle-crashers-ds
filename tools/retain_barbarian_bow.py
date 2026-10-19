"""Retain two genuine source Barbarian bow poses, leaving other pixels unchanged."""
from pathlib import Path
import re,json,sys
from PIL import Image
from convert_barbarian import pack,unpack
R=Path(__file__).resolve().parents[1]
def optimize(write=True):
 p=R/'generated/barbarian_art.h';s=p.read_text()
 if 'BARBARIAN_BOW_SOURCE_FRAMES' in s:return
 m=re.search(r'static const uint16_t barbarian_palette\[[^]]*\] = \{(.*?)\};',s,re.S);pal=list(map(int,re.findall(r'\d+',m[1])))
 q=Image.new('P',(1,1));rgb=[]
 for c in pal[1:]:rgb.extend(((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31))
 q.putpalette(rgb+rgb[:3])
 m=re.search(r'static const uint8_t barbarian_pixels\[[^]]*\] = \{(.*?)\};',s,re.S);data=bytearray(map(int,re.findall(r'\d+',m[1])));old=len(data)
 mm=re.search(r'static const ArtFrame barbarian_frames\[\] = \{(.*?)\};',s,re.S);meta=[tuple(map(int,v)) for v in re.findall(r'\{(\d+),(-?\d+),(-?\d+),(\d+),(\d+)\}',mm[1])]
 orig=list(meta);new=[]
 for frame in (4,10):
  im=Image.open(R/f'assets/barbarian/{246+frame}.png').convert('RGBA').resize((192,120),Image.Resampling.LANCZOS);a=im.getchannel('A').point(lambda a:255 if a>=100 else 0);x,y,r,b=a.getbbox();col=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);raw=bytes(col.getpixel((xx,yy))+1 if a.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r));blob=pack(raw);assert unpack(blob,0,len(raw))==raw
  new.append((len(data),x-96,y-88,r-x,b-y));data.extend(blob)
 for f in range(20):meta[245+f]=new[0 if f<10 else 1]
 print('Original bow2 poses added',len(data)-old,'max',max(v[3]*v[4] for v in meta))
 if not write:return
 s=s[:mm.start()]+'static const ArtFrame barbarian_frames[]={\n'+''.join('{'+','.join(map(str,v))+'},\n' for v in meta)+'};'+s[mm.end():]
 m=re.search(r'static const uint8_t barbarian_pixels\[[^]]*\] = \{(.*?)\};',s,re.S)
 s=s[:m.start()]+'static const uint8_t barbarian_pixels['+str(len(data))+']={\n'+','.join(map(str,data))+'\n};'+s[m.end():]
 s=s.replace('#endif','#define BARBARIAN_BOW_SOURCE_FRAMES 2\n#endif');p.write_text(s)
 audit=json.loads((R/'generated/barbarian-audit.json').read_text());audit['fallback_poses'].pop('bow',None);audit['pixel_bytes']=len(data);audit['bow_source_samples']=[4,10]
 for f in range(20):audit['source_frame_map'][245+f]=245+(4 if f<10 else 10)
 (R/'generated/barbarian-audit.json').write_text(json.dumps(audit,indent=2))
 assert all(meta[i]==orig[i] for i in range(len(meta)) if not 245<=i<265)
if __name__=='__main__':optimize('--measure' not in sys.argv)
