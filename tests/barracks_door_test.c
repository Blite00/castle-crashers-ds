#include "barracks_door_art.h"
#include "barracks_door_hashes.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t guarded[BARRACKS_DOOR_MAX_PIXELS+2];
int main(void){
 for(unsigned i=0;i<BARRACKS_DOOR_FRAME_COUNT;i++){
  memset(guarded,0xa5,sizeof guarded);const BarracksDoorFrame *f=&barracks_door_frames[i];
  assert(barracks_door_decode(i,guarded+1));unsigned n=(unsigned)f->w*f->h;
  assert(guarded[0]==0xa5&&guarded[n+1]==0xa5);
  uint32_t hash=2166136261u;for(unsigned p=0;p<n;p++){assert(guarded[p+1]<64);hash=(hash^guarded[p+1])*16777619u;}
  assert(hash==barracks_door_hashes[i]);
 }
 assert(!barracks_door_decode(BARRACKS_DOOR_FRAME_COUNT,guarded+1));
 assert(barracks_door_frames[0].w>1&&barracks_door_frames[BARRACKS_DOOR_FRAME_COUNT-1].w==1);
 puts("barracks door: 17 original frames, open final state, exact pixels and bounded decode passed");return 0;
}
