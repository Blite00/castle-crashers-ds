#include "pets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(void){
    _Static_assert(sizeof(PetDrop)==8,"compact caller-owned ground pickup");
    _Static_assert(sizeof(PetFollower)==8,"compact follower");
    PetProfile p[2]={{0}};PetFollower f[2]={{0}};PetDrop d={0};
    assert(pets_profile_valid(&p[0])&&!pets_owned(&p[0],PET_MR_BUDDY));
    assert(!pets_equip_owned(&p[0],&f[0],PET_MR_BUDDY,&d,100,200));
    assert(!pets_unearth_buddy(&d,0,1,155,286)&&!d.source_id);
    assert(!pets_unearth_buddy(&d,1,0,155,286)&&!d.source_id);
    assert(pets_unearth_buddy(&d,1,1,155,286)&&d.unlock&&d.source_id==24);
    PetDrop before=d;assert(!pets_place(&d,PET_SNOOT,0,0,1)&&!memcmp(&before,&d,sizeof d));
    assert(!pets_pickup(p,3,0,&f[0],&d,180,286)); /* strict source50/20 bounds */
    assert(!pets_pickup(p,3,0,&f[0],&d,155,296));
    assert(pets_pickup(p,3,0,&f[0],&d,155,286));
    assert(p[0].equipped==24&&!p[1].equipped&&pets_owned(&p[1],24)&&!d.source_id);
    assert(!pets_pickup(p,3,1,&f[1],&d,155,286)); /* one physical pickup */
    assert(f[0].x==155&&f[0].y==286&&f[0].love_timer==60);
    assert(pets_info(24)->faster_dig&&!pets_info(24)->strength&&pets_info(17)->strength==4);
    /* A new source unlock is not awarded to a dead/absent partner. */
    p[1]=(PetProfile){0};assert(pets_place(&d,17,155,286,1));
    assert(pets_pickup(p,1,0,&f[0],&d,155,286));assert(p[0].equipped==17&&!pets_owned(&p[1],17));
    assert(d.source_id==24&&!d.unlock&&d.ignore_timer==51);
    PetProfile saved[2];memcpy(saved,p,sizeof p);before=d;PetFollower oldf=f[0];
    assert(!pets_pickup(p,1,0,&f[0],&d,155,286));assert(!memcmp(saved,p,sizeof p)&&!memcmp(&before,&d,sizeof d)&&!memcmp(&oldf,&f[0],sizeof oldf));
    for(int i=0;i<50;i++)pets_drop_step(&d);
    assert(d.ignore_timer==1&&!pets_pickup(p,1,0,&f[0],&d,155,286));pets_drop_step(&d);
    assert(pets_pickup(p,1,0,&f[0],&d,155,286)&&p[0].equipped==24&&d.source_id==17);
    /* Ark/old-pet pickup is locked unless owned, even when standing on it. */
    for(int i=0;i<51;i++)pets_drop_step(&d);
    assert(!pets_pickup(p,3,1,&f[1],&d,155,286));
    memcpy(saved,p,sizeof p);before=d;oldf=f[0];
    assert(!pets_equip_owned(&p[0],&f[0],17,&d,155,286));
    assert(!memcmp(saved,p,sizeof p)&&!memcmp(&before,&d,sizeof d)&&!memcmp(&oldf,&f[0],sizeof oldf));
    assert(!pets_equip_owned(&p[0],&f[0],PET_NONE,NULL,155,286));
    d=(PetDrop){0};assert(pets_equip_owned(&p[0],&f[0],PET_NONE,&d,155,286));assert(!p[0].equipped&&d.source_id==24);
    /* Invalid save input is atomic and the format has no native padding. */
    uint8_t bytes[9];assert(pets_profile_encode(bytes,&p[0]));assert(bytes[0]==0&&bytes[1]==0&&bytes[2]==2&&bytes[3]==1&&!bytes[4]&&!bytes[5]&&!bytes[6]&&!bytes[7]&&bytes[8]==0);
    PetProfile restored={0};assert(pets_profile_decode(&restored,bytes)&&restored.owned==p[0].owned);
    bytes[0]=1;PetProfile previous=restored;assert(!pets_profile_decode(&restored,bytes)&&!memcmp(&previous,&restored,sizeof restored));
    bytes[0]=bytes[2]=bytes[3]=0;bytes[8]=24;assert(!pets_profile_decode(&restored,bytes));
    assert(!pets_info(0)&&!pets_info(255)&&!pets_owned(&p[0],255));
    /* Source follow goal25px behind, one-tenth approach; turn changes goal
       without teleporting. Fractional progress is deterministic. */
    PetFollower first={.x=0,.y=0,.love_timer=60,.facing=1},second=first;
    pets_follow_step(&first,125,100,1);assert(first.x==10&&first.y==10);
    first=second;unsigned rng=22;
    for(int i=0;i<5000;i++){
        rng=rng*1664525u+1013904223u;int x=(rng>>16)%2448,y=(rng>>24)%448,face=(rng&1)?1:-1;
        pets_follow_step(&first,x,y,face);pets_follow_step(&second,x,y,face);assert(!memcmp(&first,&second,sizeof first));
        assert(abs(first.fraction_x)<16&&abs(first.fraction_y)<16);
    }
    for(int i=0;i<200;i++)pets_follow_step(&first,125,100,1);
    assert(abs(first.x-100)<=2&&abs(first.y-100)<=2&&!first.love_timer);
    int x=first.x;pets_follow_step(&first,125,100,-1);assert(first.x>x&&first.x<150);
    puts("PASS: source pet IDs/stats, shovel-only Buddy, shared live-player unlock, atomic swaps/drops,51-tick reclaim delay, five-byte saves and deterministic following");
}
