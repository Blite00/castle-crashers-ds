"""Pack original MAIN.swf HUD shapes, knight portraits and meter artwork."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
def load(p): return Image.open(ROOT/p).convert('RGBA')
images=[load('assets/hud-shapes/169.png')]
for frame in (3,5):images.append(load(f'assets/portraits/DefineSprite_167/{frame}.png').resize((43,44),Image.Resampling.LANCZOS))
for sid in (182,192):
    im=load(f'assets/hud-parts/DefineSprite_{sid}/1.png'); images.append(im.crop(im.getbbox()))
for frame in (4,2):images.append(load(f'assets/portraits/DefineSprite_167/{frame}.png').resize((43,44),Image.Resampling.LANCZOS))
words=[]; meta=[]
for im in images:
    meta.append((len(words),im.width,im.height))
    words.extend(0 if a<80 else 0x8000|(r>>3)|((g>>3)<<5)|((b>>3)<<10) for r,g,b,a in im.getdata())
(ROOT/'generated/hud_art.h').write_text('#ifndef HUD_ART_H\n#define HUD_ART_H\n#include <stdint.h>\ntypedef struct {uint16_t offset,w,h;} HudImage;\nextern const HudImage hud_images[7];\nextern const uint16_t hud_pixels['+str(len(words))+'];\n#endif\n')
(ROOT/'generated/hud_art.c').write_text('#include "hud_art.h"\nconst HudImage hud_images[7]={'+','.join('{%d,%d,%d}'%x for x in meta)+'};\nconst uint16_t hud_pixels['+str(len(words))+']={'+','.join(str(x) for x in words)+'};\n')
print('Original HUD bytes:',len(words)*2)

from optimize_hud import optimize
optimize()
