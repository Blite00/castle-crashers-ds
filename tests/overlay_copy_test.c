#include "overlay_copy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint16_t source[256*192],reference[256*192],batched[256*192];
static void copy(OverlayCopy r){if(r.end){assert(r.end<=sizeof(source)&&!(r.first&3)&&!(r.end&3));memcpy((uint8_t *)batched+r.first,(uint8_t *)source+r.first,r.end-r.first);}}
int main(void){
    uint32_t random=7;unsigned calls=0,rows=0;
    for(unsigned trial=0;trial<100;trial++){
        memset(source,0,sizeof source);memset(reference,0,sizeof reference);memset(batched,0,sizeof batched);
        OverlayCopy pending={0,0};
        for(unsigned y=0;y<192;y++){
            random=random*1664525+1013904223;unsigned oldleft=(random>>16)%64*2,oldright=oldleft+100;
            random=random*1664525+1013904223;unsigned left=(random>>16)%64*2,right=left+100;
            for(unsigned x=oldleft;x<oldright;x++)reference[y*256+x]=batched[y*256+x]=0xffff;
            for(unsigned x=left;x<right;x++)source[y*256+x]=0x8000|(x+y);
            unsigned lo=left<oldleft?left:oldleft,hi=right>oldright?right:oldright;
            memcpy(reference+y*256+lo,source+y*256+lo,(hi-lo)*2);rows++;
            OverlayCopy done=overlay_copy_add(&pending,y*512+lo*2,y*512+hi*2);if(done.end){copy(done);calls++;}
        }
        copy(pending);calls++;assert(!memcmp(reference,batched,sizeof reference));
    }
    assert(calls<rows);printf("PASS: batched overlay exact pixels; %u transfers versus %u rows\n",calls,rows);
}
