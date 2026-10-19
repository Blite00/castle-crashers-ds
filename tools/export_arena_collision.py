from pathlib import Path
import struct,json,sys
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/vendor'))
import decrypt_asset as decrypt
source=Path(sys.argv[1]) if len(sys.argv)>1 else (Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'bsps'/'bsp49b.pak')
name,encrypted=decrypt.unzip_to_cok6_nrec(source); raw=decrypt.blowfish_decrypt(name,encrypted)
assert raw[:4]==b'GADP'
count=struct.unpack_from('<I',raw,16)[0]; assert count%8==0
edges=[]
for i in range(count//8):
    reserved,x1,y1,x2,y2,kind,left,right=struct.unpack_from('<8f',raw,20+i*32)
    assert reserved==0 and kind==2 and all(c==-1 or c%8==0 for c in (left,right))
    edges.append(dict(index=i,x1=326.95+x1/2,y1=307.5+y1/2,x2=326.95+x2/2,y2=307.5+y2/2,kind=int(kind),left=int(left)//8 if left>=0 else -1,right=int(right)//8 if right>=0 else -1))
groups={'exterior':[0,1,2,4,14],'center_rock':[6,8,10,16,24,26,27,28],
        'upper_right_rock':[3,5,7,9],'upper_left_rock':[11,12,13,17,21,22],
        'lower_right_rock':[15,23,25,29],'lower_left_rock':[18,19,20,30,31,32]}
polygons=[]
for name,indices in groups.items():
    remaining=[edges[i] for i in indices];edge=remaining.pop(0)
    vertices=[(edge['x1'],edge['y1']),(edge['x2'],edge['y2'])]
    while remaining:
        for e in remaining:
            a=(e['x1'],e['y1']);b=(e['x2'],e['y2'])
            if sum((v-w)**2 for v,w in zip(vertices[-1],a))<.001:
                vertices.append(b);remaining.remove(e);break
            if sum((v-w)**2 for v,w in zip(vertices[-1],b))<.001:
                vertices.append(a);remaining.remove(e);break
        else:raise ValueError('Unclosed original collision polygon: '+name)
    assert sum((v-w)**2 for v,w in zip(vertices[-1],vertices[0]))<.001
    polygons.append(dict(name=name,solid=name!='exterior',edge_indices=indices,vertices=vertices[:-1]))
(ROOT/'assets/arena-collision.json').write_text(json.dumps(dict(source='bsp49b.pak',mode='PvP arena_mode=1',transform='world = (local + (653.9,615.0))/2',edges=edges,polygons=polygons),indent=2))
points=[];records=[]
def convex_hull(points):
    # Round-off can turn nearly collinear BSP splits into a one-pixel dent.
    # Preserve an outward convex footprint after integer quantization.
    points=sorted(set(points))
    def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    lower=[];upper=[]
    for p in points:
        while len(lower)>=2 and cross(lower[-2],lower[-1],p)<=0:lower.pop()
        lower.append(p)
    for p in reversed(points):
        while len(upper)>=2 and cross(upper[-2],upper[-1],p)<=0:upper.pop()
        upper.append(p)
    return lower[:-1]+upper[:-1]
for polygon in polygons:
    if not polygon['solid']:continue
    vertices=[]
    for x,y in polygon['vertices']:
        point=(round(x),round(y))
        if not vertices or point!=vertices[-1]:vertices.append(point)
    if vertices[0]==vertices[-1]:vertices.pop()
    vertices=convex_hull(vertices)
    records.append((len(points),len(vertices)));points.extend(vertices)
header='#ifndef ARENA_COLLISION_H\n#define ARENA_COLLISION_H\n#include <stdint.h>\n'
header+='/* Original bsp49b PvP prop footprints, at half source resolution. */\n'
header+='typedef struct {int16_t x,y;} StagePoint;\n'
header+='typedef struct {uint16_t start,count;} StagePolygon;\n'
header+='#define STAGE_POLYGON_COUNT '+str(len(records))+'\n'
header+='static const StagePoint stage_points[]={'+','.join('{%d,%d}'%p for p in points)+'};\n'
header+='static const StagePolygon stage_polygons[]={'+','.join('{%d,%d}'%p for p in records)+'};\n#endif\n'
(ROOT/'generated/arena_collision.h').write_text(header)
im=Image.open(ROOT/'generated/arena-background.png').convert('RGB').resize((424,318));d=ImageDraw.Draw(im)
for e in edges:
    a=e['x1'],e['y1'];b=e['x2'],e['y2']; d.line((*a,*b),fill=(255,80,220),width=2)
    mid=((a[0]+b[0])/2,(a[1]+b[1])/2)
    if 0<=mid[0]<424 and 0<=mid[1]<318:d.text(mid,str(e['index']),fill=(255,255,0),stroke_width=1,stroke_fill='black')
im.save(ROOT/'assets/arena-collision-overlay.png')
print(len(edges),'original BSP edges')
