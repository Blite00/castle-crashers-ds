"""Losslessly compress menu terrain, decoding in place in the caller's screen."""
from pathlib import Path
import re,zlib
R=Path(__file__).resolve().parents[1]
def optimize():
 p=R/'generated/menu_art.c';s=p.read_text();match=re.search(r'const uint8_t menu_background\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)
 if not match:return
 raw=bytes(map(int,re.findall(r'\d+',match[1])));packed=zlib.compress(raw,9)[2:-4];assert zlib.decompress(packed,-15)==raw
 (R/'build/menu-background-reference.bin').write_bytes(raw)
 data='static const uint8_t menu_background_packed['+str(len(packed))+']={\n'+','.join(map(str,packed))+'\n};'
 s=s[:match.start()]+data+s[match.end():]
 s+='''\nvoid menu_draw_background(uint16_t *pixels){
 uint8_t *indices=(uint8_t *)pixels;
 if(!asset_inflate(menu_background_packed,sizeof(menu_background_packed),indices,49152)){
  for(unsigned i=0;i<49152;i++)pixels[i]=0x8000;
  return;
 }
 for(unsigned i=49152;i--;)pixels[i]=menu_palette[indices[i]];
}
'''
 s=s.replace('#include "menu_art.h"','#include "menu_art.h"\n#include "asset_inflate.h"');p.write_text(s)
 p=R/'generated/menu_art.h';s=p.read_text();s=re.sub(r'extern const uint8_t menu_background\[[^]]*\];','void menu_draw_background(uint16_t *pixels);',s);p.write_text(s)
 print('Menu lossless bytes',len(raw),'->',len(packed),'saved',len(raw)-len(packed))
if __name__=='__main__':optimize()
