"""Convert locally exported game artwork to transparent 8-bit DS palettes."""
from pathlib import Path
from PIL import Image, ImageDraw
import json,hashlib
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'generated'
OUT.mkdir(exist_ok=True)
poses=json.loads((ROOT/'assets/poses.json').read_text())
frames=[]
animation_audit=[]
for pose in poses:
    hashes=[]
    for n in range(pose['start'],pose['start']+pose['count']):
        im=Image.open(ROOT/f"assets/{pose['color']}/{n}.png").convert('RGBA')
        assert im.size==(384,240),im.size
        frames.append(im.resize((192,120),Image.Resampling.LANCZOS))
        hashes.append(hashlib.sha256(im.tobytes()).hexdigest())
    animation_audit.append(dict(color=pose['color'],pose=pose['pose'],frames=len(hashes),unique_frames=len(set(hashes))))
atlas=Image.new('RGB',(192,120*len(frames)),(0,0,0))
for n,im in enumerate(frames):atlas.paste(im,(0,n*120),im.getchannel('A'))
palette=atlas.quantize(colors=255,method=Image.Quantize.MEDIANCUT)
pal=palette.getpalette()[:765]
ds_pal=[0]+[0x8000|(pal[i]>>3)|((pal[i+1]>>3)<<5)|((pal[i+2]>>3)<<10) for i in range(0,765,3)]
def pack(block):
    out=[];i=0
    while i<len(block):
        run=1
        while run<128 and i+run<len(block) and block[i+run]==block[i]:run+=1
        if run>=3:out.extend([127+run,block[i]]);i+=run
        else:
            start=i;i+=run
            while i<len(block) and i-start<128:
                if i+2<len(block) and block[i]==block[i+1]==block[i+2]:break
                i+=1
            # A short run near the boundary never exceeds128.
            out.append(i-start-1);out.extend(block[start:i])
    return out

def unpack(data,offset,size):
    out=[]
    while len(out)<size:
        token=data[offset];offset+=1;n=(token&127)+1
        if token&128:out.extend([data[offset]]*n);offset+=1
        else:out.extend(data[offset:offset+n]);offset+=n
    assert len(out)==size
    return bytes(out)

# BEGIN KNIGHT CODEC
# Independent frames, entropy-coded RLE; two alphabets avoid mixing run controls
# with palette indices. Lookup tables handle the first eight code bits.
def knight_huffman_encode(data, meta):
    import collections,heapq
    freq=[collections.Counter(),collections.Counter()];i=0
    while i<len(data):
        t=data[i];freq[0][t]+=1;i+=1;n=1 if t&128 else t+1
        freq[1].update(data[i:i+n]);i+=n
    codes=[];tables=[];trees=[]
    for counts in freq:
        heap=[(n,s,s) for s,n in counts.items()];heapq.heapify(heap);serial=256
        while len(heap)>1:
            a,_,aa=heapq.heappop(heap);b,_,bb=heapq.heappop(heap)
            heapq.heappush(heap,(a+b,serial,(aa,bb)));serial+=1
        tree=[];code={}
        def visit(node,bits=0,length=0):
            if isinstance(node,int):code[node]=(bits,length);return -node-1
            index=len(tree);tree.append(None)
            left=visit(node[0],bits<<1,length+1);right=visit(node[1],(bits<<1)|1,length+1)
            tree[index]=(left,right);return index
        visit(heap[0][2]);table=[]
        for value in range(256):
            node=0;used=0
            while node>=0 and used<8:node=tree[node][(value>>(7-used))&1];used+=1
            table.append((0x8000|(used<<8)|(-node-1)) if node<0 else node)
        codes.append(code);tables.append(table);trees.append(tree)
    packed=[];offsets={};lengths={}
    for old,_,_,w,h in meta:
        if old in offsets:continue
        stream=[];bits=0;count=0;i=old;remaining=w*h
        def emit(symbol,context):
            nonlocal bits,count
            value,n=codes[context][symbol];bits=(bits<<n)|value;count+=n
            while count>=8:count-=8;stream.append((bits>>count)&255)
            bits&=(1<<count)-1
        while remaining:
            t=data[i];i+=1;emit(t,0);n=(t&127)+1;remaining-=n
            for _ in range(1 if t&128 else n):emit(data[i],1);i+=1
        if count:stream.append((bits<<(8-count))&255)
        offsets[old]=len(packed);lengths[old]=len(stream);packed.extend(stream)
    return packed,[(offsets[o],x,y,w,h) for o,x,y,w,h in meta],[lengths[o] for o,*_ in meta],tables,trees

