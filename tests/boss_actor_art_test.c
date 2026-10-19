#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "boss_actor_art.h"
int main(void){FILE*f=fopen("generated/boss-actor-hashes.bin","rb");assert(f);uint8_t data[BOSS_ACTOR_PIXELS_MAX+2];
 for(unsigned i=0;i<BOSS_ACTOR_TILE_COUNT;i++){memset(data,0xa5,sizeof(data));assert(boss_actor_decode_tile(i,data+1));const BossActorTile*t=&boss_actor_tiles[i];unsigned size=t->w*t->h;assert(data[0]==0xa5);for(unsigned j=size+1;j<sizeof(data);j++)assert(data[j]==0xa5);uint32_t expected,hash=2166136261u;assert(fread(&expected,4,1,f)==1);for(unsigned j=0;j<size;j++){assert(data[j+1]<128);hash=(hash^data[j+1])*16777619u;}assert(hash==expected);}
 assert(fgetc(f)==EOF);fclose(f);
 for(unsigned pose=0;pose<17;pose++)for(unsigned tick=0;tick<400;tick++){unsigned id=boss_actor_frame_for(pose,tick);assert(id<BOSS_ACTOR_FRAME_COUNT);assert(id>=boss_actor_pose_start[pose]);assert(id<boss_actor_pose_start[pose]+boss_actor_pose_count[pose]);}
 /* Every chosen knight retains its exact RGB555 source helmet color. */
 const unsigned helmet_rgb[4][3]={{102,153,255},{255,153,0},{255,0,0},{102,204,102}};
 for(unsigned knight=0;knight<4;knight++){
  uint16_t color=0x8000u|(helmet_rgb[knight][0]>>3)|((helmet_rgb[knight][1]>>3)<<5)|((helmet_rgb[knight][2]>>3)<<10);
  for(unsigned tick=0;tick<86;tick++){
   const BossActorFrame*frame=&boss_actor_frames[boss_actor_frame_for(13+knight,tick)];unsigned colored=0;
   for(unsigned j=0;j<frame->count;j++){unsigned tile=frame->first+j;assert(boss_actor_decode_tile(tile,data));const BossActorTile*t=&boss_actor_tiles[tile];for(unsigned k=0;k<t->w*t->h;k++)colored+=boss_actor_palette[data[k]]==color;}
   assert(colored>=40);
  }
 }
 assert(boss_actor_frames[boss_actor_frame_for(BOSS_ACTOR_CHEST_OPEN,141)].count>0);
 assert(boss_actor_frames[boss_actor_frame_for(BOSS_ACTOR_CHEST_OPEN,142)].count==0);
 assert(!boss_actor_decode_tile(BOSS_ACTOR_TILE_COUNT,data));assert(!boss_actor_decode_tile(0,NULL));puts("PASS: original boss/ending actor tile hashes, bounded decoders and all17 source timelines");return 0;
}
