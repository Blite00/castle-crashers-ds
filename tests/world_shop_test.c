#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "shop_board_art.h"
#include "shop_board_hashes.h"
static uint8_t out[SHOP_BOARD_MAX_PIXELS+2];
int main(void){
 for(unsigned i=0;i<10;i++){
  memset(out,0xc7,sizeof(out));assert(shop_board_decode(i,out+1));unsigned n=shop_board_frames[i].w*shop_board_frames[i].h;uint32_t hash=2166136261u;
  assert(out[0]==0xc7&&out[n+1]==0xc7);
  for(unsigned j=0;j<n;j++){assert(out[j+1]<64);hash=(hash^out[j+1])*16777619u;}
  assert(hash==shop_board_hashes[i]);
 }
 assert(shop_board_hashes[0]!=shop_board_hashes[1]&&shop_board_hashes[2]!=shop_board_hashes[3]);
 assert(!shop_board_decode(10,out)&&!shop_board_decode(0,0));puts("Original LEVEL19 world boards:10 states pixelhashes/guards pass; price/capacity/funds differ");
}
