#include "war_geometry.h"
#include "bridge_geometry.h"
#include "war_sequence.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(war_floor_min(0)>war_floor_max(0));assert(bridge_floor_min(938)>bridge_floor_max(938));
 for(int x=204;x<=4450;x++)assert(war_floor_min(x)<=war_floor_max(x));
 for(int x=168;x<=816;x++)assert(bridge_floor_min(x)<=bridge_floor_max(x));
 for(int i=0;i<39;i++){int x=war_spawns[i].x,y=war_spawns[i].y;assert(war_floor_min(x)<=war_floor_max(x));assert(y+5>=(int)war_floor_min(x)&&y-5<=(int)war_floor_max(x));}
 assert(267>=(int)bridge_floor_min(816)&&267<=(int)bridge_floor_max(816));
 puts("War/bridge BSP source spawn margins and continuous route pass");return 0;
}
