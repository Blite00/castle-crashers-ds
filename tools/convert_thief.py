"""Bake the installed game's Thief variant without changing knight assets.

Run --prepare, export thief.xml through FFDec to assets/thief, then
run without arguments. Uses the same pose ordering and RLE as castle_art.h.
"""
from pathlib import Path
import argparse
import hashlib
import json
import xml.etree.ElementTree as ET
from PIL import Image
from prepare_frames import POSES, freeze

ROOT = Path(__file__).resolve().parents[1]

def prepare():
    r = ET.parse(ROOT/'assets/extracted/player.xml').getroot()
    definitions = []
    for x in r.find('tags'):
        if not x.get('type', '').startswith('Define'):
            continue
        definitions.append(x)
        if x.get('type') != 'DefineSpriteTag':
            continue
        actions = b''.join(bytes.fromhex(a.get('actionBytes', '')) for a in x.find('subTags') if a.get('type') == 'DoActionTag')
        if any(('f_'+s+'\0').encode() in actions for s in ['Helmet', 'Belt', 'Emblem', 'Shield']):
            freeze(x, 8)
        elif b'f_Weapon\0' in actions:
            freeze(x, 10)
        elif any(('f_'+s+'\0').encode() in actions for s in ['Wings', 'Flag']):
            freeze(x, 1)
        elif x.get('spriteId') == '371':
            freeze(x, 2)
        elif x.get('spriteId') == '289':
            freeze(x, 4)
        elif x.get('spriteId') == '438':
            freeze(x, 1)
        elif x.get('spriteId') in ('436', '780'):
            x.find('subTags').clear()
            ET.SubElement(x.find('subTags'), 'item', type='ShowFrameTag', forceWriteAsLong='false')
            x.set('frameCount', '1')
    tags = r.find('tags'); tags.clear(); tags.extend(definitions)
    for name, sid, count in POSES:
        ET.SubElement(tags, 'item', type='RemoveObject2Tag', depth='1', forceWriteAsLong='false')
        place = ET.SubElement(tags, 'item', type='PlaceObject2Tag', characterId=str(sid), depth='1', forceWriteAsLong='false', placeFlagHasCharacter='true', placeFlagHasMatrix='true', placeFlagMove='false', placeFlagHasClipActions='false', placeFlagHasClipDepth='false', placeFlagHasColorTransform='false', placeFlagHasName='false', placeFlagHasRatio='false', ratio='0', clipDepth='0')
        ET.SubElement(place, 'matrix', type='MATRIX', hasRotate='false', hasScale='false', translateX='0', translateY='0', rotateSkew0='0.0', rotateSkew1='0.0', scaleX='1.0', scaleY='1.0', nTranslateBits='1', nRotateBits='0', nScaleBits='0')
        for _ in range(count):
            ET.SubElement(tags, 'item', type='ShowFrameTag', forceWriteAsLong='false')
    r.set('frameCount', str(sum(p[2] for p in POSES)))
    rect = r.find('displayRect')
    for k, v in dict(Xmin=-3840, Xmax=3840, Ymin=-3520, Ymax=1280).items(): rect.set(k, str(v))
    ET.ElementTree(r).write(ROOT/'assets/extracted/thief.xml', encoding='utf-8', xml_declaration=True)

def pack(block):
    result = bytearray(); i = 0
    while i < len(block):
        run = 1
        while run < 128 and i+run < len(block) and block[i+run] == block[i]: run += 1
        if run >= 3:
            result.extend([127+run, block[i]]); i += run
        else:
            start = i; i += run
            while i < len(block) and i-start < 128:
                if i+2 < len(block) and block[i] == block[i+1] == block[i+2]: break
                i += 1
            result.append(i-start-1); result.extend(block[start:i])
    return result

def unpack(data, offset, size):
    out = bytearray()
    while len(out) < size:
        token = data[offset]; offset += 1; n = (token & 127)+1
        if token & 128: out.extend([data[offset]]*n); offset += 1
        else: out.extend(data[offset:offset+n]); offset += n
    assert len(out) == size
    return bytes(out)

def array(name, typ, values):
    return f'static const {typ} {name}[{len(values)}] = {{\n'+''.join(','.join(map(str, values[i:i+24]))+',\n' for i in range(0, len(values), 24))+'};\n'

