"""Convert the installed game's menu illustration and bitmap font for DS.
Requires assets/menu-vignettes/1.png, exported from MENU_VIGNETTES.swf.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import struct, argparse
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(); p.add_argument('--game-directory',default=__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers')); args=p.parse_args()
fonts=Path(args.game_directory)/'data/fonts'
data=(fonts/'synjUnicode_20.fnt').read_bytes(); pos=4; chars={}
while pos<len(data):
    typ=data[pos]; size=struct.unpack_from('<I',data,pos+1)[0]; block=data[pos+5:pos+5+size]; pos+=5+size
    if typ==4:
        for i in range(0,size,20):
            cid,x,y,w,h,xo,yo,advance,page,channel=struct.unpack_from('<IHHHHhhhBB',block,i)
            chars[cid]=(x,y,w,h,xo,yo,advance)
atlas=Image.open(fonts/'synjUnicode_20_0.dds').convert('RGBA')
def lettering(text,height):
    width=sum(chars[ord(c)][6] for c in text)
    out=Image.new('RGBA',(width+8,70)); xx=0
    for c in text:
        x,y,w,h,xo,yo,advance=chars[ord(c)]
        out.alpha_composite(atlas.crop((x,y,x+w,y+h)),(xx+xo+2,yo)); xx+=advance
    bbox=out.getbbox(); out=out.crop(bbox)
    return out.resize((round(out.width*height/out.height),height),Image.Resampling.LANCZOS)
bg=Image.open(ROOT/'assets/menu-vignettes/1.png').convert('RGB')
# Preserve the original vignette, framing its painter character on the right.
bg=bg.resize((341,192),Image.Resampling.LANCZOS).crop((45,0,301,192)).convert('RGBA')
shade=Image.new('RGBA',(256,192)); d=ImageDraw.Draw(shade)
for x in range(256): d.line((x,0,x,191),fill=(0,0,0,max(32,175-x//2)))
bg.alpha_composite(shade)
# Original game font; compact title is adapted to the DS resolution.
for label,height,y in [('CASTLE',22,10),('CRASHERS',22,34)]:
    ink=lettering(label,height); x=12
    shadow=Image.new('RGBA',ink.size,(0,0,0,255)); shadow.putalpha(ink.getchannel('A'))
    bg.alpha_composite(shadow,(x+1,y+2)); bg.alpha_composite(ink,(x,y))
bg=bg.convert('RGB').quantize(colors=256)
background=list(bg.getdata()); pal=bg.getpalette(); palette=[0x8000|(pal[i*3]>>3)|((pal[i*3+1]>>3)<<5)|((pal[i*3+2]>>3)<<10) for i in range(256)]
labels=['Practice','Multiplayer','Controls','Host Room','Join Room','Back','ARENA','LOCAL WIRELESS','A SELECT','B BACK']
alpha=[]; meta=[]
for label in labels:
    ink=lettering(label,12 if label not in ('ARENA','LOCAL WIRELESS','A SELECT','B BACK') else 8)
    meta.append((len(alpha),ink.width,ink.height)); alpha.extend(ink.getchannel('A').getdata())
def array(name,typ,values):return 'const '+typ+' '+name+'['+str(len(values))+']={\n'+','.join(str(x) for x in values)+'\n};\n'
(ROOT/'generated/menu_art.h').write_text('#ifndef MENU_ART_H\n#define MENU_ART_H\n#include <stdint.h>\ntypedef struct {uint32_t offset; uint16_t w,h;} MenuLabel;\nextern const uint16_t menu_palette[256];\nextern const uint8_t menu_background[49152];\nextern const uint8_t menu_alpha['+str(len(alpha))+'];\nextern const MenuLabel menu_labels[10];\n#endif\n')
(ROOT/'generated/menu_art.c').write_text('#include "menu_art.h"\n'+array('menu_palette','uint16_t',palette)+array('menu_background','uint8_t',background)+array('menu_alpha','uint8_t',alpha)+'const MenuLabel menu_labels[10]={'+','.join('{%d,%d,%d}'%m for m in meta)+'};\n')
# Preview the same palette and placement used by menu.c.
preview=bg.convert('RGB'); d=ImageDraw.Draw(preview)
for row,label in enumerate(labels[:3]):
    ink=lettering(label,12); color=(255,222,90) if row==1 else (255,255,255)
    colored=Image.new('RGB',ink.size,color); preview.paste(colored,(24,82+row*26),ink.getchannel('A'))
d.polygon([(12,110),(18,113),(12,116)],fill=(255,222,90))
for txt,xy in [('ARENA',(13,63)),('A SELECT',(13,174))]:
    ink=lettering(txt,8); preview.paste((255,255,255),(*xy,xy[0]+ink.width,xy[1]+ink.height),ink.getchannel('A'))
preview.save(ROOT/'assets/menu-preview.png')
print('Menu bytes:',len(background)+512+len(alpha),'font/illustration imported from installed game')
# ASCII subset for dynamic scores, room messages and controls. Retain baselines.
font_alpha=[]; glyphs=[]
for cid in range(32,127):
    x,y,w,h,xo,yo,advance=chars[cid]
    glyph=Image.new('RGBA',(max(1,advance+4),70))
    glyph.alpha_composite(atlas.crop((x,y,x+w,y+h)),(xo+2,yo))
    glyph=glyph.resize((max(1,round((advance+4)*.2)),14),Image.Resampling.LANCZOS)
    glyphs.append((len(font_alpha),glyph.width,14)); font_alpha.extend(glyph.getchannel('A').getdata())
header=ROOT/'generated/menu_art.h'; header.write_text(header.read_text().replace('#endif','extern const uint8_t menu_font_alpha['+str(len(font_alpha))+'];\nextern const MenuLabel menu_glyphs[95];\n#endif'))
with (ROOT/'generated/menu_art.c').open('a') as f:
    f.write(array('menu_font_alpha','uint8_t',font_alpha))
    f.write('const MenuLabel menu_glyphs[95]={'+','.join('{%d,%d,%d}'%m for m in glyphs)+'};\n')

from optimize_menu import optimize
optimize()

from optimize_menu_alpha import optimize as optimize_alpha
optimize_alpha()

# Main menu now uses menu_text; preserve IDs but omit unused labels0..2.
import runpy
runpy.run_path(str(ROOT/'tools/prune_menu_labels.py'))
