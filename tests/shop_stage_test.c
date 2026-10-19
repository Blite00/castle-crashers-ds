#include "stage.h"
#include <assert.h>
int main(void){
 stage_set_map(4);assert(stage_world_width(4)==1024&&stage_world_height(4)==448&&stage_max_x()==990);
 const int spawn[][2]={{178,144},{514,294},{639,274},{589,294},{664,309},{689,279},{739,319},{759,274},{965,279}};
 for(unsigned i=0;i<sizeof(spawn)/sizeof(spawn[0]);i++)assert(stage_position_open(spawn[i][0],spawn[i][1]));
 assert(!stage_position_open_map(4,1100,290));assert(stage_world_width(3)==2448);
 Fighter f={.x=514,.y=144};stage_move(&f,178,144);assert(f.x==514&&f.y>240&&stage_position_open(f.x,f.y));
 f=(Fighter){.x=178,.y=f.y};stage_move(&f,514,f.y);assert(f.x==178&&f.y==144);
 f=(Fighter){.x=965,.y=294};stage_move(&f,589,294);assert(f.x==965&&stage_position_open(f.x,f.y));
 return 0;
}