def convert():
    count = sum(p[2] for p in POSES)
    # Campaign enemies use the light-combo AI. Preserve the complete pose-index
    # interface, but hold every third authored frame to bound the chapter bank.
    # Unsupported enemy-only actions fall back to a related retained pose.
    retained = {'idle', 'walk', 'attack', 'jump', 'block', 'hurt', 'light2',
                'light3', 'finisher', 'knockdown', 'land', 'dead', 'airhurt',
                'roll', 'airspin', 'bow'}
    fallback = {'heavy':'attack', 'uppercut':'light3', 'cast':'idle',
                'splash':'idle', 'airattack':'attack', 'throw':'attack',
                'clash':'block', 'heavy2':'light2', 'bow':'idle',
                'airattack2':'light2', 'airheavy':'airhurt', 'castdown':'jump'}
    starts = {}; offset = 0
    for name, sid, length in POSES:
        starts[name] = (offset, length); offset += length
    source_map = []
    for name, sid, length in POSES:
        target = name if name in retained else fallback[name]
        start, target_count = starts[target]
        stride = 4 if target == 'idle' else 3
        for frame in range(length):
            selected = min(frame, target_count-1)
            # Keep authored terminal poses, especially dead/hurt/landing holds.
            if selected != target_count-1:
                selected = selected//stride*stride
            source_map.append(start+selected)
    frames = []
    for selected in source_map:
        im = Image.open(ROOT/f'assets/thief/{selected+1}.png').convert('RGBA')
        assert im.size == (384, 240)
        frames.append(im.resize((192, 120), Image.Resampling.LANCZOS))
    atlas = Image.new('RGB', (192, 120*count))
    for n, im in enumerate(frames): atlas.paste(im, (0, n*120), im.getchannel('A'))
    palette = atlas.quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    pal = palette.getpalette()[:765]
    ds_pal = [0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
    data = bytearray(); meta = []; unique = {}; hashes = []; preview = Image.new('RGB', (192*8, 120*((count+7)//8)), (56,53,46))
    for n, im in enumerate(frames):
        q = im.convert('RGB').quantize(palette=palette, dither=Image.Dither.NONE)
        alpha = im.getchannel('A').point(lambda a: 255 if a >= 100 else 0)
        x,y,r,b = alpha.getbbox()
        assert x > 0 and y > 0 and r < 192 and b < 120, ('clipped', n, (x,y,r,b))
        block = bytes(q.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0 for yy in range(y,b) for xx in range(x,r))
        key = (r-x,b-y,block)
        if key not in unique:
            unique[key] = len(data); data.extend(pack(block))
        meta.append((unique[key],x-96,y-88,r-x,b-y))
        hashes.append(hashlib.sha256(block).hexdigest())
        rgba = Image.new('RGBA', im.size)
        for yy in range(y,b):
            for xx in range(x,r):
                c = ds_pal[q.getpixel((xx,yy))+1]
                rgba.putpixel((xx,yy),((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31,alpha.getpixel((xx,yy))))
        preview.paste(rgba,(n%8*192,n//8*120),rgba)
    for i, (offset,x,y,w,h) in enumerate(meta):
        assert hashlib.sha256(unpack(data,offset,w*h)).hexdigest() == hashes[i]
    out = ROOT/'generated'
    h = '#ifndef THIEF_ART_H\n#define THIEF_ART_H\n#include "castle_art.h"\n'
    h += f'#define THIEF_FRAME_PIXELS_MAX {max(m[3]*m[4] for m in meta)}\n#define THIEF_FRAME_COUNT {count}\n'
    h += array('thief_palette','uint16_t',ds_pal)+array('thief_pixels','uint8_t',list(data))
    h += 'static const ArtFrame thief_frames[] = {\n'+''.join('{'+','.join(map(str,m))+'},\n' for m in meta)+'};\n'
    h += '''static inline int thief_decode_frame(unsigned id,uint8_t*out){
 if(id>=THIEF_FRAME_COUNT||!out)return 0;
 const ArtFrame*f=&thief_frames[id];unsigned at=f->offset,left=(unsigned)f->w*f->h;
 while(left){
  if(at>=sizeof(thief_pixels))return 0;
  unsigned token=thief_pixels[at++],n=(token&127)+1;if(n>left)return 0;
  if(token&128){if(at>=sizeof(thief_pixels))return 0;unsigned v=thief_pixels[at++];for(unsigned j=0;j<n;j++)*out++=(uint8_t)v;}
  else {if(n>sizeof(thief_pixels)-at)return 0;for(unsigned j=0;j<n;j++)*out++=thief_pixels[at++];}
  left-=n;
 }return 1;
}
#endif
'''
    (out/'thief_art.h').write_text(h)
    preview.save(out/'thief-sheet.png')
    audit = dict(source='Installed PLAYER.swf; MAIN f_SpawnThief selects helmet/emblem/shield 8 and weapon 10 among four variants', costume=8, weapon=10, frame_slots=count, retained_authored_frames=len(set(source_map)), pose_timing='Unchanged. Every third frame held; idle holds every fourth. Unsupported AI actions alias related retained poses.', fallback_poses=fallback, source_frame_map=source_map, poses=POSES, pixel_bytes=len(data), max_frame_pixels=max(m[3]*m[4] for m in meta), verified_frame_slots=len(hashes), source_sha256=hashlib.sha256((ROOT/'assets/extracted/PLAYER.swf').read_bytes()).hexdigest())
    (out/'thief-audit.json').write_text(json.dumps(audit,indent=2))
    print(json.dumps(audit))


if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--prepare',action='store_true')
    if parser.parse_args().prepare:prepare()
    else:convert()

if __name__ == "__main__" and "--prepare" not in __import__("sys").argv:
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
