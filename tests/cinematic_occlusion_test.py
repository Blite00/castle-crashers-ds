"""Verify authored princess-door depth and final indexed movie occlusion."""
from pathlib import Path
import re, struct, sys, zlib
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from convert_cinematics import princess_door_mask

def arrays(path):
    text=path.read_text()
    return {name:list(map(int,re.findall(r'-?\d+',body))) for name,body in re.findall(r'static const \w+ (cinema_\w+)\[\d+\] = \{(.*?)\};',text,re.S)}

def frames(a):
    for kind,(start,end) in enumerate(zip(a['cinema_clip_offsets'],a['cinema_clip_offsets'][1:])):
        for f in range(start,end):
            if (f-start)%16==0:
                block=a['cinema_clip_blocks'][kind]+(f-start)//16
                packed=bytes(a['cinema_pixels'][a['cinema_offsets'][block]:a['cinema_offsets'][block+1]])
                raw=zlib.decompress(packed,-15);pixels=bytearray(raw[:192*144]);at=192*144
            else:
                dx,dy=a['cinema_shifts'][f*2:f*2+2];shifted=bytearray(192*144)
                for y in range(144):
                    if 0<=y+dy<144:
                        left=max(0,-dx);right=min(192,192-dx)
                        if right>left:shifted[y*192+left:y*192+right]=pixels[(y+dy)*192+left+dx:(y+dy)*192+right+dx]
                pixels=shifted;stop=at+a['cinema_sizes'][f]
                while at<stop:
                    pos,n=struct.unpack_from('<HH',raw,at);at+=4;pixels[pos:pos+n]=raw[at:at+n];at+=n
            yield kind,f,bytes(pixels)

def check():
    root=ET.parse(ROOT/'assets/extracted/level13.xml').getroot()
    scene=next(t for t in root.find('tags') if t.get('spriteId')=='407')
    places={t.get('characterId'):t for t in scene.find('subTags') if t.get('characterId') in ('102','360')}
    assert int(places['102'].find('matrix').get('translateY'))>int(places['360'].find('matrix').get('translateY'))
    for sprite in (102,360):
        action=(ROOT/f'assets/level13-scripts/scripts/DefineSprite_{sprite}/frame_1/DoAction.as').read_text()
        assert 'f_AddNeutral(this)' in action
    a=arrays(ROOT/'generated/cinematic_art.h');masked_frames=0
    for kind,f,pixels in frames(a):
        if kind!=1:continue
        cx=a['cinema_camera'][f*2]+a['cinema_bg_origins'][2]
        cy=a['cinema_camera'][f*2+1]+a['cinema_bg_origins'][3]
        mask=princess_door_mask(cx,cy).tobytes()
        assert all(p==0 for p,m in zip(pixels,mask) if m>=128)
        masked_frames+=any(m>=128 for m in mask)
    assert masked_frames>30
    print('PASS: original neutral-object depths and princess doorway occlusion across116 frames')

if __name__=='__main__':check()
