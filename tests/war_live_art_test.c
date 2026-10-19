#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "war_live_art.h"
int main(void){uint8_t b[WAR_LIVE_PIXELS_MAX+2];
 for(unsigned id=0;id<WAR_LIVE_FRAME_COUNT;id++){
  unsigned n=(unsigned)war_live_frames[id].w*war_live_frames[id].h;
  memset(b,0xa5,sizeof b);assert(war_live_decode(id,b+1));
  assert(b[0]==0xa5&&b[n+1]==0xa5);
 }
 assert(!war_live_decode(WAR_LIVE_FRAME_COUNT,b));puts("PASS original horse/rider/sleep24 keys and bounded fastRLE");return 0;
}
