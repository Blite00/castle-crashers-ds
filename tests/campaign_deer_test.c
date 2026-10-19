#include "campaign.h"
#include "campaign_deer.h"
#include "campaign_save.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* LEVEL22 Mill Run: scripted intro, deterministic chase (guide, monster, logs), slams, mashes, dock, wire format, world-map unlock. */
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=31|32;campaign_deer_start(c,a);
}
static void packet(const Campaign *c,const Arena *a,unsigned t){
    CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,t);
    if(!campaign_packet_valid(&p)){fprintf(stderr,"invalid deer packet t=%u stage=%u timer=%u hero0 pose=%d anim=%d x=%d y=%d z=%d\n",t,c->group,c->timer,a->p[0].pose,a->p[0].animation,a->p[0].x,a->p[0].y,a->p[0].z);assert(0);}
    campaign_unpack(&p,&guest,&remote);
    assert(remote.map_id==DEER_MAP&&guest.phase==c->phase&&guest.timer==c->timer&&guest.group==c->group);
    assert(remote.p[0].x==a->p[0].x&&remote.p[0].pose==a->p[0].pose&&remote.p[0].animation==a->p[0].animation);
    assert(campaign_deer_guide(&guest)==campaign_deer_guide(c)); /* the guest derives the whole scene from the timer */
}
/* A player: jump piles, steer into the gap of walls, hold the pace at the camera centre. */
static uint16_t player(const Campaign *c,const Arena *a,unsigned h,int skill){
    const Fighter *p=&a->p[h];uint16_t keys=0;if(!skill)return 0;
    int guide=campaign_deer_guide(c);int rel=p->x-guide;
    if(rel<DEER_HERO_OFFSET-6)keys|=RIGHT;else if(rel>DEER_HERO_OFFSET+6)keys|=LEFT;
    DeerLog logs[DEER_LOG_LIMIT];unsigned n=campaign_deer_logs(c,logs);
    int want_y=215;
    for(unsigned i=0;i<n;i++){
        int dist=logs[i].x-p->x;if(dist<-50||dist>110)continue;
        int top,bottom;
        if(campaign_deer_gap(logs[i].kind,&top,&bottom))want_y=(top+bottom)/2;
        else if(dist<60&&dist>0&&!p->z)keys|=JUMP|JUMP_EDGE;
    }
    if(p->y<want_y-3)keys|=DOWN;else if(p->y>want_y+3)keys|=UP;
    return keys;
}
static unsigned run(int coop,int skill,unsigned *slams,unsigned *mashes){
    Campaign c;Arena a;begin(&c,&a,coop);
    assert(a.map_id==DEER_MAP&&c.phase==WAR_TRAVEL&&c.group==DEER_STAGE_INTRO&&stage_position_open_map(DEER_MAP,a.p[0].x,a.p[0].y));
    unsigned t,stage_seen=0;int hp0=a.p[0].hp;*slams=*mashes=0;
    for(t=0;t<20000&&c.phase!=CAMPAIGN_WORLD_MAP;t++){
        uint16_t keys[2]={player(&c,&a,0,skill),player(&c,&a,1,skill)};
        if(t<DEER_INTRO_TICKS)keys[0]=keys[1]=0;
        int hp_before[2]={a.p[0].hp,a.p[1].hp};
        campaign_step(&c,&a,keys);
        stage_seen|=1u<<c.group;
        for(int h=0;h<2;h++){if(a.p[h].pose==DEER_POSE_SLAM&&a.p[h].animation==0)(*slams)++;if(a.p[h].hp<hp_before[h]){(*mashes)++;assert(a.p[h].hp==hp_before[h]-10||!a.p[h].hp);}}
        if(c.phase!=CAMPAIGN_WORLD_MAP&&t%5==0)packet(&c,&a,t);
        if(c.phase!=CAMPAIGN_WORLD_MAP){
            for(int h=0;h<2;h++)if(a.p[h].ready){assert(stage_position_open_map(DEER_MAP,a.p[h].x,a.p[h].y));assert(a.p[h].z>=0&&a.p[h].z<=80);}
            if(a.winner==2)break;
        }
        if(!skill){for(int h=0;h<2;h++)if(a.p[h].hp<20)a.p[h].hp=campaign_health(&c,h);} /* the idle party only tests the timeline */
    }
    (void)hp0;
    if(getenv("DEER_DEBUG"))fprintf(stderr,"skill %d coop %d: t=%u stage_seen=%x slams=%u mashes=%u hp=%d/%d winner=%d phase=%u\n",skill,coop,t,stage_seen,*slams,*mashes,a.p[0].hp,a.p[1].hp,a.winner,c.phase);
    if(skill==2)assert((stage_seen&15)==15);
    return t;
}
int main(void){
    /* Deterministic tables: stage boundaries are ordered and the waves take a believable time. */
    Campaign c;Arena a;begin(&c,&a,1);
    unsigned last=0,boundaries=0;
    for(unsigned t=0;t<6000;t++){c.timer=t;unsigned s=campaign_deer_stage(&c);assert(s>=last);if(s!=last)boundaries++;last=s;}
    assert(last==DEER_STAGE_DOCK&&boundaries==3);
    /* The guide stays inside the loop tile during the chase, advances through the flume afterwards and ends at the dock. */
    for(unsigned t=DEER_INTRO_TICKS;t<6000;t++){c.timer=t;int g=campaign_deer_guide(&c);assert(g>=DEER_GUIDE_START&&g<DEER_FLUME_END+400);if(campaign_deer_stage(&c)==DEER_STAGE_CHASE)assert(g<DEER_GUIDE_START+DEER_LOOP_PERIOD);}
    /* The guide is continuous but for the loop wrap (exactly one loop period back). */
    int jumps=0;for(unsigned t=DEER_INTRO_TICKS+1;t<3000;t++){c.timer=t-1;int g0=campaign_deer_guide(&c);c.timer=t;int g1=campaign_deer_guide(&c);if(g1<g0){assert(g0-g1>DEER_LOOP_PERIOD-12&&g0-g1<=DEER_LOOP_PERIOD+12);jumps++;}else assert(g1-g0<=10);}
    assert(jumps>=3);
    /* Logs spawn off screen right and only leave through the left. */
    c.timer=DEER_INTRO_TICKS+31;DeerLog logs[DEER_LOG_LIMIT];assert(campaign_deer_logs(&c,logs)==0||logs[0].x-campaign_deer_guide(&c)>250);
    unsigned seen_kinds=0;for(unsigned t=DEER_INTRO_TICKS;t<DEER_INTRO_TICKS+4000;t++){c.timer=t;unsigned n=campaign_deer_logs(&c,logs);for(unsigned i=0;i<n;i++){assert(logs[i].kind>=2&&logs[i].kind<=12);seen_kinds|=1u<<logs[i].kind;}}
    assert(seen_kinds==0x1dfcu && "every log kind of the sequence appears (kind 9 is never used by sprite 141)");
    /* Monster: hidden until frame 196, drawn at the entrance, gone after the finale starts. */
    int mx;c.timer=100;assert(campaign_deer_monster_key(&c,&mx)<0);c.timer=250;assert(campaign_deer_monster_key(&c,&mx)>=0);c.timer=DEER_INTRO_TICKS+200;assert(campaign_deer_monster_key(&c,&mx)==11&&mx==campaign_deer_guide(&c)-48);
    /* Wire: heroes stay valid for the whole level, with and without a second player. */
    unsigned slams,mashes;
    unsigned ticks=run(1,2,&slams,&mashes);
    assert(ticks<20000);
    run(0,2,&slams,&mashes);
    /* A party that does nothing is slammed into the monster again and again and still reaches the end. */
    unsigned idle_slams,idle_mashes;run(1,0,&idle_slams,&idle_mashes);
    assert(idle_slams>0&&idle_mashes>0);
    /* A player who dodges takes far fewer hits than an idle one. */
    run(1,2,&slams,&mashes);assert(mashes<idle_mashes);
    /* World map: the node unlocks after Thieves' Forest and completing the level sets bit 64. */
    memset(&c,0,sizeof c);assert(!campaign_world_unlocked(&c,7));c.completed=32;assert(campaign_world_unlocked(&c,7));
    begin(&c,&a,1);c.completed=63;c.timer=0;
    for(unsigned t=0;t<20000&&c.phase!=CAMPAIGN_WORLD_MAP;t++){uint16_t k[2]={player(&c,&a,0,2),player(&c,&a,1,2)};if(t<DEER_INTRO_TICKS)k[0]=k[1]=0;a.p[0].hp=a.p[1].hp=campaign_health(&c,0);campaign_step(&c,&a,k);}
    assert(c.phase==CAMPAIGN_WORLD_MAP&&c.group==7&&(c.completed&64));
    /* Score tally: the level ends on a sign that lasts CAMPAIGN_TALLY_TICKS, with each hero's kills, gold and xp relayed to the guest. */
    begin(&c,&a,1);c.profiles[0].gold=100;c.profiles[0].xp=500;c.tally_gold0[0]=100;c.tally_xp0[0]=500;
    c.profiles[0].gold+=37;c.profiles[0].xp+=21;c.level_kills[0]=9;c.level_kills[1]=300;
    c.phase=WAR_COMPLETE;c.completed|=64;
    unsigned values[3];campaign_tally_values(&c,0,values);assert(values[0]==9&&values[1]==37&&values[2]==21);
    CampaignPacket wire;Campaign guest={0};Arena remote;campaign_pack(&wire,&c,&a,5);assert(campaign_packet_valid(&wire));
    guest.tally_valid=1;guest.tally_gold0[0]=100;guest.tally_xp0[0]=500;guest.tally_gold0[1]=c.profiles[1].gold;guest.tally_xp0[1]=c.profiles[1].xp;guest.phase=DEER_STAGE_DOCK;
    campaign_unpack(&wire,&guest,&remote);campaign_tally_values(&guest,0,values);assert(values[0]==9&&values[1]==37&&values[2]==21);campaign_tally_values(&guest,1,values);assert(values[0]==300);
    unsigned held=0;uint16_t none[2]={0,0};for(unsigned t=0;t<400&&c.phase!=CAMPAIGN_WORLD_MAP;t++){campaign_step(&c,&a,none);held++;}
    assert(held==CAMPAIGN_TALLY_TICKS&&c.phase==CAMPAIGN_WORLD_MAP);
    begin(&c,&a,1);c.phase=WAR_COMPLETE;held=0;for(unsigned t=0;t<400&&c.phase!=CAMPAIGN_WORLD_MAP;t++){uint16_t press[2]={t>=160?ATTACK_EDGE:0,0};campaign_step(&c,&a,press);held++;}
    assert(held<=161&&c.phase==CAMPAIGN_WORLD_MAP); /* A press skips the sign after five seconds. */
    /* One deer in three poops for the whole ride when there are two players, never in solo play, and the flag reaches the guest. */
    unsigned poop=0;
    for(unsigned i=0;i<60;i++){begin(&c,&a,1);c.rng=i*7919u+1;c.story_flags=0;campaign_deer_start(&c,&a);poop+=!!(c.story_flags&DEER_STORY_POOP);}
    assert(poop>=10&&poop<=35);
    for(unsigned i=0;i<20;i++){begin(&c,&a,0);c.rng=i*104729u+3;c.story_flags=0;campaign_deer_start(&c,&a);assert(!(c.story_flags&DEER_STORY_POOP));}
    begin(&c,&a,1);c.story_flags=DEER_STORY_POOP;{CampaignPacket wire;Campaign guest={0};Arena remote;campaign_pack(&wire,&c,&a,9);assert(campaign_packet_valid(&wire));campaign_unpack(&wire,&guest,&remote);assert(guest.story_flags&DEER_STORY_POOP);}
    puts("campaign_deer ok");
    return 0;
}