def knight_color_delta(data,meta,frames_per_color):
    encoded=[];remapped=[];references=[];seen={};canonical={}
    for fid,(old,x,y,w,h) in enumerate(meta):
        pixels=unpack(data,old,w*h);packed=pack(pixels);base=65535
        if fid>=frames_per_color:
            bf=fid%frames_per_color;bf=canonical.setdefault(tuple(meta[bf]),bf);bo,bx,by,bw,bh=meta[bf]
            original=unpack(data,bo,bw*bh);mapped=bytearray(w*h)
            left=max(x,bx);right=min(x+w,bx+bw)
            if left<right:
                for yy in range(max(y,by),min(y+h,by+bh)):
                    start=(yy-y)*w+left-x;source=(yy-by)*bw+left-bx
                    mapped[start:start+right-left]=original[source:source+right-left]
            delta=bytes(a^b for a,b in zip(pixels,mapped));candidate=pack(delta)
            if len(candidate)<len(packed):packed=candidate;base=bf
        key=(base,bytes(packed))
        if key not in seen:seen[key]=len(encoded);encoded.extend(packed)
        remapped.append((seen[key],x,y,w,h));references.append(base)
    return encoded,remapped,references

KNIGHT_DECODER=r"""
/* Returns zero before any write beyond the authored frame bounds. */
static inline int knight_huff_symbol(const uint8_t **src,const uint8_t *end,
    uint32_t *buffer,unsigned *bits,unsigned context){
    while(*bits<8&&*src<end){*buffer=(*buffer<<8)|*(*src)++;*bits+=8;}
    unsigned peek=*bits>=8?(*buffer>>(*bits-8))&255:(*buffer<<(8-*bits))&255;
    unsigned entry=knight_huff_table[context][peek];
    if(entry&32768){unsigned n=(entry>>8)&31;if(*bits<n)return -1;*bits-=n;return entry&255;}
    if(*bits<8)return -1;
    *bits-=8;int node=(int)entry;
    for(unsigned depth=8;node>=0&&depth<24;depth++){
        if(!*bits){if(*src>=end)return -1;*buffer=*(*src)++;*bits=8;}
        node=knight_huff_tree[context][node][(*buffer>>--*bits)&1];
    }
    return node<0?-node-1:-1;
}
/* Decode a base through its source-aligned crop, or an XOR delta directly.
 * Color variants reference only original blue frames: no recursive chains. */
static inline int knight_decode_plane(unsigned id,uint8_t *out,const ArtFrame *target,int delta){
    const ArtFrame *frame=&knight_frames[id];
    const uint8_t *src=knight_pixels+frame->offset,*end=src+knight_frame_lengths[id];
    unsigned remaining=frame->w*frame->h,bits=0,position=0;uint32_t buffer=0;
    int x=0,y=0,dx=target?frame->x-target->x:0,dy=target?frame->y-target->y:0;
    if(remaining>KNIGHT_FRAME_PIXELS_MAX||frame->offset+knight_frame_lengths[id]>sizeof(knight_pixels))return 0;
    while(remaining){
        int token=knight_huff_symbol(&src,end,&buffer,&bits,0);if(token<0)return 0;
        unsigned n=(token&127)+1;if(n>remaining)return 0;remaining-=n;
        int repeat=token&128,value=repeat?knight_huff_symbol(&src,end,&buffer,&bits,1):0;
        if(value<0)return 0;
        if(!target&&repeat){
            if(!delta)memset(out+position,value,n);
            else if(value)for(unsigned k=0;k<n;k++)out[position+k]^=(uint8_t)value;
            position+=n;continue;
        }
        while(n--){
            if(!repeat){value=knight_huff_symbol(&src,end,&buffer,&bits,1);if(value<0)return 0;}
            if(target){
                if(x+dx>=0&&x+dx<target->w&&y+dy>=0&&y+dy<target->h)out[(y+dy)*target->w+x+dx]=(uint8_t)value;
                if(++x==frame->w){x=0;y++;}
            }else if(delta)out[position++]^=(uint8_t)value;
            else out[position++]=(uint8_t)value;
        }
    }
    return 1;
}
static inline int knight_decode_frame(unsigned id,uint8_t *out){
    if(id>=sizeof(knight_frames)/sizeof(knight_frames[0])||!out)return 0;
    unsigned base=knight_frame_bases[id];const ArtFrame *frame=&knight_frames[id];
    if(frame->w*frame->h>KNIGHT_FRAME_PIXELS_MAX)return 0;
    if(base==65535)return knight_decode_plane(id,out,0,0);
    if(base>=sizeof(knight_frames)/sizeof(knight_frames[0])||knight_frame_bases[base]!=65535)return 0;
    const ArtFrame *original=&knight_frames[base];
    if(original->x==frame->x&&original->y==frame->y&&original->w==frame->w&&original->h==frame->h){
        if(!knight_decode_plane(base,out,0,0))return 0;
    }else{
        for(unsigned i=0;i<(unsigned)frame->w*frame->h;i++)out[i]=0;
        if(!knight_decode_plane(base,out,frame,0))return 0;
    }
    return knight_decode_plane(id,out,0,1);
}
"""
def knight_codec_tables(array,lengths,tables,trees):
    text=array('knight_frame_lengths','uint16_t',lengths)
    text+='static const uint16_t knight_huff_table[2][256]={\n'+''.join('{'+','.join(map(str,t))+'},\n' for t in tables)+'};\n'
    text+='static const int16_t knight_huff_tree[2][255][2]={\n'+''.join('{'+','.join('{%d,%d}'%pair for pair in tree)+'},\n' for tree in trees)+'};\n'
    return text
