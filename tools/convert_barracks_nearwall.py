"""Exported original LEVEL2 sprite305 complete near wall, encoded as exact binary alpha row spans.
Rebuild source: prepare_barracks_nearwall.py; FFDec xml2swf and export frame to assets/barracks-rail.
"""
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parents[1]
im=Image.open(R/'assets/barracks-rail/4.png').convert('RGBA').resize((1280,448),Image.Resampling.LANCZOS)
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
s='''#ifndef BARRACKS_NEARWALL_ART_H
#define BARRACKS_NEARWALL_ART_H
#include <stdint.h>
/* Source LEVEL2 sprite305: near jamb, pillar and complete foreground wall.
   Source45 is the far frame and remains background. */
'''
s+=f'#define BARRACKS_NEARWALL_X {x}\n#define BARRACKS_NEARWALL_Y {y}\n#define BARRACKS_NEARWALL_W {w}\n#define BARRACKS_NEARWALL_H {h}\n'
s+='static const uint8_t barracks_nearwall_rows[]={'+','.join(map(str,rows))+'};\n'
s+='static const uint8_t barracks_nearwall_runs[][2]={'+','.join('{%d,%d}'%r for r in runs)+'};\n'
s+='''static inline int barracks_nearwall_pixel(int x,int y){
 x-=BARRACKS_NEARWALL_X;y-=BARRACKS_NEARWALL_Y;
 if((unsigned)x>=BARRACKS_NEARWALL_W||(unsigned)y>=BARRACKS_NEARWALL_H)return 0;
 for(unsigned i=barracks_nearwall_rows[y];i<barracks_nearwall_rows[y+1];i++)
  if(x>=barracks_nearwall_runs[i][0]&&x<barracks_nearwall_runs[i][1])return 1;
 return 0;
}
static inline int barracks_nearwall_behind(int footx,int footy){
 /* Include the two rasterized rows below source305's181foot origin.
    Otherwise a legal stair step at y182 briefly draws across its last row. */
 return footx>=1065&&footy<BARRACKS_NEARWALL_Y+BARRACKS_NEARWALL_H;
}
static inline int barracks_nearwall_departing(int footx){
 /* Source movie223 has staticdepth410, below wall305 at590: authored
    departing knights stay behind this wall regardless of their foot Y. */
 return footx>=1065;
}
#endif
'''
(R/'generated/barracks_nearwall.h').write_text(s)
# Pixel coverage reference hash independent of native row-span decoder.
hash=2166136261
for yy in range(448):
 for xx in range(1280):
  v=int(x<=xx<right and y<=yy<bottom and bool(a.getpixel((xx-x,yy-y))))
  hash=((hash^v)*16777619)&0xffffffff
(R/'generated/barracks_nearwall_hash.h').write_text(f'#define BARRACKS_NEARWALL_REFERENCE_HASH {hash}u\n')
print('Original rail bounds',box,'exact alpha spans bytes',len(rows)+len(runs)*2,'hash',hash)
