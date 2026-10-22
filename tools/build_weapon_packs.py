"""Build per-weapon art packs (held patches, heavy/dash combo layers, icon) from the factored source pickles.

Resident: Huffman trees, weapon-independent combo/dash layers and frame bounds (generated/weapon_art_shared.h).
Streamed: nitrofiles/weapons/w<slot>.bin (one per weapon slot) and icons.bin. Adding a weapon means adding it to the
three factor scripts' weapon lists and re-running this script; every pack is rebuilt against the shared trees.
"""
from pathlib import Path
import pickle,struct,collections,heapq,json,zlib,sys
from PIL import Image
from pack_weapon_icons import pack as rle_pack
R=Path(__file__).resolve().parents[1]
OUT=R/'nitrofiles/weapons';OUT.mkdir(parents=True,exist_ok=True)

def seq_of(stream):
    p=0;seq=[]
    while p<len(stream):
        while True:
            v=stream[p];p+=1;seq.append((0,v))
            if v<128:break
        n=stream[p];p+=1;seq.append((0,n))
        for v in stream[p:p+n]:seq.append((1,v))
        p+=n
    return seq
def train(seqs):
    counts=[collections.Counter(),collections.Counter()]
    for seq in seqs:
        for c,v in seq:counts[c][v]+=1
    trees=[];tables=[];codes=[]
    for count in counts:
        heap=[(v,k,k) for k,v in count.items()]
        if len(heap)==1:heap.append((0,max(count)+1,max(count)+1))
        heapq.heapify(heap);serial=256
        while len(heap)>1:
            a=heapq.heappop(heap);b=heapq.heappop(heap);heapq.heappush(heap,(a[0]+b[0],serial,(a[2],b[2])));serial+=1
        nodes=[];code={}
        def walk(tree,bits,n):
            if isinstance(tree,int):code[tree]=(bits,n);return -tree-1
            index=len(nodes);nodes.append(None);a=walk(tree[0],bits<<1,n+1);b=walk(tree[1],bits<<1|1,n+1);nodes[index]=(a,b);return index
        walk(heap[0][2],0,0);table=[]
        for v in range(256):
            node=0;used=0
            while node>=0 and used<8:node=nodes[node][(v>>(7-used))&1];used+=1
            table.append(32768|(used<<8)|(-node-1) if node<0 else node)
        trees.append(nodes);codes.append(code);tables.append(table)
    return trees,tables,codes
def encode(seq,codes):
    out=bytearray();bits=0;n=0
    for c,v in seq:
        value,size=codes[c][v];bits=(bits<<size)|value;n+=size
        while n>=8:n-=8;out.append((bits>>n)&255)
        bits&=(1<<n)-1
    if n:out.append((bits<<(8-n))&255)
    return bytes(out)
class Table:
    """A set of streams encoded with shared codes; ids are local to the table."""
    def __init__(s,codes):s.codes=codes;s.ids={b'':0};s.streams=[b'']
    def add(s,stream):
        if stream not in s.ids:s.ids[stream]=len(s.streams);s.streams.append(stream)
        return s.ids[stream]
    def blob(s):
        blob=bytearray();offs=[0];sizes=[]
        for st in s.streams:
            seq=seq_of(st);blob.extend(encode(seq,s.codes));offs.append(len(blob));sizes.append(len(seq))
        return bytes(blob),offs,sizes
def u16(v):return struct.pack('<%dH'%len(v),*v)
def u32(v):return struct.pack('<%dI'%len(v),*v)
def align4(b):return b+bytes((-len(b))%4)
def table_bytes(t):
    blob,offs,sizes=t.blob()
    return u32([len(t.streams),len(blob)])+u32(offs)+align4(u16(sizes))+align4(blob)
def arr(n,t,v):return f'static const {t} {n}[{len(v)}]={{'+','.join(map(str,v))+'};\n'

# ---- source data
hs,hcommon,hfix,hbounds,hmax=pickle.loads((R/'build/held-factored.pkl').read_bytes())
cs,cbase,ccolor,cweap,cfix,cbounds,csamples=pickle.loads((R/'build/default-combo-factored.pkl').read_bytes())
ds,dbase,dcolor,dweap,dfix,dbounds,dsamples=pickle.loads((R/'build/dash-attack-factored.pkl').read_bytes())
NW=len(cweap);NHELD=len(hcommon);FIRST_HELD=NW-NHELD  # held patches exist for the last NHELD slots
assert FIRST_HELD==2 and len(dweap)==NW

# ---- shared Huffman trees per system, trained over every weapon's streams
held_used={hs[i] for lst in hcommon for i in lst}|{hs[i] for c in hfix for lst in c for i in lst}
def used(streams,base,color,weap,fix):
    ids=set()
    for lst in weap:ids.update(lst)
    for c in fix:
        for w in c:ids.update(w)
    ids.update(base)
    for c in color:ids.update(c)
    return [streams[i] for i in sorted(ids)]
trees={};tables={};codes={}
trees['held'],tables['held'],codes['held']=train([seq_of(s) for s in held_used])
trees['combo'],tables['combo'],codes['combo']=train([seq_of(s) for s in used(cs,cbase,ccolor,cweap,cfix)])
trees['dash'],tables['dash'],codes['dash']=train([seq_of(s) for s in used(ds,dbase,dcolor,dweap,dfix)])
tree_hash=zlib.crc32(repr((trees,tables)).encode())&0xffffffff

