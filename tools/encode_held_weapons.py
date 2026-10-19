"""Encode shared held-weapon patches with separate control/color Huffman tables."""
from pathlib import Path
import pickle,collections,heapq,json
from convert_barbarian import array
R=Path(__file__).resolve().parents[1];streams,common,fixes,bounds,maxpixels=pickle.loads((R/'build/held-factored.pkl').read_bytes())
seqs=[];counts=[collections.Counter(),collections.Counter()]
for s in streams:
 p=0;seq=[]
 while p<len(s):
  while True:
   v=s[p];p+=1;seq.append((0,v));counts[0][v]+=1
   if v<128:break
  n=s[p];p+=1;seq.append((0,n));counts[0][n]+=1
  for v in s[p:p+n]:seq.append((1,v));counts[1][v]+=1
  p+=n
 seqs.append(seq)
trees=[];tables=[];codes=[]
for count in counts:
 heap=[(v,k,k) for k,v in count.items()];heapq.heapify(heap);serial=256
 while len(heap)>1:
  a=heapq.heappop(heap);b=heapq.heappop(heap);heapq.heappush(heap,(a[0]+b[0],serial,(a[2],b[2])));serial+=1
 nodes=[];code={}
 def walk(tree,bits,n):
  if isinstance(tree,int):code[tree]=(bits,n);return -tree-1
  index=len(nodes);nodes.append(None);a=walk(tree[0],bits<<1,n+1);b=walk(tree[1],bits<<1|1,n+1);nodes[index]=(a,b);return index
 walk(heap[0][2],0,0);table=[]
 for v in range(256):
  node=0;used=0
  while node>=0 and used<8:node=nodes[node][(v>>(7-used))&1];used+=1
  table.append(32768|(used<<8)|(-node-1) if node<0 else node)
 trees.append(nodes);codes.append(code);tables.append(table)
blob=bytearray();offsets=[0];lengths=[]
for seq in seqs:
 bits=0;n=0
 for context,v in seq:
  value,size=codes[context][v];bits=(bits<<size)|value;n+=size
  while n>=8:n-=8;blob.append((bits>>n)&255)
 if n:blob.append((bits<<(8-n))&255)
 offsets.append(len(blob));lengths.append(len(seq))
h='''/* Original PLAYER f_Weapon selectors2/9/19/10/38/30. Exact hardware-color patches over base knight frames. */
#ifndef KNIGHT_WEAPON_PATCHES_H
#define KNIGHT_WEAPON_PATCHES_H
#include <string.h>
#include "castle_art.h"
'''
h+=f'#define KNIGHT_WEAPON_FRAME_PIXELS_MAX {maxpixels}\n'
h+=array('knight_weapon_patch_data','uint8_t',blob)+array('weapon_patch_offsets','uint32_t',offsets)+array('weapon_patch_sizes','uint16_t',lengths)
for i in range(2):
 h+=f'static const int16_t weapon_tree{i}[][2]={{'+','.join('{'+str(a)+','+str(b)+'}' for a,b in trees[i])+'};\n'
 h+=array(f'weapon_fast{i}','uint16_t',tables[i])
