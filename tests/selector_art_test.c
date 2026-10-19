#include "selector_art.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void){uint8_t buf[SELECTOR_MAX_PIXELS+2];static const uint32_t expected[]={4042404611u,48263351u,3591279134u,1354590616u,3420868350u,3060690425u,1755476755u,2459179492u,1616398828u,851521054u,3894668771u,435112447u,2538024792u,2468738918u,2822484163u,4032337117u,3038349208u,3830014883u,1116780857u,1004729416u};
for(unsigned f=0;f<SELECTOR_FRAME_COUNT;f++){buf[0]=17;buf[SELECTOR_MAX_PIXELS+1]=23;assert(selector_decode(f,buf+1));uint32_t hash=2166136261u;for(unsigned i=0;i<selector_frames[f].w*selector_frames[f].h;i++){assert(buf[i+1]<16);hash=(hash^selector_palette[buf[i+1]])*16777619u;}assert(hash==expected[f]);assert(buf[0]==17&&buf[SELECTOR_MAX_PIXELS+1]==23);}
assert(!selector_decode(SELECTOR_FRAME_COUNT,buf)&&!selector_decode(0,0));puts("PASS:20 original selector frames, source RGB555 hashes and decode guards");}
