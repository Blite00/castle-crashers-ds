"""LEVEL22 lane: BSP22 is a 1577x256 source box (x -1107..470, y -1113..-857) that the original AutoRun scrolls through; the DS world is
the flat flume from tools/deer_scene.py, so the lane is open along all of it. Rows follow the plank area of the art (DEER_LANE_*)."""
from pathlib import Path
import re
R=Path(__file__).resolve().parents[1]
lay=(R/'generated/deer_layout.h').read_text();W=int(re.search(r'DEER_WIDTH (\d+)',lay)[1]);top=int(re.search(r'DEER_LANE_TOP (\d+)',lay)[1]);bot=int(re.search(r'DEER_LANE_BOTTOM (\d+)',lay)[1])
runs=[(0,448,0),(60,top,bot),(W-60,448,0)]
h='#ifndef DEER_GEOMETRY_H'+chr(10)+'#define DEER_GEOMETRY_H'+chr(10)+'#include <stdint.h>'+chr(10)
h+=f'#define DEER_GEOMETRY_WIDTH {W}'+chr(10)
h+='static const uint16_t deer_floor_runs[3][3]={'+','.join('{%d,%d,%d}'%r for r in runs)+'};'+chr(10)
h+='''static inline unsigned deer_floor_bound(unsigned x,unsigned bound){
 if(x>=%d)return bound?0:448;
 unsigned i=x>=%d?2:x>=60?1:0;return deer_floor_runs[i][bound?2:1];
}
#define deer_floor_min(x) deer_floor_bound((x),0)
#define deer_floor_max(x) deer_floor_bound((x),1)
#endif
'''%(W,W-60)
(R/'generated/deer_geometry.h').write_text(h);print('ok')
