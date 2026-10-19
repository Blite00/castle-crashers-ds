#include "hub_ambient_art.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){unsigned char out[HUB_AMBIENT_PIXELS_MAX+2];unsigned hashes[HUB_AMBIENT_FRAME_COUNT];
for(unsigned i=0;i<HUB_AMBIENT_FRAME_COUNT;i++){memset(out,0xa5,sizeof out);assert(hub_ambient_decode(i,out+1));unsigned n=hub_ambient_frames[i].w*hub_ambient_frames[i].h;assert(n<=HUB_AMBIENT_PIXELS_MAX&&out[0]==0xa5&&out[n+1]==0xa5);unsigned h=2166136261u;for(unsigned j=0;j<n;j++){assert(out[j+1]<128);h=(h^out[j+1])*16777619u;}hashes[i]=h;}
assert(hashes[0]!=hashes[4]&&hashes[4]!=hashes[8]);assert(!hub_ambient_decode(HUB_AMBIENT_FRAME_COUNT,out));assert(!hub_ambient_decode(0,NULL));puts("Original smith animation: nine distinct source keys, palette and bounded decode PASS");return 0;}
