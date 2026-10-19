#include "buried_art.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 uint8_t out[BURIED_PIXELS_MAX+2],scratch[BURIED_PIXELS_MAX+2];
 FILE*f=fopen("generated/buried-hashes.bin","rb");assert(f);
 for(unsigned id=0;id<BURIED_FRAME_COUNT;id++){
  memset(out,0xa5,sizeof out);memset(scratch,0xa5,sizeof scratch);
  assert(buried_decode(id,out+1,scratch+1));unsigned n=buried_frames[id].w*buried_frames[id].h;
  assert(out[0]==0xa5&&out[n+1]==0xa5&&scratch[0]==0xa5&&scratch[n+1]==0xa5);
  unsigned hash=2166136261u,expected;for(unsigned i=0;i<n;i++)hash=(hash^out[i+1])*16777619u;
  assert(fread(&expected,4,1,f)==1&&hash==expected);
 }
 assert(fgetc(f)==EOF);fclose(f);
 for(unsigned c=0;c<4;c++)for(unsigned t=1;t<=41;t++)assert(buried_frame(c,t)<BURIED_FRAME_COUNT);
 assert(buried_frame(4,1)==BURIED_FRAME_COUNT&&buried_frame(0,0)==BURIED_FRAME_COUNT);
 assert(!buried_decode(BURIED_FRAME_COUNT,out,scratch)&&!buried_decode(0,out,out));
 puts("PASS original buried entry/wiggle/emerge keys, four colors, decode hashes/canaries");
}
