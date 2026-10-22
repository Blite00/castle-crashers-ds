"""Bounded shared-layer Huffman bank for genuine heavy3/stomp keyframes."""
from pathlib import Path
import pickle,collections,heapq,json,struct
R=Path(__file__).resolve().parents[1];streams,base,colors,weapons,fixes,bounds,samples=pickle.loads((R/'build/default-combo-factored.pkl').read_bytes())
src=(R/'tools/encode_held_weapons.py').read_text();exec(src[src.index('seqs=[]'):src.index("h='''")])
def arr(n,t,v):return f'static const {t} {n}[{len(v)}]={{'+','.join(map(str,v))+'};\n'
h='''/* Authored PLAYER608/734 keyframes. Source pose timing preserved by caller. */
#ifndef DEFAULT_COMBO_ART_H
#define DEFAULT_COMBO_ART_H
#include "castle_art.h"
#include <string.h>
'''
h+=f'#define DEFAULT_COMBO_PIXELS_MAX {max(w*hh for x,y,w,hh in bounds)}\n'
h+=arr('default_combo_data','uint8_t',blob)+arr('default_combo_offsets','uint16_t',offsets)+arr('default_combo_sizes','uint16_t',lengths)
assert len(blob)<65536
for i in range(2):
 h+=f'static const int16_t default_combo_tree{i}[][2]={{'+','.join('{%d,%d}'%v for v in trees[i])+'};\n'
 h+=arr(f'default_combo_fast{i}','uint16_t',tables[i])
h+='static const ArtFrame default_combo_frames[10]={'+','.join('{0,%d,%d,%d,%d}'%v for v in bounds)+'};\n'
h+='static const uint8_t default_combo_streams[4][7][10][4]={'+','.join('{'+','.join('{'+','.join('{%d,%d,%d,%d}'%(base[k],weapons[w][k],colors[c][k],fixes[c][w][k]) for k in range(10))+'}' for w in range(7))+'}' for c in range(4))+'};\n'
h+='''typedef struct {const uint8_t*p,*end;uint32_t bits;unsigned count;int good;} DefaultComboBits;
static inline int default_combo_byte(DefaultComboBits*b,unsigned context){
 const uint16_t*fast=context?default_combo_fast1:default_combo_fast0;
 const int16_t(*tree)[2]=context?default_combo_tree1:default_combo_tree0;
 while(b->count<8&&b->p<b->end){b->bits=(b->bits<<8)|*b->p++;b->count+=8;}
 int node=0;
 if(b->count>=8){unsigned entry=fast[(b->bits>>(b->count-8))&255];if(entry&32768){b->count-=(entry>>8)&31;return entry&255;}node=entry;b->count-=8;}
 while(node>=0){if(!b->count){if(b->p==b->end){b->good=0;return 0;}b->bits=*b->p++;b->count=8;}node=tree[node][(b->bits>>--b->count)&1];}
 return -node-1;
}
static inline ArtFrame default_combo_frame(unsigned sample,unsigned color,unsigned weapon){
 if(sample>=10||color>=4||weapon>=7){ArtFrame empty={0,0,0,0,0};return empty;}
 return default_combo_frames[sample];
}
static inline int default_combo_decode(unsigned sample,unsigned color,unsigned weapon,uint8_t*out){
 if(sample>=10||color>=4||weapon>=7||!out)return 0;
 const ArtFrame*f=&default_combo_frames[sample];unsigned size=f->w*f->h;
 if(size>DEFAULT_COMBO_PIXELS_MAX)return 0;
 memset(out,0,size);
 for(unsigned layer=0;layer<4;layer++){
  unsigned stream=default_combo_streams[color][weapon][sample][layer],remaining=default_combo_sizes[stream],cursor=0;
  DefaultComboBits b={.p=default_combo_data+default_combo_offsets[stream],.end=default_combo_data+default_combo_offsets[stream+1],.good=1};
  while(remaining){
   unsigned skip=0,shift=0,v;
   do{if(!remaining||shift>14)return 0;v=(unsigned)default_combo_byte(&b,0);remaining--;skip|=(v&127)<<shift;shift+=7;}while(v&128);
   if(!remaining)return 0;
   unsigned n=(unsigned)default_combo_byte(&b,0);remaining--;cursor+=skip;
   int x=(int)(cursor%192)-96-f->x,y=(int)(cursor/192)-88-f->y;
   if(n>remaining||x<0||y<0||y>=f->h||x+(int)n>f->w)return 0;
   for(unsigned j=0;j<n;j++)out[y*f->w+x+j]=(uint8_t)default_combo_byte(&b,1);
   remaining-=n;cursor+=n;
  }
  if(!b.good)return 0;
 }
 return 1;
}
#endif
'''
(R/'build/default_combo_art.h').write_text(h)
# Native reference hashes, independent complete source target images.
bank=pickle.loads((R/'build/default-combos.pkl').read_bytes());out=bytearray()
for c in range(4):
 for w in range(12):
  for k,n in enumerate(samples):
   f=bank['meta'][(c*12+w)*36+n];raw=bank['raws'][(c*12+w)*36+n];_,x,y,ww,hh=f;bx,by,bw,bh=bounds[k];target=bytearray(bw*bh)
   for yy in range(hh):target[(y-by+yy)*bw+x-bx:(y-by+yy)*bw+x-bx+ww]=raw[yy*ww:(yy+1)*ww]
   hv=2166136261
   for v in target:hv=((hv^bank['palette'][v])*16777619)&0xffffffff
   out.extend(struct.pack('<I',hv))
(R/'generated/default-combo-test-hashes.bin').write_bytes(out)
size=len(blob)+len(offsets)*2+len(lengths)*2+sum(len(t)*4 for t in trees)+1024+120+640
print('Default combo bank payload',len(blob),'total static',size,'maxpixels',max(w*hh for x,y,w,hh in bounds))
(R/'generated/default-combo-audit.json').write_text(json.dumps(dict(source_sprites=[608,734],source_frames=[[1,4,7,10],[1,6,9,12,15,20]],knights=4,weapons=[1,2,9,19,10,38,30],payload_bytes=len(blob),static_bytes=size,max_pixels=max(w*hh for x,y,w,hh in bounds)),indent=2))
