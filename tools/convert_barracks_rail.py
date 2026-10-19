"""Exported original LEVEL2 sprite303 near rail, encoded as exact binary alpha row spans.
Rebuild source: prepare_barracks_rail.py; FFDec xml2swf and export frame to assets/barracks-rail.
"""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
im=Image.open(R/'assets/barracks-rail/3.png').convert('RGBA').resize((1280,448),Image.Resampling.LANCZOS)
a=im.getchannel('A').point(lambda v:255 if v>=96 else 0);box=a.getbbox();x,y,right,bottom=box;w=right-x;h=bottom-y;a=a.crop(box)
runs=[];rows=[]
for yy in range(h):
 rows.append(len(runs));xx=0
 while xx<w:
  if not a.getpixel((xx,yy)):xx+=1;continue
  start=xx
  while xx<w and a.getpixel((xx,yy)):xx+=1
  runs.append((start,xx))
rows.append(len(runs));assert len(runs)<256 and w<256
s='''#ifndef BARRACKS_RAIL_ART_H
#define BARRACKS_RAIL_ART_H
#include <stdint.h>
/* Source LEVEL2 sprite303: near-side wooden rail and its brackets.
   The source far rail is baked scenery and must not occlude stair users. */
'''
s+=f'#define BARRACKS_RAIL_X {x}\n#define BARRACKS_RAIL_Y {y}\n#define BARRACKS_RAIL_W {w}\n#define BARRACKS_RAIL_H {h}\n'
s+='static const uint8_t barracks_rail_rows[]={'+','.join(map(str,rows))+'};\n'
s+='static const uint8_t barracks_rail_runs[][2]={'+','.join('{%d,%d}'%r for r in runs)+'};\n'
s+='''static inline int barracks_rail_pixel(int x,int y){
 x-=BARRACKS_RAIL_X;y-=BARRACKS_RAIL_Y;
 if((unsigned)x>=BARRACKS_RAIL_W||(unsigned)y>=BARRACKS_RAIL_H)return 0;
 for(unsigned i=barracks_rail_rows[y];i<barracks_rail_rows[y+1];i++)
  if(x>=barracks_rail_runs[i][0]&&x<barracks_rail_runs[i][1])return 1;
 return 0;
}
static inline int barracks_rail_behind(int footx,int footy){
 /* Use the solid BSP2 riser, not the helper plane several pixels above
    it. Otherwise a hero pressed against the rail changes to front depth. */
 static const int16_t edge[][4]={{953,276,959,272},{959,272,980,258},
  {980,258,1025,227},{1025,227,1092,182},{1092,182,1100,177},
  {1100,177,1124,172},{1124,172,1131,170}};
 if(footx<938||footx>1151)return 0;
 unsigned i=0;while(i<6&&footx>edge[i][2])i++;
 const int16_t *e=edge[i];
 return (footy-e[1])*(e[2]-e[0])<=(footx-e[0])*(e[3]-e[1]);
}
#endif
'''
(R/'generated/barracks_rail.h').write_text(s)
# Pixel coverage reference hash independent of native row-span decoder.
hash=2166136261
for yy in range(448):
 for xx in range(1280):
  v=int(x<=xx<right and y<=yy<bottom and bool(a.getpixel((xx-x,yy-y))))
  hash=((hash^v)*16777619)&0xffffffff
(R/'generated/barracks_rail_hash.h').write_text(f'#define BARRACKS_RAIL_REFERENCE_HASH {hash}u\n')
print('Original rail bounds',box,'exact alpha spans bytes',len(rows)+len(runs)*2,'hash',hash)
