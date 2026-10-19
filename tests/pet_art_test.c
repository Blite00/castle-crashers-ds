#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "pet_art.h"
#include "dig_art.h"
static void verify(const char*path,const ArtFrame*frames,unsigned count,const uint16_t*palette,int(*decode)(unsigned,uint8_t*),unsigned capacity){
 FILE*f=fopen(path,"rb");assert(f);uint8_t guard[DIG_FRAME_PIXELS_MAX+32];
 for(unsigned repeat=0;repeat<5;repeat++){
  rewind(f);
  for(unsigned i=0;i<count;i++){
   uint8_t bytes[4];assert(fread(bytes,1,4,f)==4);uint32_t expected=bytes[0]|((uint32_t)bytes[1]<<8)|((uint32_t)bytes[2]<<16)|((uint32_t)bytes[3]<<24);
   memset(guard,0xa5,sizeof guard);assert(decode(i,guard+16));unsigned n=frames[i].w*frames[i].h;assert(n<=capacity);
   uint32_t hash=2166136261u;for(unsigned j=0;j<n;j++)hash=(hash^palette[guard[16+j]])*16777619u;
   assert(hash==expected);for(unsigned j=0;j<16;j++)assert(guard[j]==0xa5);for(unsigned j=16+n;j<sizeof guard;j++)assert(guard[j]==0xa5);
  }
 }
 memset(guard,0xa5,sizeof guard);assert(!decode(count,guard+16));assert(!decode(0,NULL));for(unsigned j=0;j<sizeof guard;j++)assert(guard[j]==0xa5);fclose(f);
}
int main(void){
 verify("generated/pet-art-test-hashes.bin",pet_frames,PET_FRAME_COUNT,pet_palette,pet_decode_frame,PET_FRAME_PIXELS_MAX);
 verify("generated/dig-art-test-hashes.bin",dig_frames,DIG_FRAME_COUNT,dig_palette,dig_decode_frame,DIG_FRAME_PIXELS_MAX);
 assert(pet_art_frame(0,0,0)==PET_FRAME_COUNT);assert(pet_art_frame(17,0,0)==0);assert(pet_art_frame(24,0,0)==18);assert(pet_art_frame(2,0,0)==42);assert(PET_FRAME_COUNT==96&&pet_art_frame(9,0,0)==60&&pet_art_frame(21,0,0)==78);for(unsigned t=0;t<1000;t++){assert(pet_art_frame(2,PET_ART_FOLLOW,t)<47);assert(pet_art_frame(2,PET_ART_HAPPY,t)>=47&&pet_art_frame(2,PET_ART_HAPPY,t)<60);}
 for(unsigned tick=0;tick<1000;tick++){assert(pet_art_frame(24,PET_ART_DIG,tick)>=36);assert(pet_art_frame(24,PET_ART_DIG,tick)<42);assert(pet_art_frame(17,PET_ART_FOLLOW,tick)<5);assert(pet_art_frame(24,PET_ART_HAPPY,tick)>=23&&pet_art_frame(24,PET_ART_HAPPY,tick)<36);}
 for(unsigned c=0;c<4;c++){assert(dig_art_frame(c,1)==c*14);assert(dig_art_frame(c,39)==c*14+7);assert(dig_art_frame(c,76)==c*14+13);}
 assert(dig_art_frame(4,1)==DIG_FRAME_COUNT);puts("Pet/dig: all source pixel hashes, lookup timing and decode guards PASS");return 0;
}
