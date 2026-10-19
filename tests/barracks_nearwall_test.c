#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "barracks_nearwall.h"
#include "barracks_nearwall_hash.h"
int main(void){
 uint32_t hash=2166136261u;
 for(int y=0;y<448;y++)for(int x=0;x<1280;x++)hash=(hash^(unsigned)barracks_nearwall_pixel(x,y))*16777619u;
 assert(hash==BARRACKS_NEARWALL_REFERENCE_HASH);
 for(int x=-100;x<1400;x++)assert(!barracks_nearwall_pixel(x,-1)&&!barracks_nearwall_pixel(x,448));
 for(int y=-100;y<600;y++)assert(!barracks_nearwall_pixel(-1,y)&&!barracks_nearwall_pixel(1280,y));
 assert(barracks_nearwall_behind(1100,181));assert(barracks_nearwall_behind(1100,182));assert(!barracks_nearwall_behind(1100,183));
 assert(barracks_nearwall_departing(1067));assert(!barracks_nearwall_departing(1000));
 /* Authored actors approach at (1067,185), before the live hero depth gate. */
 assert(!barracks_nearwall_behind(1067,185)&&barracks_nearwall_departing(1067));
 assert(!barracks_nearwall_pixel(1090,140));assert(barracks_nearwall_pixel(1120,140));
 puts("Original barracks rail: full-world alpha hash, bounds and sloped front/back checks passed");
}
