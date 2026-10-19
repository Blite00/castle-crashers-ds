"""Original PLAYER bow item icon and itemselect overhead pulse."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import sys,json
from PIL import Image
from prepare_frames import freeze
ROOT=Path(__file__).resolve().parents[1]
if '--prepare' in sys.argv:
 r=E.parse(ROOT/'assets/extracted/player.xml').getroot();defs=[x for x in r.find('tags') if x.get('type','').startswith('Define')]
 for x in defs:
  if x.get('spriteId')=='305':freeze(x,1)
  if x.get('spriteId')=='289':freeze(x,3)
  if x.get('spriteId')=='316':
   tags=x.find('subTags')
   for t in list(tags):
    if t.get('characterId') in ['315','311']:tags.remove(t)
 tags=r.find('tags');tags.clear();tags.extend(defs)
 for sid,count in [(305,1),(316,9)]:
  E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
  t=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
  E.SubElement(t,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',scaleX='1.0',scaleY='1.0')
  for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
 r.set('frameCount','10');rect=r.find('displayRect')
 for k,v in [('Xmin',-1600),('Xmax',1600),('Ymin',-1600),('Ymax',1600)]:rect.set(k,str(v))
 E.ElementTree(r).write(ROOT/'assets/extracted/items.xml',encoding='utf-8',xml_declaration=True)
else:
 ims=[Image.open(ROOT/f'assets/items/{i}.png').convert('RGBA').resize((80,80),Image.Resampling.LANCZOS) for i in [1,*range(3,11)]]
 # The HUD uses a legible 32x32 item; overhead keeps source half scale.
 icon=ims[0].crop(ims[0].getbbox());icon.thumbnail((32,32),Image.Resampling.LANCZOS)
 ims[0]=Image.new('RGBA',(32,32));ims[0].paste(icon,((32-icon.width)//2,(32-icon.height)//2))
 atlas=Image.new('RGB',(80,80*9))
 for i,im in enumerate(ims):atlas.paste(im,(0,80*i),im)
 q=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT);pal=q.getpalette()[:765]
 colors=[0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
 data=[];meta=[]
 for i,im in enumerate(ims):
  im=im.resize((32,32) if i==0 else (40,40),Image.Resampling.LANCZOS)
  alpha=im.getchannel('A').point(lambda a:255 if a>=60 else 0);box=alpha.getbbox();assert box
  x,y,r,b=box;pic=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
  meta.append((len(data),x-im.width//2,y-im.height//2,r-x,b-y))
  data.extend(pic.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
 def arr(n,t,v):return f'static const {t} {n}[]={{'+','.join(map(str,v))+'};\n'
 h='#ifndef ITEM_ART_H\n#define ITEM_ART_H\n#include <stdint.h>\ntypedef struct {uint32_t offset;int16_t x,y,w,h;} ItemFrame;\n'
 h+=arr('item_palette','uint16_t',colors)+arr('item_pixels','uint8_t',data)
 h+='static const ItemFrame item_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n#define ITEM_POPUP_FRAMES 8\n#endif\n'
 (ROOT/'generated/item_art.h').write_text(h)
 (ROOT/'generated/item-audit.json').write_text(json.dumps(dict(source='PLAYER.swf',icon='sprite305 frame1 bow',popup='overlay317 itemselect ->316 with pulse269 frames1..8; console prompts removed',root_offset=[0.5,-62.6],frames=9,pixel_bytes=len(data)),indent=2))
 atlas.save(ROOT/'generated/item-preview.png')
 print('Item pixels',len(data),meta)

if '--prepare' not in sys.argv:
 import runpy
 runpy.run_path(str(ROOT/'tools/share_item_art.py'))
