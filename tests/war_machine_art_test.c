#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "war_machine_art.h"
int main(void){FILE*f=fopen("generated/war-machine-hashes.bin","rb");assert(f);uint8_t data[WAR_MACHINE_PIXELS_MAX+2];
 for(unsigned i=0;i<WAR_MACHINE_TILE_COUNT;i++){memset(data,0xa5,sizeof(data));assert(war_machine_decode_tile(i,data+1));const WarMachineTile*t=&war_machine_tiles[i];unsigned size=t->w*t->h;assert(data[0]==0xa5);for(unsigned j=size+1;j<sizeof(data);j++)assert(data[j]==0xa5);uint32_t expected,hash=2166136261u;assert(fread(&expected,4,1,f)==1);for(unsigned j=0;j<size;j++){assert(data[j+1]<128);hash=(hash^data[j+1])*16777619u;}assert(hash==expected);}
 assert(fgetc(f)==EOF);fclose(f);
 for(unsigned i=0;i<war_machine_pose_start[WAR_MACHINE_JACK];i++){const WarMachineFrame*frame=&war_machine_frames[i];for(unsigned j=0;j<frame->count;j++)assert(war_machine_tiles[frame->first+j].length&0x80000000u);}
 for(unsigned pose=0;pose<4;pose++)for(unsigned tick=0;tick<200;tick++){unsigned id=war_machine_frame_for(pose,tick);assert(id<WAR_MACHINE_FRAME_COUNT);assert(id>=war_machine_pose_start[pose]);assert(id<war_machine_pose_start[pose]+war_machine_pose_count[pose]);}
 assert(!war_machine_decode_tile(WAR_MACHINE_TILE_COUNT,data));assert(!war_machine_decode_tile(0,NULL));puts("PASS:579 source machine tile hashes/canaries,27 keyframes and fastRLE idle/walk");return 0;
}
