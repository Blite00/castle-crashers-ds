"""Import actual FX item selector834: coin13, bag8, pear2, chicken7, bluegem9, potion10."""
from pathlib import Path
import sys,xml.etree.ElementTree as E
from copy import deepcopy
from PIL import Image
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
if '--prepare' in sys.argv:
 r=E.parse(R/'assets/extracted/fx.xml').getroot();tags=r.find('tags')
 definitions=[t for t in tags if t.get('type','').startswith('Define')]
 original=next(t for t in definitions if t.get('spriteId')=='834')
 tags.clear();tags.extend(definitions)
 for i,f in enumerate([13,8,2,7,9,10]):
  s=deepcopy(original);s.set('spriteId',str(2000+i));freeze(s,f);tags.append(s)
  E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1')
  t=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(2000+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
  E.SubElement(t,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
  E.SubElement(tags,'item',type='ShowFrameTag')
 r.set('frameCount','6');rect=r.find('displayRect')
 for k,v in dict(Xmin=-960,Ymin=-960,Xmax=960,Ymax=960).items():rect.set(k,str(v))
 E.ElementTree(r).write(R/'assets/extracted/loot.xml',encoding='utf-8',xml_declaration=True)
else:
 frames=[Image.open(R/f'assets/loot/{i}.png').convert('RGBA').resize((48,48),Image.Resampling.LANCZOS) for i in range(1,7)]
 sheet=Image.new('RGB',(48*6,48))
 for i,im in enumerate(frames):sheet.paste(im,(i*48,0),im.getchannel('A'))
 q=sheet.quantize(colors=63,method=Image.Quantize.MEDIANCUT);pal=q.getpalette()[:189]
 colors=[0]+[32768|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,189,3)]
 data=[];meta=[]
 for im in frames:
  mask=im.getchannel('A').point(lambda a:255 if a>=100 else 0);x,y,r,b=mask.getbbox();indexed=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
  meta.append((len(data),x-24,y-24,r-x,b-y))
  data.extend(indexed.getpixel((xx,yy))+1 if mask.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
 h='#ifndef LOOT_ART_H\n#define LOOT_ART_H\n#include "castle_art.h"\n'
 h+='#define LOOT_FRAME_COUNT 6\n#define LOOT_FRAME_GEM 4\n#define LOOT_FRAME_POTION 5\n'
 h+='extern const uint16_t loot_palette[64];\nextern const uint8_t loot_pixels[];\nextern const ArtFrame loot_frames[6];\n#endif\n'
 c='#include "loot_art.h"\nconst uint16_t loot_palette[]={'+','.join(map(str,colors))+'};\n'
 c+='const uint8_t loot_pixels[]={'+','.join(map(str,data))+'};\n'
 c+='const ArtFrame loot_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in meta)+'};\n'
 (R/'generated/loot_art.c').write_text(c)
 (R/'generated/loot_art.h').write_text(h);sheet.save(R/'generated/loot-preview.png');print('Original loot:',len(data),'pixel bytes')

if '--prepare' not in sys.argv:
 import runpy
 runpy.run_path(str(R/'tools/fix_potion_art.py'))

if '--prepare' not in sys.argv:
 import runpy
 runpy.run_path(str(R/'tools/append_food_variants.py'))
