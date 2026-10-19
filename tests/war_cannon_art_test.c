#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "war_cannon_art.h"
int main(void){uint8_t b[WAR_CANNON_PIXELS_MAX+2];
 for(unsigned f=0;f<WAR_CANNON_FRAME_COUNT;f++){
  const WarCannonFrame*frame=&war_cannon_frames[f];assert(frame->count);
  for(unsigned i=frame->first;i<frame->first+frame->count;i++){
   unsigned n=(unsigned)war_cannon_tiles[i].w*war_cannon_tiles[i].h;assert(n<=WAR_CANNON_PIXELS_MAX);
   memset(b,0xa5,sizeof b);assert(war_cannon_decode_tile(i,b+1));assert(b[0]==0xa5&&b[n+1]==0xa5);
  }
 }
 assert(!war_cannon_decode_tile(999,b));puts("PASS original cannonball/bomb/blast bounded tiles");return 0;
}