# ---- shared (weapon independent) combo/dash layers
def shared_layers(name,streams,base,color):
    t=Table(codes[name]);b=[t.add(streams[i]) for i in base];c=[[t.add(streams[i]) for i in row] for row in color]
    blob,offs,sizes=t.blob()
    return t,b,c,blob,offs,sizes
sc=shared_layers('combo',cs,cbase,ccolor);sd=shared_layers('dash',ds,dbase,dcolor)

# ---- one pack per weapon slot
pack_max=0;sizes_report={}
for w in range(NW):
    sections=[]
    if w>=FIRST_HELD:
        s=w-FIRST_HELD;t=Table(codes['held'])
        common=[t.add(hs[i]) for i in hcommon[s]];fix=[[t.add(hs[i]) for i in hfix[c][s]] for c in range(4)]
        bounds=bytes(v&255 for b in hbounds[s] for v in b)
        sec=u16(common)+u16([v for row in fix for v in row])+bounds+table_bytes(t)
        sections.append(sec)
    else:sections.append(b'')
    for name,(streams,weap,fix) in (('combo',(cs,cweap,cfix)),('dash',(ds,dweap,dfix))):
        t=Table(codes[name]);wl=[t.add(streams[i]) for i in weap[w]];fl=[[t.add(streams[i]) for i in fix[c][w]] for c in range(4)]
        sections.append(align4(u16(wl))+align4(u16([v for row in fl for v in row]))+table_bytes(t))
    head=struct.pack('<4sIII',b'WPK2',tree_hash,w,0)
    offs=[];body=bytearray()
    base=len(head)+4*6
    for sec in sections:offs.append(base+len(body));body.extend(align4(sec))
    lens=[len(align4(sec)) for sec in sections]
    data=head+u32(offs)+u32(lens)+bytes(body)
    (OUT/f'w{w}.bin').write_bytes(data);pack_max=max(pack_max,len(data));sizes_report[w]=len(data)

# ---- icons: self-contained palette + RLE per record
recs=[]
for i in range(1,NW+1):
    im=Image.open(R/f'assets/campaign-weapons/{i}.png').convert('RGBA');box=im.getchannel('A').getbbox();im=im.crop(box);im.thumbnail((30,30),Image.Resampling.LANCZOS)
    icon=Image.new('RGBA',(32,32));icon.paste(im,((32-im.width)//2,(32-im.height)//2))
    q=icon.convert('RGB').quantize(colors=31,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE);p=q.getpalette()[:93]
    pal=[0]+[0x8000|(p[k]>>3)|((p[k+1]>>3)<<5)|((p[k+2]>>3)<<10) for k in range(0,93,3)]
    px=bytes((q.getpixel((x,y))+1 if icon.getpixel((x,y))[3]>=64 else 0) for y in range(32) for x in range(32))
    recs.append(u16(pal)+bytes(rle_pack(px)))
head=struct.pack('<4sI',b'WIC1',len(recs));off=len(head)+4*(len(recs)+1);offs=[]
for r in recs:offs.append(off);off+=len(r)
offs.append(off)
(OUT/'icons.bin').write_bytes(head+u32(offs)+b''.join(recs))

# ---- resident header
h='/* Generated by build_weapon_packs.py. Weapon-independent art tables; per-weapon data streams from nitrofiles/weapons/. */\n#ifndef WEAPON_ART_SHARED_H\n#define WEAPON_ART_SHARED_H\n#include <stdint.h>\n#include "castle_art.h"\n'
h+=f'#define WEAPON_PACK_COUNT {NW}\n#define WEAPON_PACK_FIRST_HELD {FIRST_HELD}\n#define WEAPON_PACK_MAX_BYTES {(pack_max+15)//16*16}\n#define WEAPON_PACK_TREE_HASH 0x{tree_hash:08x}u\n#define WEAPON_ICON_COUNT {NW}\n'
h+=f'#define KNIGHT_WEAPON_FRAME_PIXELS_MAX {hmax}\n#define DEFAULT_COMBO_PIXELS_MAX {max(w*hh for x,y,w,hh in cbounds)}\n#define DASH_ATTACK_PIXELS_MAX {max(w*hh for x,y,w,hh in dbounds)}\n'
h+=f'#define DEFAULT_COMBO_SAMPLES {len(csamples)}\n#define DASH_ATTACK_SAMPLES {len(dsamples)}\n'
for name in ('held','combo','dash'):
    for i in range(2):
        h+=f'static const int16_t {name}_tree{i}[][2]={{'+','.join('{%d,%d}'%v for v in trees[name][i])+'};\n'+arr(f'{name}_fast{i}','uint16_t',tables[name][i])
for name,(t,b,c,blob,offs,sizes),bounds in (('combo',sc,cbounds),('dash',sd,dbounds)):
    h+=arr(f'{name}_shared_data','uint8_t',blob)+arr(f'{name}_shared_offsets','uint32_t',offs)+arr(f'{name}_shared_sizes','uint16_t',sizes)
    h+=arr(f'{name}_base','uint16_t',b)+f'static const uint16_t {name}_color[4][{len(b)}]={{'+','.join('{'+','.join(map(str,r))+'}' for r in c)+'};\n'
    h+=f'static const ArtFrame {name}_frames[{len(b)}]={{'+','.join('{0,%d,%d,%d,%d}'%v for v in bounds)+'};\n'
h+='#endif\n'
(R/'generated/weapon_art_shared.h').write_text(h)
print('packs',sizes_report,'max',pack_max,'shared combo',len(sc[3]),'dash',len(sd[3]),'hash',hex(tree_hash))