h+='static const uint16_t weapon_common[5][304]={'+','.join('{'+','.join(map(str,row))+'}' for row in common)+'};\n'
h+='static const uint16_t weapon_fix[4][5][304]={'+','.join('{'+','.join('{'+','.join(map(str,row))+'}' for row in color)+'}' for color in fixes)+'};\n'
h+='typedef struct {int8_t x,y;uint8_t w,h;} WeaponBounds;\nstatic const WeaponBounds weapon_bounds[5][304]={'+','.join('{'+','.join('{'+','.join(map(str,b))+'}' for b in row)+'}' for row in bounds)+'};\n'
h+='''typedef struct {const uint8_t *p,*end;uint32_t bits;unsigned count;int valid;} WeaponBits;
static inline unsigned weapon_byte(WeaponBits *b,unsigned color){
 const int16_t (*tree)[2]=color?weapon_tree1:weapon_tree0;const uint16_t *fast=color?weapon_fast1:weapon_fast0;
 while(b->count<8&&b->p<b->end){b->bits=(b->bits<<8)|*b->p++;b->count+=8;}
 int node=0;
 if(b->count>=8){unsigned code=fast[(b->bits>>(b->count-8))&255];if(code&32768){b->count-=(code>>8)&31;return code&255;}node=(int)code;b->count-=8;}
 while(node>=0){
  if(!b->count){if(b->p>=b->end){b->valid=0;return 0;}b->bits=*b->p++;b->count=8;}
  node=tree[node][(b->bits>>--b->count)&1];
 }
 return (unsigned)(-node-1);
}
static inline ArtFrame weapon_frame(unsigned id,unsigned slot){
 ArtFrame f=knight_frames[id];if(slot>=2&&slot<7){const WeaponBounds *b=&weapon_bounds[slot-2][id%304];f.x=b->x;f.y=b->y;f.w=b->w;f.h=b->h;}return f;
}
static inline int weapon_apply_stream(unsigned stream,uint8_t *out,const ArtFrame *f){
 WeaponBits b={.p=knight_weapon_patch_data+weapon_patch_offsets[stream],.end=knight_weapon_patch_data+weapon_patch_offsets[stream+1],.valid=1};
 unsigned remaining=weapon_patch_sizes[stream],cursor=0;
 while(remaining){
  unsigned skip=0,shift=0,value;
  do{if(!remaining||shift>14)return 0;value=weapon_byte(&b,0);remaining--;skip|=(value&127)<<shift;shift+=7;}while(value&128);
  if(!remaining)return 0;
  unsigned count=weapon_byte(&b,0);remaining--;cursor+=skip;
  int x=(int)(cursor%192)-96-f->x,y=(int)(cursor/192)-88-f->y;
  if(count>remaining||x<0||y<0||y>=f->h||x+(int)count>f->w)return 0;
  for(unsigned i=0;i<count;i++)out[y*f->w+x+i]=(uint8_t)weapon_byte(&b,1);
  remaining-=count;cursor+=count;
 }
 return b.valid;
}
static inline int weapon_apply(unsigned id,unsigned slot,uint8_t *out){
 if(id>=1216||slot>=7||!out)return 0;if(slot<2)return 1;
 const ArtFrame *base=&knight_frames[id];ArtFrame f=weapon_frame(id,slot);unsigned dx=base->x-f.x,dy=base->y-f.y;
 if(f.w*f.h>KNIGHT_WEAPON_FRAME_PIXELS_MAX)return 0;
 for(int row=base->h-1;row>=0;row--)memmove(out+(row+dy)*f.w+dx,out+row*base->w,base->w);
 memset(out,0,dy*f.w);
 for(unsigned row=dy;row<dy+base->h;row++){memset(out+row*f.w,0,dx);memset(out+row*f.w+dx+base->w,0,f.w-dx-base->w);}
 memset(out+(dy+base->h)*f.w,0,(f.h-dy-base->h)*f.w);
 return weapon_apply_stream(weapon_common[slot-2][id%304],out,&f)&&weapon_apply_stream(weapon_fix[id/304][slot-2][id%304],out,&f);
}
#endif
'''
h=h.replace('return 0;if(slot<2)','return 0;\n if(slot<2)');(R/'build/knight_weapon_patches.h').write_text(h)
size=len(blob)+len(offsets)*4+len(lengths)*2+sum(len(t)*4 for t in trees)+1024+1216+4864+2432
print('FINAL held weapon bytes',size,'payload',len(blob),'max frame',maxpixels)
audit=json.loads((R/'generated/held-weapon-audit.json').read_text());audit.update(total_bytes=size,data_bytes=len(blob),codec='shared control/color Huffman sparse patches',streams=len(streams));(R/'generated/held-weapon-audit.json').write_text(json.dumps(audit,indent=2))
