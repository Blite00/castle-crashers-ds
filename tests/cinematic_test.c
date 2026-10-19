#include "cinematic.h"
#include "cinematic_test_hashes.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint16_t guarded[256*192+2];
static uint32_t hash(void){
    uint32_t h=2166136261u;
    for(unsigned i=1;i<=256*192;i++)h=(h^guarded[i])*16777619u;
    return h;
}
static void verify(void){
    for(unsigned kind=0;kind<2;kind++){
        uint32_t first=0,last=0;unsigned changes=0;
        for(unsigned tick=0;tick<cinematic_duration(kind)+8;tick+=2){
            for(unsigned i=0;i<256*192+2;i++)guarded[i]=0x1234;
            cinematic_draw(guarded+1,kind,tick);
            assert(guarded[0]==0x1234 && guarded[256*192+1]==0x1234);
            unsigned visible=0;
            for(unsigned i=1;i<=256*192;i++){
                assert(guarded[i]!=0x1234);
                assert(guarded[i]&0x8000);visible++;
            }
            assert(visible==256*192);
            uint32_t h=hash();if(!tick)first=h;else if(h!=last)changes++;
            unsigned frame=cinematic_frame(kind,tick);
            assert(h==cinema_expected_hashes[cinema_test_clip_offsets[kind]+frame]);
            if(tick>=cinematic_duration(kind))assert(h==last);
            last=h;
        }
        assert(changes>10); (void)first;
    }
    /* Alternating destinations, backwards seeks, and cross-clip jumps must
       rebuild independently of whatever the double buffer contained before. */
    for(unsigned i=0;i<80;i++){
        unsigned kind=i%2,tick=(i*97)%cinematic_duration(kind);
        memset(guarded+1,(int)i,256*192*sizeof(uint16_t));
        cinematic_draw(guarded+1,kind,tick);
        assert(hash()==cinema_expected_hashes[cinema_test_clip_offsets[kind]+cinematic_frame(kind,tick)]);
    }
    assert(cinematic_duration(0)==364 && cinematic_duration(1)==232);
    /* Only the authored opening is black; crystaldeparture and princess
       choreography must not inherit the similarly named MUSICFadeOut call. */
    cinematic_draw(guarded+1,CINEMATIC_CRYSTAL,0);
    for(unsigned i=1;i<=256*192;i++)assert(guarded[i]==0x8000);
    cinematic_draw(guarded+1,CINEMATIC_CRYSTAL,348);
    unsigned lit=0;for(unsigned i=1;i<=256*192;i++)lit+=guarded[i]!=0x8000;
    assert(lit>256*192/2);
    int x,y;cinematic_camera(CINEMATIC_PRINCESSES,0,&x,&y);
    assert(x==1242&&y==98);
    cinematic_camera(CINEMATIC_PRINCESSES,cinematic_duration(1)-1,&x,&y);
    assert(x==1295&&y==72);
    uint32_t prior=hash();cinematic_draw(guarded+1,2,0);assert(prior==hash());
    cinematic_draw(0,0,0);
    puts("Cinematic: all298 complete source-choreography hashes, complete backgrounds, clipping guards and end holds pass");
}


static uint8_t workspace[128002];
int main(void){
    verify();
    memset(workspace,0xa5,sizeof workspace);
    cinematic_set_workspace(workspace+1,128000);
    verify();
    assert(workspace[0]==0xa5&&workspace[128001]==0xa5);
    cinematic_reset();unsigned before=cinematic_inflate_count();
    for(unsigned t=0;t<cinematic_duration(0);t+=2){
        cinematic_draw(guarded+1,0,t);
        assert(hash()==cinema_expected_hashes[cinematic_frame(0,t)]);
    }
    unsigned calls=cinematic_inflate_count()-before;
    assert(calls==63); /*12 actor groups +51 original background row blocks. */
    cinematic_draw(guarded+1,0,362);assert(cinematic_inflate_count()-before==calls);
    /* Both output pages may change; cached state never relies on either. */
    memset(guarded+1,0,256*192*2);cinematic_draw(guarded+1,0,362);
    assert(hash()==cinema_expected_hashes[181]);
    /* Gameplay/audio borrows the region between scenes. Reset discards it. */
    memset(workspace+1,0x5a,128000);cinematic_reset();
    cinematic_draw(guarded+1,1,100);assert(hash()==cinema_expected_hashes[182+50]);
    assert(workspace[0]==0xa5&&workspace[128001]==0xa5);
    cinematic_reset();before=cinematic_inflate_count();
    for(unsigned t=0;t<cinematic_duration(1);t+=2){
        cinematic_draw(guarded+1,1,t);
        assert(hash()==cinema_expected_hashes[182+cinematic_frame(1,t)]);
    }
    assert(cinematic_inflate_count()-before==355); /*8 actor groups +347 narrow background row blocks. */
    /* A smaller loan alternates cropped caches and fallback safely, including
       seeks across camera windows that change the memory layout. */
    cinematic_set_workspace(workspace+1,100000);workspace[100001]=0x39;verify();
    assert(workspace[100001]==0x39);
    assert(workspace[0]==0xa5&&workspace[128001]==0xa5);
    cinematic_set_workspace(workspace+1,100);cinematic_draw(guarded+1,0,150);
    assert(hash()==cinema_expected_hashes[75]);
    cinematic_set_workspace(NULL,0);
    printf("Cached throne: %u inflates total across182 images; seeks, external reset, workspace guards and small-workspace fallback pass\n",calls);
    return 0;
}
