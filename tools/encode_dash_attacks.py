"""Bounded shared-layer Huffman bank for genuine heavy3/stomp keyframes."""
from pathlib import Path
import pickle,collections,heapq,json,struct
R=Path(__file__).resolve().parents[1];streams,base,colors,weapons,fixes,bounds,samples=pickle.loads((R/'build/dash-attack-factored.pkl').read_bytes())
src=(R/'tools/encode_held_weapons.py').read_text();exec(src[src.index('seqs=[]'):src.index("h='''")])
def arr(n,t,v):return f'static const {t} {n}[{len(v)}]={{'+','.join(map(str,v))+'};\n'
h='''/* Authored PLAYER737/736 keyframes. Source pose timing preserved by caller. */
#ifndef DASH_ATTACK_ART_H
#define DASH_ATTACK_ART_H
#include "castle_art.h"
#include <string.h>
'''
h+=f'#define DASH_ATTACK_PIXELS_MAX {max(w*hh for x,y,w,hh in bounds)}\n'
h+=arr('dash_attack_data','uint8_t',blob)+arr('dash_attack_offsets','uint16_t',offsets)+arr('dash_attack_sizes','uint16_t',lengths)
assert len(blob)<65536
for i in range(2):
 h+=f'static const int16_t dash_attack_tree{i}[][2]={{'+','.join('{%d,%d}'%v for v in trees[i])+'};\n'
 h+=arr(f'dash_attack_fast{i}','uint16_t',tables[i])
h+='static const ArtFrame dash_attack_frames[6]={'+','.join('{0,%d,%d,%d,%d}'%v for v in bounds)+'};\n'
h+='static const uint8_t dash_attack_streams[4][7][6][4]={'+','.join('{'+','.join('{'+','.join('{%d,%d,%d,%d}'%(base[k],weapons[w][k],colors[c][k],fixes[c][w][k]) for k in range(6))+'}' for w in range(7))+'}' for c in range(4))+'};\n'
h+='''typedef struct {const uint8_t*p,*end;uint32_t bits;unsigned count;int good;} DashAttackBits;
static inline int dash_attack_byte(DashAttackBits*b,unsigned context){
 const uint16_t*fast=context?dash_attack_fast1:dash_attack_fast0;
 const int16_t(*tree)[2]=context?dash_attack_tree1:dash_attack_tree0;
 while(b->count<8&&b->p<b->end){b->bits=(b->bits<<8)|*b->p++;b->count+=8;}
 int node=0;
 if(b->count>=8){unsigned entry=fast[(b->bits>>(b->count-8))&255];if(entry&32768){b->count-=(entry>>8)&31;return entry&255;}node=entry;b->count-=8;}
 while(node>=0){if(!b->count){if(b->p==b->end){b->good=0;return 0;}b->bits=*b->p++;b->count=8;}node=tree[node][(b->bits>>--b->count)&1];}
 return -node-1;
}
static inline ArtFrame dash_attack_frame(unsigned sample,unsigned color,unsigned weapon){
 if(sample>=6||color>=4||weapon>=7){ArtFrame empty={0,0,0,0,0};return empty;}
 return dash_attack_frames[sample];
}
static inline int dash_attack_decode(unsigned sample,unsigned color,unsigned weapon,uint8_t*out){
 if(sample>=6||color>=4||weapon>=7||!out)return 0;
 const ArtFrame*f=&dash_attack_frames[sample];unsigned size=f->w*f->h;
 if(size>DASH_ATTACK_PIXELS_MAX)return 0;
 memset(out,0,size);
 for(unsigned layer=0;layer<4;layer++){
  unsigned stream=dash_attack_streams[color][weapon][sample][layer],remaining=dash_attack_sizes[stream],cursor=0;
  DashAttackBits b={.p=dash_attack_data+dash_attack_offsets[stream],.end=dash_attack_data+dash_attack_offsets[stream+1],.good=1};
  while(remaining){
   unsigned skip=0,shift=0,v;
   do{if(!remaining||shift>14)return 0;v=(unsigned)dash_attack_byte(&b,0);remaining--;skip|=(v&127)<<shift;shift+=7;}while(v&128);
   if(!remaining)return 0;
   unsigned n=(unsigned)dash_attack_byte(&b,0);remaining--;cursor+=skip;
   int x=(int)(cursor%192)-96-f->x,y=(int)(cursor/192)-88-f->y;
   if(n>remaining||x<0||y<0||y>=f->h||x+(int)n>f->w)return 0;
   for(unsigned j=0;j<n;j++)out[y*f->w+x+j]=(uint8_t)dash_attack_byte(&b,1);
   remaining-=n;cursor+=n;
  }
  if(!b.good)return 0;
 }
 return 1;
}
#endif
'''
(R/'build/dash_attack_art.h').write_text(h)
# Native reference hashes, independent complete source target images.
bank=pickle.loads((R/'build/dash-attacks.pkl').read_bytes());out=bytearray()
for c in range(4):
 for w in range(9):
  for k,n in enumerate(samples):
   f=bank['meta'][(c*9+w)*24+n];raw=bank['raws'][(c*9+w)*24+n];_,x,y,ww,hh=f;bx,by,bw,bh=bounds[k];target=bytearray(bw*bh)
   for yy in range(hh):target[(y-by+yy)*bw+x-bx:(y-by+yy)*bw+x-bx+ww]=raw[yy*ww:(yy+1)*ww]
   hv=2166136261
   for v in target:hv=((hv^bank['palette'][v])*16777619)&0xffffffff
   out.extend(struct.pack('<I',hv))
(R/'generated/dash-attack-test-hashes.bin').write_bytes(out)
size=len(blob)+len(offsets)*2+len(lengths)*2+sum(len(t)*4 for t in trees)+1024+72+384
print('Default combo bank payload',len(blob),'total static',size,'maxpixels',max(w*hh for x,y,w,hh in bounds))
(R/'generated/dash-attack-audit.json').write_text(json.dumps(dict(source_sprites=[737,736],source_frames=[[1,5,12],[1,5,12]],knights=4,weapons=[1,2,9,19,10,38,30],payload_bytes=len(blob),static_bytes=size,max_pixels=max(w*hh for x,y,w,hh in bounds)),indent=2))
