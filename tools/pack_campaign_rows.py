"""Lossless64-row chunks: vertical scrolling decodes only newly exposed bands."""
from pathlib import Path
import re,zlib
R=Path(__file__).resolve().parents[1]
def array(name,kind,values):return f'static const {kind} {name}[{len(values)}]={{\n'+''.join(','.join(map(str,values[i:i+32]))+',\n' for i in range(0,len(values),32))+'};\n'
def repack(name):
 prefix='campaign_'+name;p=R/f'generated/{prefix}.h';s=p.read_text();pattern=lambda n:r'static const \w+ '+n+r'\[[^]]*\]\s*=\s*\{(.*?)\};\n?'
 match=re.search(pattern(prefix+'_strip_data'),s,re.S)
 if not match:return
 data=bytes(map(int,re.findall(r'\d+',match[1])));offmatch=re.search(pattern(prefix+'_strip_offsets'),s,re.S);offsets=list(map(int,re.findall(r'\d+',offmatch[1])));strips=[zlib.decompress(data[a:b],-15) for a,b in zip(offsets,offsets[1:])];assert all(len(x)==32*448 for x in strips)
 (R/f'build/{prefix}-row-reference.bin').write_bytes(b''.join(strips))
 blob=bytearray();off=[0]
 for raw in strips:
  for first in range(0,448,64):
   chunk=raw[first*32:(first+64)*32];packed=zlib.compress(chunk,9)[2:-4];assert zlib.decompress(packed,-15)==chunk;blob+=packed;off.append(len(blob))
 s=re.sub(pattern(prefix+'_strip_offsets'),lambda m:array(prefix+'_row_offsets','uint32_t',off),s,flags=re.S)
 s=re.sub(pattern(prefix+'_strip_data'),lambda m:array(prefix+'_row_data','uint8_t',blob),s,flags=re.S)
 decoder=f'''/*64rowchunks bound per-pan decode work; only2048bytes temporary stack. */
static inline int {prefix}_decode_rows(unsigned strip,unsigned first,unsigned count,uint8_t *out){{
 if(strip>={prefix.upper()}_STRIP_COUNT||first>448||count>448-first||!out)return 0;
 uint8_t chunk[32*64];
 while(count){{
  unsigned local=first&63,take=64-local;if(take>count)take=count;
  unsigned block=strip*7+first/64,offset={prefix}_row_offsets[block];
  uint8_t *target=local==0&&take==64?out:chunk;
  if(!asset_inflate({prefix}_row_data+offset,{prefix}_row_offsets[block+1]-offset,target,32*64))return 0;
  if(target==chunk)memcpy(out,chunk+local*32,take*32);
  out+=take*32;first+=take;count-=take;
 }}
 return 1;
}}
static inline int {prefix}_decode_strip(unsigned strip,uint8_t *out){{
 return {prefix}_decode_rows(strip,0,448,out);
}}
'''
 s=re.sub(r'static inline int '+prefix+r'_decode_strip\(unsigned strip,uint8_t \*out\)\{.*?\n\}',lambda m:decoder,s,flags=re.S)
 s=s.replace('#include <stdint.h>','#include <stdint.h>\n#include <string.h>\n#include "asset_inflate.h"');p.write_text(s)
 print(name,'rows codec',len(data)+len(offsets)*4,'->',len(blob)+len(off)*4,'delta',len(blob)+len(off)*4-len(data)-len(offsets)*4)
if __name__=='__main__':
 import sys
 for name in sys.argv[1:] or ('world','shop','barracks'):repack(name)

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
