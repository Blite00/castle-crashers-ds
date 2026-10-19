#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "campaign_world.h"
#include "campaign_shop.h"
#include "campaign_barracks.h"
typedef int (*Rows)(unsigned,unsigned,unsigned,uint8_t*);
static uint8_t original[14336],out[14338];
int main(void){Rows funcs[]={campaign_world_decode_rows,campaign_shop_decode_rows,campaign_barracks_decode_rows};const unsigned strips[]={77,32,40};const char *names[]={"world","shop","barracks"};unsigned calls=0;
 for(unsigned map=0;map<3;map++){
  char path[128];snprintf(path,sizeof(path),"build/campaign_%s-row-reference.bin",names[map]);FILE *f=fopen(path,"rb");assert(f);
  for(unsigned strip=0;strip<strips[map];strip++){
   assert(fread(original,1,14336,f)==14336);
   for(unsigned first=0;first<448;first+=7){
    unsigned sizes[]={0,1,7,63,64,65,318,448-first};
    for(unsigned n=0;n<8;n++){unsigned count=sizes[n];if(count>448-first)continue;memset(out,0xa5,sizeof(out));assert(funcs[map](strip,first,count,out+1));assert(!memcmp(original+first*32,out+1,count*32));assert(out[0]==0xa5&&out[count*32+1]==0xa5&&out[14337]==0xa5);calls++;}
   }
  }
  fclose(f);assert(!funcs[map](strips[map],0,1,out));assert(!funcs[map](0,449,0,out));assert(!funcs[map](0,447,2,out));assert(!funcs[map](0,0,1,NULL));
  unsigned widths[]={1,8,318,448};for(unsigned w=0;w<4;w++){clock_t start=clock();for(unsigned repeat=0;repeat<1000;repeat++)assert(funcs[map](repeat%strips[map],0,widths[w],out));printf("%s %urows1000calls %.2fms\n",names[map],widths[w],1000.0*(clock()-start)/CLOCKS_PER_SEC);}
 }
 printf("PASS:%u partialrow requests match original indexedpixels withcanaries; invalidbounds rejected\n",calls);return 0;}
