#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "thief_art.h"
int main(void){uint8_t b[THIEF_FRAME_PIXELS_MAX+2];
 for(unsigned id=0;id<THIEF_FRAME_COUNT;id++){
  unsigned n=(unsigned)thief_frames[id].w*thief_frames[id].h;
  memset(b,0xa5,sizeof b);assert(thief_decode_frame(id,b+1));
  assert(b[0]==0xa5&&b[n+1]==0xa5);
 }
 assert(!thief_decode_frame(THIEF_FRAME_COUNT,b));puts("PASS original Thief304 poses and bounded fastRLE");return 0;
}
