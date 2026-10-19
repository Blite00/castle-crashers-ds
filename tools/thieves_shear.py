"""Vertical shear for Thieves' Forest (LEVEL4).

The source corridor climbs about 500 source pixels over 13,000, which cannot fit the DS world's fixed 448-row height
together with the canopy above it. Every world column is therefore shifted vertically by dy(x) so the walkable band
sits near a constant height; art, depth layers, floor bounds and every source coordinate use the same table.
"""
from pathlib import Path
import struct,json,math
R=Path(__file__).resolve().parents[1]
ORIGIN=(0,-1350);SIZE=(6688,900);MID=290;MAX_SLOPE=0.45
b=(R/'assets/extracted/bsp4.bin').read_bytes();n=struct.unpack_from('<I',b,16)[0]//8
edges=[struct.unpack_from('<8f',b,20+i*32)[1:6] for i in range(n)]
FLOOR={1,2,3}
def band(wx):
    ys=[y1+(y2-y1)*(wx-x1)/(x2-x1) for x1,y1,x2,y2,k in edges if int(k) in FLOOR and x1!=x2 and min(x1,x2)<=wx<max(x1,x2)]
    return (min(ys),max(ys)) if len(ys)>=2 else None
def raw_floor():
    W=SIZE[0];cols=[]
    for x in range(W):
        v=band(x*2+ORIGIN[0]);cols.append(v)
    # Fill gaps shorter than 60 columns by interpolation (the source covers them with ramp edges); never beyond the corridor.
    i=0
    while i<W:
        if cols[i] is None:
            j=i
            while j<W and cols[j] is None:j+=1
            if i>0 and j<W and j-i<=60:
                a,c=cols[i-1],cols[j]
                for k in range(i,j):
                    t=(k-i+1)/(j-i+1);cols[k]=(a[0]+(c[0]-a[0])*t,a[1]+(c[1]-a[1])*t)
            i=j
        else:i+=1
    return cols
cols=raw_floor()
first=next(i for i,v in enumerate(cols) if v);last=max(i for i,v in enumerate(cols) if v)
mid=[None]*SIZE[0]
for x in range(SIZE[0]):
    v=cols[min(max(x,first),last)]
    mid[x]=((v[0]+v[1])/2-ORIGIN[1])/2
# Smooth with a 100-column box filter, then limit the slope so no column is sheared more than MAX_SLOPE rows from its neighbour.
def box(a,r):
    out=[];s=sum(a[:r+1]);cnt=r+1
    for i in range(len(a)):
        out.append(s/cnt)
        if i+r+1<len(a):s+=a[i+r+1];cnt+=1
        if i-r>=0:s-=a[i-r];cnt-=1
    return out
sm=box(mid,50)
d=[v-MID for v in sm]
for _ in range(3):
    for i in range(1,len(d)):d[i]=min(max(d[i],d[i-1]-MAX_SLOPE),d[i-1]+MAX_SLOPE)
    for i in range(len(d)-2,-1,-1):d[i]=min(max(d[i],d[i+1]-MAX_SLOPE),d[i+1]+MAX_SLOPE)
dy=[round(v) for v in d]
out=dict(origin=ORIGIN,size=SIZE,mid=MID,dy=dy,first=first,last=last)
(R/'assets/thieves-shear.json').write_text(json.dumps(out))
lo=min(dy);hi=max(dy);print('dy range',lo,hi,'first/last open column',first,last)
# Sheared floor band, for the report.
tops=[];bots=[]
for x in range(first,last+1):
    v=cols[x]
    if v:tops.append((v[0]-ORIGIN[1])/2-dy[x]);bots.append((v[1]-ORIGIN[1])/2-dy[x])
print('floor top range',round(min(tops)),round(max(tops)),'bottom range',round(min(bots)),round(max(bots)))
