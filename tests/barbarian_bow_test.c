#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "barbarian_art.h"
static unsigned char out[BARBARIAN_FRAME_PIXELS_MAX+2];
int main(void){
 for(unsigned id=245;id<265;id++){
  const ArtFrame*f=&barbarian_frames[id];unsigned n=f->w*f->h,p=0,at=f->offset;memset(out,0xc7,sizeof(out));
  while(p<n){unsigned t=barbarian_pixels[at++],count=(t&127)+1;assert(count<=n-p);if(t&128){memset(out+1+p,barbarian_pixels[at++],count);}else{memcpy(out+1+p,barbarian_pixels+at,count);at+=count;}p+=count;}
  assert(out[0]==0xc7&&out[n+1]==0xc7);assert(f->offset!=barbarian_frames[0].offset);
 }
 assert(barbarian_frames[245].offset!=barbarian_frames[255].offset);
 puts("Original Barbarian aim/release poses decode safely; bow no longer aliases idle");
}
