#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "barracks_rail.h"
#include "barracks_rail_hash.h"
int main(void){
 uint32_t hash=2166136261u;
 for(int y=0;y<448;y++)for(int x=0;x<1280;x++)hash=(hash^(unsigned)barracks_rail_pixel(x,y))*16777619u;
 assert(hash==BARRACKS_RAIL_REFERENCE_HASH);
 for(int x=-100;x<1400;x++)assert(!barracks_rail_pixel(x,-1)&&!barracks_rail_pixel(x,448));
 for(int y=-100;y<600;y++)assert(!barracks_rail_pixel(-1,y)&&!barracks_rail_pixel(1280,y));
 /* Regression points in the narrow band between helper plane and wall. */
 assert(barracks_rail_behind(1000,243));
 assert(barracks_rail_behind(1025,226));
 assert(barracks_rail_behind(1092,181));
 assert(!barracks_rail_behind(1000,245));
 assert(barracks_rail_behind(1000,224));assert(!barracks_rail_behind(1000,264));
 assert(!barracks_rail_behind(800,200));assert(!barracks_rail_behind(1200,100));
 puts("Original barracks rail: full-world alpha hash, bounds and sloped front/back checks passed");
}
