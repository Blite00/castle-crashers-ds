"""Four original attribute symbols, 16x16 and four-bit palette indices."""
from pathlib import Path
from PIL import Image
import struct,json,hashlib
R=Path(__file__).resolve().parents[1];ims=[];bounds=[]
for i in range(1,5):
 im=Image.open(R/f'assets/ui-attribute-icons/{i}.png').convert('RGBA');b=im.getbbox();assert b;bounds.append(b);im=im.crop(b);im.thumbnail((14,14),Image.Resampling.LANCZOS);icon=Image.new('RGBA',(16,16));icon.alpha_composite(im,((16-im.width)//2,(16-im.height)//2));ims.append(icon)
train=Image.new('RGB',(16,64),(0,0,0))
for i,im in enumerate(ims):train.paste(im.convert('RGB'),(0,i*16))
q=train.quantize(colors=15,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette();palette=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,45,3)];data=[];hashes=[];preview=Image.new('RGB',(16*4,16),(235,218,174))
for i,im in enumerate(ims):
 a=im.getchannel('A').tobytes();v=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE).tobytes();raw=bytes(n+1 if aa>=80 else 0 for n,aa in zip(v,a));hv=2166136261
 for n in raw:hv=((hv^palette[n])*16777619)&0xffffffff
 hashes.append(hv);data.extend(raw[j]|(raw[j+1]<<4) for j in range(0,256,2));rgb=[]
 for n in raw:
  c=palette[n];rgb.append(((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31,255 if n else 0))
 show=Image.new('RGBA',(16,16));show.putdata(rgb);preview.paste(show,(i*16,0),show)
def arr(n,t,a):return f'static const {t} {n}[{len(a)}]={{'+','.join(map(str,a))+'};\n'
h='''/* Original MENU647 icons: shapes367,366,364,365. Indexed at16x16. */
#ifndef UI_ICON_ART_H
#define UI_ICON_ART_H
#include <stdint.h>
enum {UI_ICON_STRENGTH=0,UI_ICON_DEFENSE,UI_ICON_MAGIC,UI_ICON_AGILITY,UI_ICON_COUNT};
#define UI_ICON_WIDTH 16
#define UI_ICON_HEIGHT 16
#define UI_ICON_PIXELS 256
'''+arr('ui_icon_palette','uint16_t',palette)+arr('ui_icon_data','uint8_t',data)+'''static inline int ui_icon_decode(unsigned icon,uint8_t*out){
 if(icon>=UI_ICON_COUNT||!out)return 0;
 const uint8_t*p=ui_icon_data+icon*128;
 for(unsigned i=0;i<128;i++){out[i*2]=p[i]&15;out[i*2+1]=p[i]>>4;}
 return 1;
}
#endif
''';(R/'generated/ui_icon_art.h').write_text(h);(R/'generated/ui-icon-test-hashes.bin').write_bytes(struct.pack('<4I',*hashes));(R/'generated/ui-icon-audit.json').write_text(json.dumps(dict(source='MENU.swf',panel_sprite=647,shapes=[367,366,364,365],order=['Strength','Defense','Magic','Agility'],source_bounds=bounds,size=[16,16],static_bytes=544,decode_bytes=256,source_xml_sha256=hashlib.sha256((R/'assets/extracted/menu.xml').read_bytes()).hexdigest()),indent=2));preview.resize((512,128),Image.Resampling.NEAREST).save(R/'build/ui-icons-preview.png');print('UI icons:544B static,256B decode,source shapes367/366/364/365')
