#include "forest_croc_art.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Every baked MOUNT2 crocodile frame inflates to exactly its rectangle, stays inside its guard bytes and uses only palette entries. */
int main(void){
    uint8_t guard[FOREST_CROC_PIXELS_MAX+32];
    for(unsigned i=0;i<FOREST_CROC_FRAMES;i++){
        const ForestCrocFrame *f=&forest_croc_frames[i];unsigned n=(unsigned)f->w*f->h;assert(n&&n<=FOREST_CROC_PIXELS_MAX);
        memset(guard,0xa5,sizeof guard);assert(forest_croc_decode(i,guard+16));
        unsigned opaque=0;for(unsigned j=0;j<n;j++){assert(guard[16+j]<64);opaque+=guard[16+j]!=0;}
        for(unsigned j=0;j<16;j++)assert(guard[j]==0xa5);
        for(unsigned j=16+n;j<sizeof guard;j++)assert(guard[j]==0xa5);
        assert(opaque>n/8); /* A real drawing, not an empty rectangle. */
        assert(f->x>-100&&f->x+f->w<100&&f->y>-100&&f->y<10); /* Around the mount's ground point. */
    }
    assert(!forest_croc_decode(FOREST_CROC_FRAMES,guard)&&!forest_croc_decode(0,NULL));
    puts("PASS: 19 baked crocodile mount frames decode exactly with guards and bounds");return 0;
}
