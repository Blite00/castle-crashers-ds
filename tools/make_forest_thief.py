"""Per-chapter bank copy of the thief frame stream: python make_forest_thief.py [forest|thieves].

A bank holds one chapter, so War's resident copy is not available elsewhere; each chapter that fields thieves carries its own."""
from pathlib import Path
import re,sys
R=Path(__file__).resolve().parents[1]
NAME=sys.argv[1] if len(sys.argv)>1 else 'forest'
s=(R/'generated/thief_art.h').read_text()
s=re.sub(r'/\* CHAPTER_ASSET_BEGIN thief_pixels \*/\n#if.*?\n#else\n(.*?)\n#endif\n/\* CHAPTER_ASSET_END thief_pixels \*/',lambda m:m[1],s,flags=re.S)
m=re.search(r'static const uint8_t\s+thief_pixels\s*\[(\d*)\]\s*=\s*\{(.*?)\};',s,re.S)
vals=list(map(int,re.findall(r'\d+',m[2])))
template='''#ifndef @U@_THIEF_ART_H
#define @U@_THIEF_ART_H
#include "thief_art.h"
static const uint8_t @N@_thief_pixels[@LEN@]={
@DATA@};
static inline int @N@_thief_decode_frame(unsigned id,uint8_t*out){
 if(id>=THIEF_FRAME_COUNT||!out)return 0;
 const ArtFrame*f=&thief_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(@N@_thief_pixels))return 0;
  unsigned token=@N@_thief_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(@N@_thief_pixels))return 0;unsigned v=@N@_thief_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else {if(n>sizeof(@N@_thief_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=@N@_thief_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''
data=''.join(','.join(map(str,vals[i:i+32]))+',\n' for i in range(0,len(vals),32))
out=template.replace('@U@',NAME.upper()).replace('@N@',NAME).replace('@LEN@',str(len(vals))).replace('@DATA@',data)
(R/f'generated/{NAME}_thief_art.h').write_text(out);print(NAME,'thief pixels',len(vals))
