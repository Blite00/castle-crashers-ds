#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boss_art.h"
static void check(const char*name,unsigned width,unsigned layers,const CampaignRouteLayer*records,int(*rows)(unsigned,unsigned,unsigned,uint8_t*),int(*fg)(unsigned,uint8_t*)){
 char path[128];snprintf(path,sizeof(path),"assets/campaign-%s-reference.bin",name);FILE*f=fopen(path,"rb");assert(f);uint8_t*reference=malloc(width*448);assert(reference);assert(fread(reference,1,width*448,f)==width*448);assert(fgetc(f)==EOF);fclose(f);uint8_t decoded[14338];
 for(unsigned x=0;x<width/32;x++)for(unsigned first=0;first<448;first+=31){unsigned count=448-first;if(count>79)count=79;memset(decoded,0xa5,sizeof(decoded));assert(rows(x,first,count,decoded+1));assert(decoded[0]==0xa5);for(unsigned y=0;y<count;y++)assert(!memcmp(decoded+1+y*32,reference+(first+y)*width+x*32,32));for(unsigned i=count*32+1;i<sizeof(decoded);i++)assert(decoded[i]==0xa5);}
 snprintf(path,sizeof(path),"generated/%s-fg-hashes.bin",name);f=fopen(path,"rb");assert(f);
 for(unsigned i=0;i<layers;i++){unsigned size=records[i].w*records[i].h;assert(size<=14336);memset(decoded,0xa5,sizeof(decoded));assert(fg(i,decoded+1));assert(decoded[0]==0xa5);for(unsigned j=size+1;j<sizeof(decoded);j++)assert(decoded[j]==0xa5);uint32_t want,hash=2166136261u;assert(fread(&want,4,1,f)==1);for(unsigned j=0;j<size;j++)hash=(hash^decoded[j+1])*16777619u;assert(hash==want);}
 assert(fgetc(f)==EOF);fclose(f);assert(!rows(width/32,0,1,decoded));assert(!rows(0,448,1,decoded));assert(!rows(0,0,449,decoded));assert(!fg(layers,decoded));free(reference);
}
int main(void){check("boss",1024,CAMPAIGN_BOSS_FG_COUNT,campaign_boss_fg_layers,campaign_boss_decode_rows,campaign_boss_decode_fg);puts("PASS: source LEVEL7 terrain/foreground hashes and bounded row decoder");return 0;}
