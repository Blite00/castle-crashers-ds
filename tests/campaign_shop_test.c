#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "campaign_world.h"
#include "campaign_shop.h"
#include "campaign_shop_geometry.h"
int main(void){uint8_t guard[14338];for(unsigned i=0;i<32;i++){memset(guard,0xa5,sizeof guard);assert(campaign_shop_decode_strip(i,guard+1));assert(guard[0]==0xa5&&guard[14337]==0xa5);for(int n=1;n<=14336;n++)assert(guard[n]&&guard[n]<=128); }assert(!campaign_shop_decode_strip(32,guard));for(int x=0;x<1024;x++)assert(campaign_shop_floor_min[x]<=campaign_shop_floor_max[x]);return 0;}
