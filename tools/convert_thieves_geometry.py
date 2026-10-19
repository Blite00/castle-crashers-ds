"""Thieves' Forest (LEVEL4) floor bounds, shear table and level points in DS world coordinates.

World = ((source x - 0) / 2, (source y + 1350) / 2 - dy(x)); dy comes from thieves_shear.py so art, floor and markers agree.
"""
from pathlib import Path
import struct,json,math
import xml.etree.ElementTree as E
R=Path(__file__).resolve().parents[1]
sh=json.loads((R/'assets/thieves-shear.json').read_text());dy=sh['dy'];OX,OY=sh['origin'];W,HR=sh['size'];H=448
b=(R/'assets/extracted/bsp4.bin').read_bytes();n=struct.unpack_from('<I',b,16)[0]//8
edges=[struct.unpack_from('<8f',b,20+i*32)[1:6] for i in range(n)]
def band(wx):
    ys=[y1+(y2-y1)*(wx-x1)/(x2-x1) for x1,y1,x2,y2,k in edges if int(k) in (1,2,3) and x1!=x2 and min(x1,x2)<=wx<max(x1,x2)]
    return (min(ys),max(ys)) if len(ys)>=2 else None
cols=[band(x*2+OX) for x in range(W)]
i=0
while i<W:  # same gap fill as the shear tool
    if cols[i] is None:
        j=i
        while j<W and cols[j] is None:j+=1
        if i>0 and j<W and j-i<=60:
            a,c=cols[i-1],cols[j]
            for k in range(i,j):
                t=(k-i+1)/(j-i+1);cols[k]=(a[0]+(c[0]-a[0])*t,a[1]+(c[1]-a[1])*t)
        i=j
    else:i+=1
valid=[]
for x,v in enumerate(cols):
    if v is None:valid.append(None);continue
    top=(v[0]-OY)/2-dy[x];bot=(v[1]-OY)/2-dy[x]
    valid.append((max(0,math.ceil(top)+5),min(H-1,math.floor(bot)-5)))
runs=[]
for x,v in enumerate(valid):
    if v is None or v[0]>v[1]:v=(448,0)
    if not runs or runs[-1][1:]!=v:runs.append((x,*v))
# Dy table as runs too.
druns=[]
for x,d in enumerate(dy):
    if not druns or druns[-1][1]!=d:druns.append((x,d))
def wx(sx):return sx/2
def wy(sx,sy):
    x=min(max(int(sx/2),0),W-1);return (sy-OY)/2-dy[x]
# waypoint table: floats after the edge records, (x,y,flag) triples
off=20+n*32;vals=struct.unpack_from(f'<{(len(b)-off)//4}f',b,off)
wps=[]
for k in range(0,len(vals)-2,3):
    x,y,f=vals[k:k+3]
    if x==0 and y==0:break
    wps.append((round(wx(x)),round(wy(x,y))))
# Named scene markers from sprite 393.
root=E.parse(R/'assets/extracted/level4-full.xml').getroot();ts=root.find('tags')
scene=next(t for t in ts if t.get('type')=='DefineSpriteTag' and t.get('spriteId')=='393')
marks={}
for t in scene.find('subTags'):
    if t.get('type','').startswith('PlaceObject') and t.get('name') and t.find('matrix') is not None:
        m=t.find('matrix');sx=int(m.get('translateX','0'))/20;sy=int(m.get('translateY','0'))/20
        marks[t.get('name')]=(round(wx(sx)),round(wy(sx,sy)),t.get('characterId'))
keep=['p1_start','p2_start','p3_start','p4_start','door1','door2','edge_left','edge_right','trollspawn1','trollspawn2','trollspawn3','trollspawn4','trollpet','weapon_pt','animal_pt','gold1','gold2','gold10','gold11','gold12','gold13','campfire','thiefwash','gate1','owl','bear','bearcam','deer','intro','bird_cp','abs_bottom']
h='#ifndef THIEVES_GEOMETRY_H\n#define THIEVES_GEOMETRY_H\n#include <stdint.h>\n/* LEVEL4 Thieves Forest, half scale, columns sheared by thieves_shear.py; 5px foot margin. */\n'
h+=f'#define THIEVES_WIDTH {W}\n'
h+=f'static const uint16_t thieves_floor_runs[{len(runs)}][3]={{\n'+''.join('{%d,%d,%d},'%r+('\n' if k%8==7 else '') for k,r in enumerate(runs))+'\n};\n'
h+=f'static const int16_t thieves_shear_runs[{len(druns)}][2]={{\n'+''.join('{%d,%d},'%r+('\n' if k%8==7 else '') for k,r in enumerate(druns))+'\n};\n'
h+='''static inline unsigned thieves_floor_bound(unsigned x,unsigned bound){
 unsigned lo=0,hi=%d;
 if(x>=%d)return bound?0:448;
 while(lo+1<hi){unsigned mid=(lo+hi)/2;if(thieves_floor_runs[mid][0]<=x)lo=mid;else hi=mid;}
 return thieves_floor_runs[lo][bound?2:1];
}
#define thieves_floor_min(x) thieves_floor_bound((x),0)
#define thieves_floor_max(x) thieves_floor_bound((x),1)
'''%(len(runs),W)
h+=f'#define THIEVES_WAYPOINT_COUNT {len(wps)}\n'
h+=f'static const int16_t thieves_waypoints[{len(wps)}][2]={{'+','.join('{%d,%d}'%w for w in wps)+'};\n'
for k in keep:
    if k in marks:h+=f'#define THIEVES_{k.upper()}_X {marks[k][0]}\n#define THIEVES_{k.upper()}_Y {marks[k][1]}\n'
h+='#endif\n'
(R/'generated/thieves_geometry.h').write_text(h)
print('runs',len(runs),'dy runs',len(druns),'waypoints',wps)
print({k:marks[k] for k in keep if k in marks})
