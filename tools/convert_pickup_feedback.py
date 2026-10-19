"""Original onward hand and item pickup feedback from owned Flash assets."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from PIL import Image
import json,sys
ROOT=Path(__file__).resolve().parents[1]
SOURCES=[('onward','main',28,50),('pickup','fx',874,15)]
if '--prepare' in sys.argv:
 for name,src,sid,count in SOURCES:
  r=E.parse(ROOT/f'assets/extracted/{src}.xml').getroot();tags=r.find('tags');defs=[t for t in tags if t.get('type','').startswith('Define')];tags.clear();tags.extend(defs)
  p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
  E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',scaleX='1.0',scaleY='1.0')
  for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
  r.set('frameCount',str(count));rect=r.find('displayRect')
  for k,v in [('Xmin',-3200),('Xmax',3200),('Ymin',-2400),('Ymax',2400)]:rect.set(k,str(v))
  E.ElementTree(r).write(ROOT/f'assets/extracted/{name}.xml',encoding='utf-8',xml_declaration=True)
else:
 samples=[('onward',f) for f in range(2,18)]+[('pickup',f) for f in range(1,5)]
 ims=[Image.open(ROOT/f'assets/{name}/{f}.png').convert('RGBA').resize((160,120),Image.Resampling.LANCZOS) for name,f in samples]
 atlas=Image.new('RGB',(160*8,120*4),(45,45,45))
 for i,im in enumerate(ims):atlas.paste(im,(i%8*160,i//8*120),im)
 q=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);pal=q.getpalette()[:765]
 colors=[0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
 data=[];meta=[];stored={}
 for frame_index,im in enumerate(ims):
  a=im.getchannel('A');box=a.point(lambda v:255 if v>=60 else 0).getbbox() or (80,60,81,61)
  x,y,r,b=box;assert x>0 and y>0 and r<160 and b<120,box
  pix=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
  block=bytes(pix.getpixel((xx,yy))+1 if a.getpixel((xx,yy))>=60 else 0 for yy in range(y,b) for xx in range(x,r))
  width,height=r-x,b-y
  offset_x,offset_y=x-80,y-60
  if frame_index<16:
   # Match feedback_sprite's old shrink=2 exactly: top-left sample every2pixels.
   block=bytes(block[yy*width+xx] for yy in range(0,height,2) for xx in range(0,width,2))
   width=(width+1)//2;height=(height+1)//2
   # C signed division truncates towardzero rather than floor.
   offset_x=int(offset_x/2);offset_y=int(offset_y/2)
  key=(width,height,block)
  if key not in stored:stored[key]=len(data);data.extend(block)
  meta.append((stored[key],offset_x,offset_y,width,height))
 def arr(n,t,values):return f'static const {t} {n}[]={{'+','.join(map(str,values))+'};\n'
 h='#ifndef PICKUP_FEEDBACK_ART_H\n#define PICKUP_FEEDBACK_ART_H\n#include <stdint.h>\ntypedef struct {uint32_t offset;int16_t x,y;uint16_t w,h;} FeedbackFrame;\n'
 h+='#define ONWARD_RENDER_SHRINK 1\n#define ONWARD_FRAME_START 0\n#define ONWARD_FRAME_COUNT 16\n#define PICKUP_FRAME_START 16\n#define PICKUP_FRAME_COUNT 4\n'
 h+=arr('feedback_palette','uint16_t',colors)+arr('feedback_pixels','uint8_t',data)
 h+='static const FeedbackFrame feedback_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n#endif\n'
 (ROOT/'generated/pickup_feedback_art.h').write_text(h);atlas.save(ROOT/'generated/pickup-feedback-preview.png')
 (ROOT/'generated/pickup-feedback-audit.json').write_text(json.dumps(dict(source='MAIN.swf go_arrow sprite28 frames2..17; FX.swf item881 pickup305 ->sprite874 frames1..4 (frame4 calls remove)',coordinate_scale=dict(onward=.25,pickup=.5),frame_origin=[80,60],onward_presampling="Exact former renderer top-left nearest-neighbor stride2; signed offsets truncate towardzero",pixel_bytes=len(data),onward_behavior='Original50-frame pointer sequence repeats hand pulses at7/23/39 with s_Toggle. One16-frame cycle exported.',pickup_behavior='Original item pickup flash removes atframe4;  money also flies to HUD and plays s_Coin1/2. Food plays s_Eat and f_Heal value above head.',samples=samples),indent=2))
 print('Feedback pixelbytes',len(data))
