"""Preserve hot Barbarian poses as RLE; losslessly Huffman-pack rare poses."""
from pathlib import Path
import re,ast
from convert_barbarian import pack,unpack
R=Path(__file__).resolve().parents[1]
def main():
 s=(R/'generated/barbarian_art.h').read_text();data=bytes(map(int,re.findall(r'\d+',re.search(r'barbarian_pixels\[\d+\]\s*=\s*\{(.*?)\};',s,re.S)[1])));meta=[tuple(map(int,v)) for v in re.findall(r'\{(\d+),(-?\d+),(-?\d+),(\d+),(\d+)\}',s)]
 source=R/'tools/convert_assets.py';tree=ast.parse(source.read_text());ns={};exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef)],type_ignores=[]),str(source),'exec'),ns)
 starts=[0,13,29,42,54,57,63,76,90,108,124,139,154,166,175,181,187,189,190,207,215,228,233,245,265,277,291,304]
 common={meta[i][0] for pose in [1,2,5,16,23] for i in range(starts[pose],starts[pose+1])}
 raw=bytearray();rare=[];raredata=[];lookup={};ids=[];records=[];rareindex=[]
 for o,x,y,w,h in meta:
  if o not in lookup:
   lookup[o]=len(records);block=pack(unpack(data,o,w*h))
   if o in common:records.append((len(raw),len(block)));raw.extend(block)
   else:rareindex.append(len(records));records.append(None);rare.append((len(raredata),x,y,w,h));raredata.extend(block)
  ids.append(lookup[o])
 packed,rm,lengths,tables,trees=ns['knight_huffman_encode'](raredata,rare)
 for rid,m,n in zip(rareindex,rm,lengths):records[rid]=(m[0],n|32768)
 def arr(n,t,a):return f'static const {t} {n}[{len(a)}]={{'+','.join(map(str,a))+'};\n'
 h='#ifndef BARBARIAN_PACKED_H\n#define BARBARIAN_PACKED_H\n#include "barbarian_art.h"\n#include <string.h>\n'
 h+=arr('barbarian_hot_data','uint8_t',raw)+arr('barbarian_rare_data','uint8_t',packed)+arr('barbarian_packed_ids','uint8_t',ids)+arr('barbarian_packed_offsets','uint32_t',[r[0] for r in records])+arr('barbarian_packed_lengths','uint16_t',[r[1] for r in records])
 for i in range(2):
  h+=arr(f'barbarian_fast{i}','uint16_t',tables[i]);h+=f'static const int16_t barbarian_tree{i}[][2]={{'+','.join('{%d,%d}'%v for v in trees[i])+'};\n'
 h+='''static inline int barbarian_huff_symbol(const uint8_t**src,const uint8_t*end,uint32_t*bits,unsigned*count,unsigned context){
 const uint16_t*fast=context?barbarian_fast1:barbarian_fast0;
 const int16_t(*tree)[2]=context?barbarian_tree1:barbarian_tree0;
 while(*count<8&&*src<end){*bits=(*bits<<8)|*(*src)++;*count+=8;}
 unsigned peek=*count>=8?(*bits>>(*count-8))&255:(*bits<<(8-*count))&255;
 unsigned entry=fast[peek];if(entry&32768){unsigned n=(entry>>8)&31;if(*count<n)return -1;*count-=n;return entry&255;}
 if(*count<8)return -1;
 *count-=8;int node=(int)entry;
 for(unsigned depth=8;node>=0&&depth<24;depth++){if(!*count){if(*src==end)return -1;*bits=*(*src)++;*count=8;}node=tree[node][(*bits>>--*count)&1];}
 return node<0?-node-1:-1;
}
static inline int barbarian_decode_frame(unsigned id,uint8_t*out){
 if(id>=304||!out)return 0;
 const ArtFrame*f=&barbarian_frames[id];unsigned index=barbarian_packed_ids[id],length=barbarian_packed_lengths[index],offset=barbarian_packed_offsets[index],remaining=f->w*f->h;
 if(remaining>BARBARIAN_FRAME_PIXELS_MAX)return 0;
 if(!(length&32768)){
  if(offset+length>sizeof(barbarian_hot_data))return 0;
  const uint8_t*p=barbarian_hot_data+offset,*end=p+length;
  while(remaining){if(p==end)return 0;unsigned token=*p++,n=(token&127)+1;if(n>remaining)return 0;remaining-=n;
   if(token&128){if(p==end)return 0;memset(out,*p++,n);}else{if((unsigned)(end-p)<n)return 0;memcpy(out,p,n);p+=n;}out+=n;}
  return p==end;
 }
 length&=32767;if(offset+length>sizeof(barbarian_rare_data))return 0;
 const uint8_t*p=barbarian_rare_data+offset,*end=p+length;uint32_t bits=0;unsigned count=0;
 while(remaining){int token=barbarian_huff_symbol(&p,end,&bits,&count,0);if(token<0)return 0;unsigned n=(token&127)+1;if(n>remaining)return 0;remaining-=n;
  if(token&128){int value=barbarian_huff_symbol(&p,end,&bits,&count,1);if(value<0)return 0;memset(out,value,n);out+=n;}
  else while(n--){int value=barbarian_huff_symbol(&p,end,&bits,&count,1);if(value<0)return 0;*out++=(uint8_t)value;}
 }
 return 1;
}
#endif
'''
 (R/'generated/barbarian_packed.h').write_text(h);size=len(raw)+len(packed)+len(ids)+len(records)*6+1024+sum(len(t)*4 for t in trees)
 print('Mixed Barbarian static',size,'saved',len(data)-size,'hot RLE',len(raw),'rare Huffman',len(packed))
if __name__=='__main__':main()
