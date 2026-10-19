#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "blacksmith_art.h"
#include "frog_art.h"
#include "ark_art.h"
#include "original_worldmap.h"
static void check(const char*name,unsigned width,unsigned layers,const CampaignRouteLayer*records,int(*rows)(unsigned,unsigned,unsigned,uint8_t*),int(*fg)(unsigned,uint8_t*)){
 char path[128];snprintf(path,sizeof(path),"assets/campaign-%s-reference.bin",name);FILE*f=fopen(path,"rb");assert(f);uint8_t*reference=malloc(width*448);assert(reference);assert(fread(reference,1,width*448,f)==width*448);assert(fgetc(f)==EOF);fclose(f);uint8_t decoded[14338];
 for(unsigned x=0;x<width/32;x++)for(unsigned first=0;first<448;first+=31){unsigned count=448-first;if(count>79)count=79;memset(decoded,0xa5,sizeof(decoded));assert(rows(x,first,count,decoded+1));assert(decoded[0]==0xa5);for(unsigned y=0;y<count;y++)assert(!memcmp(decoded+1+y*32,reference+(first+y)*width+x*32,32));for(unsigned i=count*32+1;i<sizeof(decoded);i++)assert(decoded[i]==0xa5);}
 snprintf(path,sizeof(path),"generated/%s-fg-hashes.bin",name);f=fopen(path,"rb");assert(f);
 for(unsigned i=0;i<layers;i++){unsigned size=records[i].w*records[i].h;assert(size<=14336);memset(decoded,0xa5,sizeof(decoded));assert(fg(i,decoded+1));assert(decoded[0]==0xa5);for(unsigned j=size+1;j<sizeof(decoded);j++)assert(decoded[j]==0xa5);uint32_t want,hash=2166136261u;assert(fread(&want,4,1,f)==1);for(unsigned j=0;j<size;j++)hash=(hash^decoded[j+1])*16777619u;assert(hash==want);}
 assert(fgetc(f)==EOF);fclose(f);assert(!rows(width/32,0,1,decoded));assert(!rows(0,448,1,decoded));assert(!rows(0,0,449,decoded));assert(!fg(layers,decoded));free(reference);
}
int main(void){
check("blacksmith",768,CAMPAIGN_BLACKSMITH_FG_COUNT,campaign_blacksmith_fg_layers,campaign_blacksmith_decode_rows,campaign_blacksmith_decode_fg);
check("frog",3200,CAMPAIGN_FROG_FG_COUNT,campaign_frog_fg_layers,campaign_frog_decode_rows,campaign_frog_decode_fg);
check("ark",2816,CAMPAIGN_ARK_FG_COUNT,campaign_ark_fg_layers,campaign_ark_decode_rows,campaign_ark_decode_fg);
FILE*f=fopen("assets/worldmap-reference.bin","rb");assert(f);unsigned char ref[672*256],out[6146];assert(fread(ref,1,sizeof ref,f)==sizeof ref);fclose(f);for(unsigned x=0;x<21;x++)for(unsigned y=0;y<256;y+=29){unsigned n=256-y;if(n>192)n=192;memset(out,0xa5,sizeof out);assert(original_worldmap_decode_rows(x,y,n,out+1));assert(out[0]==0xa5&&out[n*32+1]==0xa5);for(unsigned j=0;j<n;j++)assert(!memcmp(out+1+j*32,ref+(y+j)*672+x*32,32));}puts("PASS original hub terrain, foreground, world map source indices and guards");return 0;}
