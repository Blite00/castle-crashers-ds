"""Bake selected original FX timelines; --prepare precedes FFDec frame export."""
from copy import deepcopy
from pathlib import Path
import argparse
import hashlib
import json
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw
from prepare_frames import freeze
ROOT=Path(__file__).resolve().parents[1]
GROUPS=[('icebolt',484,8,3),('firebolt',484,8,4),('icesplash',1310,34,0),
        ('firesplash',1402,16,0),('frozen',1645,14,0),('burn',1637,14,0),
        ('ice_ready',35,14,3),('fire_ready',35,14,4),('blue_arrow',538,8,3),('orange_arrow',538,8,4),('lightningbolt',484,8,2),('poisonbolt',484,8,1),
        ('red_ready',35,14,2),('green_ready',35,14,1),('lightningsplash',1629,12,0),('poisonsplash',1397,31,0)]

def prepare():
    r=ET.parse(ROOT/'assets/extracted/fx.xml').getroot()
    definitions=[x for x in r.find('tags') if x.get('type','').startswith('Define')]
    originals={x.get('spriteId'):x for x in definitions if x.get('spriteId')}
    tags=r.find('tags');tags.clear();tags.extend(definitions)
    for index,(name,sid,count,variant) in enumerate(GROUPS):
        if variant:
            duplicate=deepcopy(originals[str(sid)]);sid=3000+index
            duplicate.set('spriteId',str(sid));freeze(duplicate,variant);tags.append(duplicate)
        ET.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
        p=ET.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',
            placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
        ET.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',
            translateX='0',translateY='-800' if index in (6,7,12,13) else '0',scaleX='1.0',scaleY='1.0')
        for _ in range(count):ET.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
    r.set('frameCount',str(sum(g[2] for g in GROUPS)))
    rect=r.find('displayRect')
    for key,value in [('Xmin',-5120),('Xmax',5120),('Ymin',-5120),('Ymax',5120)]:rect.set(key,str(value))
    ET.ElementTree(r).write(ROOT/'assets/extracted/effects.xml',encoding='utf-8',xml_declaration=True)

