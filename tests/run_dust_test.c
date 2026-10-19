#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "run_dust_art.h"
static uint8_t out[RUN_DUST_MAX_PIXELS+2];
int main(void){
 for(unsigned i=0;i<3;i++){
  memset(out,0xc7,sizeof(out));assert(run_dust_decode(i,out+1));unsigned n=run_dust_frames[i].w*run_dust_frames[i].h,opaque=0;
  assert(out[0]==0xc7&&out[n+1]==0xc7);for(unsigned j=0;j<n;j++){assert(out[j+1]<3);opaque+=out[j+1]!=0;}assert(opaque>10);
 }
 assert(!run_dust_decode(3,out)&&!run_dust_decode(0,0));puts("Original dust three frames decode with canaries and exact palette indices");
}
