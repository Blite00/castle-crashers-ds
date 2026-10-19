from pathlib import Path
import ast,struct,math
from PIL import Image
R=Path(__file__).resolve().parents[1]
# Share the exact bounded asset codec without running its terrain exporter.
tree=ast.parse((R/'tools/convert_campaign_world.py').read_text());ns={}
exec(compile(ast.Module(body=[x for x in tree.body if isinstance(x,(ast.Import,ast.ImportFrom,ast.FunctionDef))],type_ignores=[]),'worldcodec','exec'),ns)
pack,unpack,arr=[ns[x] for x in ('pack','unpack','arr')]
W,H,SW=1024,448,32
im=Image.open(R/'assets/campaign-shop/1.png').convert('RGBA').resize((W,H),Image.Resampling.LANCZOS)
p=im.load()
# Original stage stops drawing below its floor; extend each bottom edge under camera overscan.
for x in range(W):
 y=H-1
 while y>=0 and p[x,y][3]<250:y-=1
 if y>=0:
  for k in range(y+1,H):p[x,k]=p[x,y]
bg=Image.new('RGBA',(W,H),(81,104,115,255));bg.alpha_composite(im)
q=bg.convert('RGB').quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette()
pal=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,384,3)]+[0]*127
# DS RGB555 collapses many of the quantizer's RGB888 entries. Merge identical
# hardware colors before compression; rendered pixels remain exactly unchanged.
original_pal=pal[:];unique=[0];indices={0:0};remap=[]
for color in original_pal:
 if color not in indices:indices[color]=len(unique);unique.append(color)
 remap.append(indices[color])
pixels=bytes(remap[v+1] for v in q.tobytes());pal=unique+[0]*(256-len(unique))
assert all(original_pal[v+1]==pal[pixels[i]] for i,v in enumerate(q.tobytes()))
q.convert('RGB').save(R/'generated/campaign-shop-preview.png')
blob=bytearray();offs=[0]
for x in range(0,W,SW):
 data=b''.join(pixels[y*W+x:y*W+x+SW] for y in range(H));b=pack(data);assert unpack(b)==data;blob+=b;offs.append(len(blob))
h='/* Original LEVEL19 scene164, open shop; origin source(400,-600), half scale. */\n#ifndef CAMPAIGN_SHOP_H\n#define CAMPAIGN_SHOP_H\n#include <stdint.h>\n#define CAMPAIGN_SHOP_WIDTH 1024\n#define CAMPAIGN_SHOP_HEIGHT 448\n#define CAMPAIGN_SHOP_STRIP_COUNT 32\n#define CAMPAIGN_SHOP_STRIP_PIXELS (32*448)\n'
h+=arr('campaign_shop_palette','uint16_t',pal)+arr('campaign_shop_strip_offsets','uint32_t',offs)+arr('campaign_shop_strip_data','uint8_t',blob)
h+='''/* Requires campaign_world.h's shared bounded decoder. */
static inline int campaign_shop_decode_strip(unsigned strip,uint8_t *out){
 if(strip>=CAMPAIGN_SHOP_STRIP_COUNT)return 0;
 unsigned off=campaign_shop_strip_offsets[strip];
 return campaign_world_decode_bytes(campaign_shop_strip_data+off,campaign_shop_strip_offsets[strip+1]-off,out,CAMPAIGN_SHOP_STRIP_PIXELS);
}
#endif
'''
(R/'generated/campaign_shop.h').write_text(h)
b=(R/'assets/extracted/bsp19.bin').read_bytes();n=struct.unpack_from('<I',b,16)[0]//8
edges=[struct.unpack_from('<8f',b,20+i*32)[1:6] for i in range(n)]
mins=[];maxs=[]
for x in range(W):
 wx=2*x+400;ys=[]
 for x1,y1,x2,y2,k in edges:
  if k in (1,2,3) and min(x1,x2)<=wx<max(x1,x2):ys.append(y1+(y2-y1)*(wx-x1)/(x2-x1))
 if not ys:ys=[-365,-220] if wx<800 else [-65,75]
 lo=max(8,min(440,math.ceil((min(ys)+600)/2)+5));hi=max(lo,min(443,math.floor((max(ys)+600)/2)-5));mins.append(lo);maxs.append(hi)
h='#ifndef CAMPAIGN_SHOP_GEOMETRY_H\n#define CAMPAIGN_SHOP_GEOMETRY_H\n#include <stdint.h>\n/* BSP19 outer walk corridor; world=(source-(400,-600))/2. */\n'
h+=arr('campaign_shop_floor_min','uint16_t',mins)+arr('campaign_shop_floor_max','uint16_t',maxs)+'#endif\n';(R/'generated/campaign_shop_geometry.h').write_text(h)
print('Shop ROM data:',len(blob)+512+len(offs)*4,'bytes; shared scratch14336; geometry4096')
for x in [178,514,589,639,664,689,739,759,965]:print(x,mins[x],maxs[x])

from pack_campaign_rows import repack
repack("shop")

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