def convert():
    images=[];frame=1;groups=[]
    for name,sid,count,variant in GROUPS:
        chosen=[round(i*(count-1)/7) for i in range(8)]
        groups.append(dict(name=name,sprite=sid,variant=variant,source_frames=chosen))
        for i in chosen:
            im=Image.open(ROOT/f'assets/effects/{frame+i}.png').convert('RGBA').resize((256,256),Image.Resampling.LANCZOS)
            if name in ('frozen','burn'):
                # These are impact bursts, fitted to the DS knight, not status auras.
                small=im.resize((140,140),Image.Resampling.LANCZOS)
                im=Image.new('RGBA',(256,256));im.paste(small,(58,58))
            if name.endswith('_arrow'):
                im=im.rotate([5,2,0,-4,-8,-12,-18,-26][i],resample=Image.Resampling.BICUBIC,center=(128,128))
            if name=='lightningsplash':
                small=im.resize((140,140),Image.Resampling.LANCZOS)
                im=Image.new('RGBA',(256,256));im.paste(small,(58,58))
            if name.endswith('_ready'):
                # Keep the body anchor while limiting soft-particle fill on ARM9.
                small=im.resize((192,192),Image.Resampling.LANCZOS)
                im=Image.new('RGBA',(256,256));im.paste(small,(32,27))
            images.append(im)
        frame+=count
    # Downward cast uses the original projectile artwork at its travel angle.
    for name,source in [('ice_down',0),('fire_down',1),('lightning_down',10),('poison_down',11)]:
        groups.append(dict(name=name,derived_from_group=source,rotation_degrees=-35,source_frames=list(range(8))))
        images.extend(im.rotate(-35,resample=Image.Resampling.BICUBIC,center=(128,128)) for im in images[source*8:source*8+8])
    atlas=Image.new('RGB',(256,256*len(images)))
    for i,im in enumerate(images):atlas.paste(im,(0,i*256),im)
    palette=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT)
    p=palette.getpalette()[:765]
    colors=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,765,3)]
    pixels=[];meta=[];ready_alpha=[];ready_offsets=[];sheet=Image.new('RGB',(256*8,256*len(groups)),(55,55,55));draw=ImageDraw.Draw(sheet)
    for i,im in enumerate(images):
        raw_alpha=im.getchannel('A')
        soft=groups[i//8]['name'].endswith('_ready')
        alpha=raw_alpha.point(lambda a:255 if a>=(24 if soft else 80) else 0)
        box=alpha.getbbox()
        if not box:box=(128,128,129,129)
        x,y,r,b=box
        if x==0 or y==0 or r==256 or b==256:raise RuntimeError(f'Clipped effect frame {i}: {box}')
        q=im.convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE)
        meta.append((len(pixels),x-128,y-128,r-x,b-y))
        if soft:ready_offsets.append(len(ready_alpha))
        for yy in range(y,b):
            for xx in range(x,r):
                pixels.append(q.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0)
                if soft:ready_alpha.append(min(16,(raw_alpha.getpixel((xx,yy))+8)//16))
        sheet.paste(im,(i%8*256,i//8*256),im)
        draw.text((i%8*256,i//8*256),groups[i//8]['name'])
    if len(pixels)>190000:raise RuntimeError(f'Effects exceed budget: {len(pixels)}')
    def array(name,typ,values):
        return f'static const {typ} {name}[] = {{\n'+''.join(','.join(map(str,values[i:i+32]))+',\n' for i in range(0,len(values),32))+'};\n'
    header='#ifndef EFFECTS_ART_H\n#define EFFECTS_ART_H\n#include <stdint.h>\n'
    header+='typedef struct {uint32_t offset;int16_t x,y;uint16_t w,h;} EffectFrame;\n'
    header+=array('effect_palette','uint16_t',colors)+array('effect_pixels','uint8_t',pixels)
    header+=array('effect_ready_alpha','uint8_t',ready_alpha)+array('effect_ready_offsets','uint32_t',ready_offsets)
    header+='static const EffectFrame effect_frames[] = {\n'+''.join('{'+','.join(map(str,m))+'},\n' for m in meta)+'};\n#endif\n'
    (ROOT/'generated/effects_art.h').write_text(header)
    sheet.save(ROOT/'generated/effects-sheet.png')
    report=dict(source='data/game/fx.pak',swf_sha256=hashlib.sha256((ROOT/'assets/extracted/FX.swf').read_bytes()).hexdigest(),
        magic_ready_provenance='MAIN f_MagicClock emits glowset, FX sprite35 selects magic_type; variants3 ice and4 fire. Body offset -40 source pixels baked as -20 DS. Original randomized repeated particles adapted to a deterministic cycle.',
        arrow_provenance='FX arrow sprite538 is a character variant selector (f_ArrowColor), frozen at blue3/orange4. Render static frames, mirrored by travel direction; trajectory rotation not implemented.',
        impact_presentation='14-tick one-shot elemental bursts at 140/256 scale; final small ice shard held while frozen',arrow_angles=[5,2,0,-4,-8,-12,-18,-26],frames=len(images),pixel_bytes=len(pixels),ready_alpha_bytes=len(ready_alpha),ready_alpha_precision='0..16 alpha, ready glows only',palette_bytes=512,metadata_bytes=len(meta)*12,groups=groups)
    (ROOT/'generated/effects-audit.json').write_text(json.dumps(report,indent=2))
    from optimize_effects import optimize
    optimize()
    print(f'Imported {len(images)} original FX frames: {len(pixels)} pre-dedup pixel bytes')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--prepare',action='store_true')
    if parser.parse_args().prepare:prepare()
    else:convert()
