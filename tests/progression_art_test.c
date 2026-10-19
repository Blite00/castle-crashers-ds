#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "progression_art.h"
int main(void){
 FILE*f=fopen("generated/progression-art-hashes.bin","rb");assert(f);uint8_t data[PROGRESSION_ART_MAX_PIXELS+2];
 for(unsigned i=0;i<2;i++){
  memset(data,0xa5,sizeof(data));assert(progression_decode(i,data+1));const ProgressionArtFrame*a=&progression_frames[i];unsigned size=a->w*a->h;assert(size<=2048);assert(data[0]==0xa5);for(unsigned j=size+1;j<sizeof(data);j++)assert(data[j]==0xa5);uint32_t expected,hash=2166136261u;assert(fread(&expected,4,1,f)==1);for(unsigned j=0;j<size;j++)hash=(hash^progression_palette[data[j+1]])*16777619u;assert(hash==expected);
 }
 assert(fgetc(f)==EOF);fclose(f);assert(!progression_decode(2,data));assert(!progression_decode(0,NULL));puts("PASS: source LEVEL and New Combo label pixels, tiny decode bounds and canaries");return 0;
}
