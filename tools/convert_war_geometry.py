from pathlib import Path
import struct,math
R=Path(__file__).resolve().parents[1]
for name,level,width,ox,oy in [('war',11,4608,-2112,-700),('bridge',12,1024,-1024,-800),('forest',18,2304,-256,-736)]:
 b=(R/f'assets/extracted/bsp{level}.bin').read_bytes();n=struct.unpack_from('<I',b,16)[0]//8
 edges=[struct.unpack_from('<8f',b,20+i*32)[1:6] for i in range(n)]
 valid=[]
 for x in range(width):
  wx=x*2+ox;ys=[]
  for x1,y1,x2,y2,k in edges:
   if k in (1,2,3) and min(x1,x2)<=wx<max(x1,x2):ys.append(y1+(y2-y1)*(wx-x1)/(x2-x1))
  valid.append((max(0,math.ceil((min(ys)-oy)/2)+5),min(447,math.floor((max(ys)-oy)/2)-5)) if len(ys)>=2 else None)
 # Outside source corridor is blocked, never synthesize a traversable floor.
 runs=[]
 for x,v in enumerate(valid):
  if v is None or v[0]>v[1]:v=(448,0)
  if not runs or runs[-1][1:]!=v:runs.append((x,*v))
 guard=f'{name.upper()}_GEOMETRY_H'
 h=f'#ifndef {guard}\n#define {guard}\n#include <stdint.h>\n/* BSP{level} source corridor, half scale; 5px foot margin. Lossless runs. */\n'
 h+=f'static const uint16_t {name}_floor_runs[{len(runs)}][3]={{\n'+''.join('{%d,%d,%d},'%r+('\n' if i%8==7 else '') for i,r in enumerate(runs))+'\n};\n'
 h+=f'''static inline unsigned {name}_floor_bound(unsigned x,unsigned bound){{
 unsigned lo=0,hi={len(runs)};
 if(x>={width})return bound?0:448;
 while(lo+1<hi){{unsigned mid=(lo+hi)/2;if({name}_floor_runs[mid][0]<=x)lo=mid;else hi=mid;}}
 return {name}_floor_runs[lo][bound?2:1];
}}
#define {name}_floor_min(x) {name}_floor_bound((x),0)
#define {name}_floor_max(x) {name}_floor_bound((x),1)
#endif
'''
 (R/f'generated/{name}_geometry.h').write_text(h)
 print(name,len(runs),len(runs)*6,'bytes', 'openrange',next(i for i,v in enumerate(valid) if v),max(i for i,v in enumerate(valid) if v))
