#include "effects.h"
#include <assert.h>
#include <string.h>
int main(void) {
    static uint16_t buffer[256*192+2];
    Arena a;
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;
    buffer[0]=0x1234;buffer[256*192+1]=0x5678;
    for(int frame=0;frame<24;frame++) {
        a.tick=frame;
        for(int i=0;i<ARENA_SHOTS;i++)
            a.shots[i]=(Projectile){.x=i*55-10,.y=frame*9,.life=20,.owner=i&1,.vx=i&1?-5:5};
        for(int i=0;i<2;i++)
            a.p[i]=(Fighter){.ready=1,.x=i?255:0,.y=190,.facing=i?-1:1,.freeze=30,.burn=30,
                .attack_kind=HIT_SPLASH,.cooldown=6-frame%6};
        effects_render(buffer+1,&a);
        assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    }
    int drawn=0;
    for(int i=1;i<=256*192;i++)drawn+=buffer[i]!=0;
    assert(drawn>100);
    /* Holding the modifier displays readiness without launching any attack. */
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;a.p[0]=(Fighter){.ready=1,.x=128,.y=150,.hp=100,.facing=1,.previous=MAGIC};
    memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    drawn=0;for(int i=1;i<=256*192;i++)drawn+=buffer[i]!=0;assert(drawn>0);
    a.p[0].previous=0;memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    for(int i=1;i<=256*192;i++)assert(buffer[i]==0);
    assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    /* Original arrows must mirror exactly, remain static across ticks, and
       clip at either edge without touching adjacent memory. */
    static uint16_t right_arrow[256*192],left_arrow[256*192];
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;
    a.shots[0]=(Projectile){.x=128,.y=100,.life=10,.kind=SHOT_ARROW,.vx=10};
    effects_render(right_arrow,&a);
    a.shots[0].vx=-10;effects_render(left_arrow,&a);
    drawn=0;
    for(int y=0;y<192;y++)for(int x=1;x<256;x++) {
        assert(right_arrow[y*256+x]==left_arrow[y*256+256-x]);
        drawn+=right_arrow[y*256+x]!=0;
    }
    assert(drawn>0);
    a.tick=7;a.shots[0].vx=10;memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    assert(memcmp(buffer+1,right_arrow,sizeof right_arrow)==0);
    for(int owner=0;owner<2;owner++)for(int edge=0;edge<2;edge++) {
        a.shots[0].owner=owner;a.shots[0].x=edge?255:0;a.shots[0].vx=edge?-10:10;
        effects_render(buffer+1,&a);
        assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    }
    /* Fire impact plays once from hit age, never from the global clock. */
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;a.p[0]=(Fighter){.ready=1,.x=128,.y=140,.hp=100,.facing=1,.burn=ARENA_BURN_TICKS};
    memset(right_arrow,0,sizeof right_arrow);effects_render(right_arrow,&a);
    drawn=0;for(int i=0;i<256*192;i++)drawn+=right_arrow[i]!=0;
    assert(drawn>0&&drawn<6000);
    a.tick=117;memset(left_arrow,0,sizeof left_arrow);effects_render(left_arrow,&a);
    assert(!memcmp(right_arrow,left_arrow,sizeof right_arrow));
    for(int age=14;age<ARENA_BURN_TICKS;age++){
        a.p[0].burn=ARENA_BURN_TICKS-age;memset(left_arrow,0,sizeof left_arrow);effects_render(left_arrow,&a);
        for(int i=0;i<256*192;i++)assert(left_arrow[i]==0);
    }
    /* Arrows change pitch with vertical velocity, with mirrored silhouettes. */
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;a.shots[0]=(Projectile){.x=128,.y=100,.life=20,.kind=SHOT_ARROW,.vx=16,.vz=5};
    memset(right_arrow,0,sizeof right_arrow);effects_render(right_arrow,&a);
    a.shots[0].vz=-20;memset(left_arrow,0,sizeof left_arrow);effects_render(left_arrow,&a);
    assert(memcmp(right_arrow,left_arrow,sizeof right_arrow));
    a.shots[0].vx=-16;memset(buffer+1,0,sizeof right_arrow);effects_render(buffer+1,&a);
    for(int y=0;y<192;y++)for(int x=1;x<256;x++)assert(left_arrow[y*256+x]==buffer[1+y*256+256-x]);
    /* Equipping shows the authored popup above the head; its timer clears it. */
    memset(&a,0,sizeof a);a.p[0].ready=a.p[1].ready=1;a.p[0]=(Fighter){.ready=1,.x=128,.y=150,.hp=100,.bow_equipped=1,.equip_timer=30};
    memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    drawn=0;for(int y=60;y<110;y++)for(int x=100;x<156;x++)drawn+=buffer[1+y*256+x]!=0;
    assert(drawn>0);
    a.p[0].equip_timer=0;memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    for(int i=1;i<=256*192;i++)assert(buffer[i]==0);
    assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    /* An unjoined fighter has no status, readiness, item or projectile visuals. */
    memset(&a,0,sizeof a);a.p[0]=(Fighter){.hp=100,.x=128,.y=150,.previous=MAGIC,.burn=ARENA_BURN_TICKS,.freeze=30,.equip_timer=30,.bow_equipped=1};
    a.shots[0]=(Projectile){.life=20,.x=128,.y=100,.kind=SHOT_ARROW};
    memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
    for(int i=1;i<=256*192;i++)assert(buffer[i]==0);
    /* All four downward spell silhouettes differ from their horizontal art,
       and reflect horizontally without reversing their downward slope. */
    const int magic_kinds[]={SHOT_ICE,SHOT_FIRE,SHOT_LIGHTNING,SHOT_POISON};
    for(int k=0;k<4;k++){
        memset(&a,0,sizeof a);a.p[0].ready=1;
        a.shots[0]=(Projectile){.x=128,.y=100,.life=20,.kind=magic_kinds[k],.vx=13};
        memset(right_arrow,0,sizeof right_arrow);effects_render(right_arrow,&a);
        a.shots[0].vz=-9;memset(left_arrow,0,sizeof left_arrow);effects_render(left_arrow,&a);
        assert(memcmp(right_arrow,left_arrow,sizeof right_arrow));
        a.shots[0].vx=-13;memset(buffer+1,0,sizeof right_arrow);effects_render(buffer+1,&a);
        for(int y=0;y<192;y++)for(int x=1;x<256;x++)assert(left_arrow[y*256+x]==buffer[1+y*256+256-x]);
        assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    }
    /* Holding magic never cycles back through the source's blank endpoints. */
    for(unsigned knight=0;knight<4;knight++)for(unsigned t=0;t<90;t++){
        memset(&a,0,sizeof a);a.tick=t;a.p[0]=(Fighter){.ready=1,.hp=100,.x=128,.y=140,.knight_id=knight,.previous=MAGIC};
        memset(buffer+1,0,256*192*2);effects_render(buffer+1,&a);
        unsigned visible=0;for(int i=1;i<=256*192;i++)visible+=buffer[i]!=0;
        assert(visible>0);
    }
    return 0;
}