# END KNIGHT CODEC

data=[];meta=[];unique_pixels={};uncached_bytes=0;frame_pixel_hashes=[];preview=Image.new('RGB',(192*9,120*((len(frames)+8)//9)),(56,53,46))
for n,im in enumerate(frames):
    q=im.convert('RGB').quantize(palette=palette,dither=Image.Dither.NONE)
    alpha=im.getchannel('A').point(lambda a:255 if a>=100 else 0)
    box=alpha.getbbox();assert box
    x,y,r,b=box
    assert x>0 and y>0 and r<192 and b<120,('clipped sprite',n,box)
    block=bytes(q.getpixel((xx,yy))+1 if alpha.getpixel((xx,yy)) else 0
                for yy in range(y,b) for xx in range(x,r))
    frame_pixel_hashes.append(hashlib.sha256(block).hexdigest())
    uncached_bytes+=len(block)
    # Identical authored hold frames share storage; timing/metadata stay intact.
    key=(r-x,b-y,block)
    offset=unique_pixels.get(key)
    if offset is None:
        offset=len(data);unique_pixels[key]=offset;data.extend(pack(block))
    meta.append((offset,x-96,y-88,r-x,b-y))
    # Preview the exact RGB555 colors that the DS will display.
    converted=Image.new('RGBA',im.size)
    for yy in range(120):
        for xx in range(192):
            c=ds_pal[q.getpixel((xx,yy))+1]
            converted.putpixel((xx,yy),((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31,alpha.getpixel((xx,yy))))
    preview.paste(converted,(n%9*192,n//9*120),converted)
preview.save(OUT/'knight-sheet.png')
for frame,(offset,x,y,w,h) in enumerate(meta):
    assert hashlib.sha256(unpack(data,offset,w*h)).hexdigest()==frame_pixel_hashes[frame],('dedup mismatch',frame)
def array(name,typ,values):
    return f'static const {typ} {name}[{len(values)}] = {{\n'+''.join(','.join(map(str,values[i:i+24]))+',\n' for i in range(0,len(values),24))+'};\n'
data,meta,bases=knight_color_delta(data,meta,sum(p["count"] for p in poses if p["color"]=="blue"))
data,meta,lengths,tables,trees=knight_huffman_encode(data,meta)
h='#ifndef CASTLE_ART_H\n#define CASTLE_ART_H\n#include <stdint.h>\n#include <string.h>\n'
h+='typedef struct {uint32_t offset; int16_t x,y; uint16_t w,h;} ArtFrame;\n'
h+=f'#define KNIGHT_FRAME_PIXELS_MAX {max(m[3]*m[4] for m in meta)}\n#define KNIGHT_COLOR_COUNT 4\n'
h+=array('knight_palette','uint16_t',ds_pal)+array('knight_pixels','uint8_t',data)
h+='static const ArtFrame knight_frames[] = {\n'+''.join('{'+','.join(map(str,m))+'},\n' for m in meta)+'};\n'
h+=knight_codec_tables(array,lengths,tables,trees)+array("knight_frame_bases","uint16_t",bases)+KNIGHT_DECODER
first_color=[p for p in poses if p['color']=='blue']
h+=array('pose_start','uint16_t',[p['start']-1 for p in first_color])+array('pose_count','uint16_t',[p['count'] for p in first_color])
h+=f'#define KNIGHT_FRAMES_PER_COLOR {sum(p["count"] for p in first_color)}\n#endif\n'
(OUT/'castle_art.h').write_text(h)
audit={'source':'Installed Castle Crashers data/game/player.pak','source_sha256':hashlib.sha256((ROOT/'assets/extracted/PLAYER.swf').read_bytes()).hexdigest(),'frames':len(frames),'colors':256,'format':'Independent two-context Huffman RLE8 with one-layer blue-frame color deltas; exact indexed pixels, RGB555 palette, index0 transparent','frame_canvas':[192,120],'foot_origin':[96,88],'pixel_bytes':len(data),'unshared_pixel_bytes':uncached_bytes,'deduplicated_pixel_bytes':uncached_bytes-len(data),'dedup_verified_frames':len(frame_pixel_hashes),'palette_bytes':512,'metadata_bytes':len(meta)*12,'poses':poses,'animation_audit':animation_audit}
audit['original_motion']={'source':'MAIN.swf, frame2 DoAction, f_ResetPlayerInfoAndStats / f_Jumping',
 'source_frame_rate':30,'speed_launch':-20,'gravity':2,'terminal_fall_speed':32,
 'half_resolution_launch':10,'half_resolution_gravity':1,'half_resolution_terminal':16,
 'jump_stop':'PLAYER sprite356 frame12 stop()',
 'land_end':'PLAYER sprite396 frame6 fp_StandAnim',
 'knockdown_stop':'PLAYER sprite389 child388 frame6 stop()',
 'knockdown_recovery':'PLAYER sprite389 frame21 f_GetUp',
 'dead_hold':'PLAYER sprite394 frame2 stops when vs_fight',
 'airhurt':'PLAYER root juggle1 label, sprite379 one frame',
 'throw':'PLAYER root throw label, sprite839 seventeen frames; swing at6, end_attack at17',
 'throw_release':'PLAYER victim sprite841 frame7 calls f_Thrown (zero-based event6)',
 'roll':'PLAYER roll/rollaway labels use sprite1675, eight frames. Reused for adapted recovery; rollaway script itself is offscreen respawn.',
 'spin':'PLAYER topspin sprite700 frames1-13, hits at3 and9; frame13 clears spinning and returns to jump. Later frames are alternate entries.',
 'spin_gate':'MAIN f_TopSpinInit has readable body_y < -75 height gate; unlock/input conditions are decompiler-damaged.',
 'clash':'PLAYER swordlock body sprite738 five frames; heavy thrust sprite584 frame6 installs f_HitReactionSwordLock.',
 'clash_mash':'MAIN f_SwordLockMash increments grapple_score with f_AnyButtonPressed; readable score>=10 and opponent+4 threshold, damaged surrounding operands.',
 'heavy2':'PLAYER punch2_2 sprite588 frames1-12; hit calls at2,6; punching false7; end_attack/stop12.',
 'bow':'PLAYER bow sprite795 twenty frames; shoots at11, default end20; agility-dependent windup/recovery skips exist in original.',
 'airattack2':'PLAYER punch11_2 sprite691 twelve frames, hit5, punchingfalse7, end12; original group11 alternates690/691.',
 'airheavy':'PLAYER slamdown sprite714 first14frames, hit5, punchingbusyfalse9, end14 (frame15stop); original aerialgroup12 thenexistingpunch2_2.',
 'castdown':'PLAYER magic_air_down sprite785 thirteen frames; fp_MagicMove frame4, end13; punch_pt(1143,-515)twips. OriginalBlueusesareaice; allknightdownbolt isDSadaptation.',
 'confidence':'Numeric constants and action statements readable; decompiler has malformed conditionals elsewhere, not used as behavioral evidence.'}
(OUT/'asset-audit.json').write_text(json.dumps(audit,indent=2))
print(json.dumps({k:v for k,v in audit.items() if k!='poses'},indent=2))

# Canonical hardware-color indices retain every displayed RGB555 pixel.
import subprocess as _codec_subprocess, sys as _codec_sys
_codec_subprocess.run([_codec_sys.executable,str(ROOT/'tools/canonicalize_knight_palette.py')],check=True)
