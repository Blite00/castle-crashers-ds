#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "barracks_actor_art.h"
static uint8_t pixels[BARRACKS_ACTOR_MAX_PIXELS+2];
int main(void){
 for(unsigned i=0;i<sizeof(barracks_actor_frames)/sizeof(*barracks_actor_frames);i++){
  const BarracksActorFrame*f=&barracks_actor_frames[i];unsigned n=f->w*f->h;
  memset(pixels,0xc7,sizeof(pixels));assert(barracks_actor_decode(i,pixels+1));assert(pixels[0]==0xc7&&pixels[n+1]==0xc7);
  if(i>=BARRACKS_DRINKER_FIRST&&i<BARRACKS_DRINKER_FIRST+BARRACKS_DRINKER_COUNT){
   assert(f->h>=28&&f->y<=-19);unsigned head=0;
   for(unsigned j=0;j<f->w*12u;j++)head+=pixels[j+1]!=0;
   assert(head>=60);
  }
 }
 puts("All barracks actors decode with guards; both drinking poses retain full helmet coverage");
}

