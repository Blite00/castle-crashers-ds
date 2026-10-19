"""Original BSP walk corridor in continuous half-scale LEVEL13 coordinates."""
from pathlib import Path
import struct,math
R=Path(__file__).resolve().parents[1];r=(R/'assets/extracted/bsp13.bin').read_bytes();n=struct.unpack_from('<I',r,16)[0]//8
edges=[struct.unpack_from('<8f',r,20+i*32)[1:6] for i in range(n)]
mins=[];maxs=[]
for x in range(2448):
 wx=x*2-200;ys=[]
 for x1,y1,x2,y2,k in edges:
  if k in (1,2) and min(x1,x2)<=wx<max(x1,x2):ys.append(y1+(y2-y1)*(wx-x1)/(x2-x1))
 if not ys:ys=[-420,-205] if wx<0 else [-435,-267]
 lo=max(8,min(440,math.ceil((min(ys)+810)/2)+5));hi=max(lo,min(443,math.floor((max(ys)+810)/2)-5));mins.append(lo);maxs.append(hi)
h='#ifndef CAMPAIGN_WORLD_GEOMETRY_H\n#define CAMPAIGN_WORLD_GEOMETRY_H\n#include <stdint.h>\n/* Source coordinates convert to world=(source+(200,810))/2. */\n'
for name,data in [('campaign_world_floor_min',mins),('campaign_world_floor_max',maxs)]:h+='static const uint16_t '+name+'[2448]={'+','.join(map(str,data))+'};\n'
# Original BSP100/101 helper planes define each stair surface; the outer wall
# envelope is not a height map. Keep these distinct for bounded slope following.
ramp_edges=[[(110,0),(114,0),(107,1),(107,0)],[(19,0),(113,0),(26,1),(7,0)],[(10,1),(10,0),(127,0),(130,0)]]
ramps=[]
for ids in ramp_edges:
 points=[(round((edges[e][end*2]+200)/2),round((edges[e][end*2+1]+810)/2)) for e,end in ids]
 a,b=(points[0],points[3]) if len(ramps)<2 else (points[1],points[2])
 ramps.append((points,b[0]-a[0],b[1]-a[1]))
h+='typedef struct {int16_t x[4],y[4],rise_x,rise_y;} CampaignStair;\n'
h+='#define CAMPAIGN_STAIR_COUNT 3\nstatic const CampaignStair campaign_stairs[]={'+','.join('{{'+','.join(str(x) for x,y in pts)+'},{'+','.join(str(y) for x,y in pts)+'},'+str(dx)+','+str(dy)+'}' for pts,dx,dy in ramps)+'};\n'
h+='#endif\n';(R/'generated/campaign_world_geometry.h').write_text(h)
print('Continuous corridor limits generated:2448columns; BSP-derived outer envelope')
