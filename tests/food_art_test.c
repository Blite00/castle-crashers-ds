#include <assert.h>
#include <stdio.h>
#include "loot_art.h"
#include "loot_hashes.h"
int main(void){
 for(unsigned i=0;i<10;i++){
  const ArtFrame*f=&loot_frames[i];uint32_t h=2166136261u;
  for(unsigned n=0;n<f->w*f->h;n++){unsigned c=loot_pixels[f->offset+n];assert(c<128);h=(h^c)*16777619u;}
  assert(h==loot_hashes[i]);
 }
 for(unsigned v=0;v<5;v++){int frame=loot_food_frame(10|(v<<5));assert(frame==(v?5+(int)v:2));assert(loot_frames[frame].w>10);for(unsigned other=0;other<v;other++)assert(loot_hashes[frame]!=loot_hashes[loot_food_frame(10|(other<<5))]);}
 assert(loot_food_frame(255)==2);puts("Original five foods and shop potion: all10pixelhashes, bounds and variantmapping pass");
}
