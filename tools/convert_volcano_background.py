"""Pack PvP scenery into shared-palette background, depth props and foreground."""
from pathlib import Path
from PIL import Image
import json,math
ROOT=Path(__file__).resolve().parents[1]
SIZE=(424,318)
def load(path):return Image.open(path).convert('RGBA').resize(SIZE,Image.Resampling.LANCZOS)
bg=load(ROOT/'assets/arena-volcano/1.png')
props=[]
for meta in json.loads((ROOT/'assets/arena-volcano-props.json').read_text()):
    im=load(ROOT/f"assets/arena-volcano-props/{meta['frame']}.png")
    bbox=im.getchannel('A').point(lambda v:255 if v>=80 else 0).getbbox()
    if bbox:props.append((meta,im,bbox))
props.sort(key=lambda p:p[0]['depth_y'])
fg=load(ROOT/'assets/arena-volcano-foreground/1.png'); bbox=fg.getchannel('A').point(lambda v:255 if v>=80 else 0).getbbox()
layers=props+[({'sprite':159,'depth_y':32767},fg,bbox)]
samples=list(bg.convert('RGB').getdata())
for _,im,box in layers:samples.extend((r,g,b) for r,g,b,a in im.crop(box).getdata() if a>=80)
sheet=Image.new('RGB',(512,math.ceil(len(samples)/512)));sheet.putdata(samples)
q=sheet.quantize(colors=255,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
pal=q.getpalette(); colors=[0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
def indexed(im):
    rgb=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
    return [v+1 if a>=80 else 0 for v,a in zip(rgb.getdata(),im.getchannel('A').getdata())]
def array(name,typ,data):
    return f'static const {typ} {name}[{len(data)}]={{\n'+''.join(','.join(map(str,data[i:i+32]))+',\n' for i in range(0,len(data),32))+'};\n'
metadata=[];pixels=[]
for meta,im,box in layers:
    x,y,right,bottom=box;metadata.append((len(pixels),x,y,right-x,bottom-y,round(meta['depth_y'])))
    pixels.extend(indexed(im.crop(box)))
# Hardware backdrop: exactly the same indexed, thresholded prop composition as
# the software renderer. Foreground remains separate above the actor overlay.
base_pixels=indexed(bg)
composed=base_pixels.copy()
occlusion=[0]*(SIZE[0]*SIZE[1])
assert len(props)<256
for layer_id,(offset,x,y,width,height,depth) in enumerate(metadata[:len(props)],1):
    for row in range(height):
        for column in range(width):
            color=pixels[offset+row*width+column]
            dx,dy=x+column,y+row
            if color and 0<=dx<SIZE[0] and 0<=dy<SIZE[1]:
                dest=dy*SIZE[0]+dx
                composed[dest]=color
                occlusion[dest]=layer_id
# Props are already in composed pixels. Runtime needs their depth metadata only.
fg_offset=metadata[-1][0]
pixels=pixels[fg_offset:]
metadata=[(0,*m[1:]) for m in metadata]
header='#ifndef ARENA_VOLCANO_H\n#define ARENA_VOLCANO_H\n#include <stdint.h>\n'

header+=f'#define VOLCANO_PROP_COUNT {len(props)}\n#define VOLCANO_FOREGROUND_INDEX {len(props)}\n'
header+=array('volcano_palette','uint16_t',colors)
header+=array('volcano_composed_pixels','uint8_t',composed)
header+=array('volcano_occlusion','uint8_t',occlusion)
header+=array('volcano_layer_pixels','uint8_t',pixels)
header+='static const ArenaSceneryLayer volcano_layers['+str(len(metadata))+']={'+','.join('{%d,%d,%d,%d,%d,%d}'%m for m in metadata)+'};\n#endif\n'
(ROOT/'generated/arena_volcano.h').write_text(header)
preview=bg.copy()
for _,im,_ in layers:preview.alpha_composite(im)
preview.convert('RGB').save(ROOT/'generated/arena-volcano-background.png')
bg.save(ROOT/'generated/arena-volcano-base.png');fg.save(ROOT/'generated/arena-volcano-foreground.png')
print('Scenery:',len(props),'depth-sorted props;',len(pixels),'layer bytes')

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
