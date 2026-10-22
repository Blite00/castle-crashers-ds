#include "campaign.h"
#include "campaign_flood.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* LEVEL23 Flooded Castle: spawn, hazards as pure functions of the timer, checkpoints, boss, clear bit, wire. */
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=63|64;campaign_flood_start(c,a);
}
static void packet(const Campaign *c,const Arena *a){
    CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,1);assert(campaign_packet_valid(&p));
    campaign_unpack(&p,&guest,&remote);assert(remote.map_id==FLOOD_MAP&&guest.timer==c->timer&&guest.group==c->group);
    FloodObject r1[FLOOD_ROCK_LIMIT],r2[FLOOD_ROCK_LIMIT];unsigned n=campaign_flood_rocks(c->timer,r1);assert(n==campaign_flood_rocks(guest.timer,r2)&&!memcmp(r1,r2,n*sizeof *r1));
}
static void run(int coop){
    Campaign c;Arena a;begin(&c,&a,coop);
    assert(a.map_id==FLOOD_MAP&&c.phase==WAR_FIGHT&&stage_position_open_map(FLOOD_MAP,a.p[0].x,a.p[0].y));
    uint16_t none[2]={0,0};unsigned checkpoints=0,maxlive=0;
    for(unsigned t=0;t<6000&&c.phase!=WAR_COMPLETE;t++){
        a.tick++;campaign_step(&c,&a,none);
        unsigned live=0;for(int i=0;i<6;i++)live+=c.enemies[i].ready&&c.enemies[i].hp>0;if(live>maxlive)maxlive=live;
        /* a stand-in for the players: wipe whatever is alive every 40 ticks, keeping the heroes healthy */
        if(t%40==39){for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.enemies[i].hp>0){c.enemies[i].hp=0;c.enemies[i].pose=POSE_DEAD;c.kills++;}checkpoints++;}
        for(int h=0;h<2;h++)if(a.p[h].ready&&a.p[h].hp<60)a.p[h].hp=120;
        if(t%97==0)packet(&c,&a);
        for(int h=0;h<2;h++)if(a.p[h].ready)assert(stage_position_open_map(FLOOD_MAP,a.p[h].x,a.p[h].y));
    }
    assert(c.phase==WAR_COMPLETE&&(c.completed&128)&&c.group==1&&maxlive>0);
    (void)checkpoints;
}
static void fish_charge(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    a.p[0].z=0;
    c.enemies[0]=(Fighter){.ready=1,.hp=60,.knight_id=2,.x=600,.y=a.p[0].y,.facing=-1};c.species[0]=FLOOD_FISH;
    for(int i=0;i<5;i++){c.enemies[0].hit_seq++;a.tick++;campaign_step(&c,&a,none);}
    int x0=c.enemies[0].x;for(int i=0;i<10;i++){a.tick++;campaign_step(&c,&a,none);}
    assert(c.enemies[0].x<=x0-30&&c.enemies[0].pose==POSE_HEAVY);
}
static void catfish_stun(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    c.enemies[0]=(Fighter){.ready=1,.hp=600,.knight_id=2,.x=560,.y=FLOOD_KINGBOAT_Y,.facing=-1};c.species[0]=FLOOD_CAT;c.flood.mode[0]=CAT_SPIT;c.group=1;
    a.p[0].x=330;a.p[0].y=300;a.p[0].hp=1000;
    unsigned stunned=0,seen_ball=0;for(int i=0;i<300;i++){c.enemies[0].y=FLOOD_KINGBOAT_Y;c.enemies[0].x=560;a.tick++;campaign_step(&c,&a,none);if(c.flood.stun)stunned++;if(c.war.machine.ready){seen_ball++;packet(&c,&a);}}
    assert(stunned>=100&&stunned<=125&&seen_ball>20);
    /* out of the boat's lane the ball misses */
    begin(&c,&a,0);c.enemies[0]=(Fighter){.ready=1,.hp=600,.knight_id=2,.x=560,.y=FLOOD_KINGBOAT_Y+40,.facing=-1};c.species[0]=FLOOD_CAT;c.flood.mode[0]=CAT_SPIT;c.group=1;
    a.p[0].x=330;a.p[0].y=300;a.p[0].hp=1000;stunned=0;
    for(int i=0;i<200;i++){c.enemies[0].y=FLOOD_KINGBOAT_Y+40;c.enemies[0].x=560;a.tick++;campaign_step(&c,&a,none);if(c.flood.stun)stunned++;}
    assert(!stunned);
}
static void bat_grab(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    a.p[0].z=0;a.p[0].pose=POSE_IDLE;a.p[0].hp=100;
    c.enemies[0]=(Fighter){.ready=1,.hp=50,.knight_id=2,.x=a.p[0].x+4,.y=a.p[0].y,.z=20,.facing=-1};c.species[0]=FLOOD_BAT;c.flood.mode[0]=1;c.flood.tx[0]=a.p[0].x+5;c.flood.ty[0]=a.p[0].y;c.flood.aux[0]=20;
    for(int i=0;i<10&&!c.flood.grab[0];i++){a.tick++;campaign_step(&c,&a,none);}
    a.tick++;campaign_step(&c,&a,none);
    assert(c.flood.grab[0]&&a.p[0].pose==POSE_DIG&&!c.enemies[0].ready);
    packet(&c,&a);
    int x=a.p[0].x;
    for(int i=0;i<8;i++){uint16_t keys[2]={RIGHT,0};a.tick++;campaign_step(&c,&a,keys);}
    assert(a.p[0].x==x&&c.flood.grab[0]); /* held: walking does nothing */
    for(int press=0;press<8&&c.flood.grab[0];press++){uint16_t keys[2]={ATTACK|ATTACK_EDGE,0};a.tick++;campaign_step(&c,&a,keys);a.tick++;campaign_step(&c,&a,none);}
    assert(!c.flood.grab[0]&&a.p[0].hp>0);
}
static unsigned log_count(const Campaign *c){unsigned n=0;for(int d=0;d<CAMPAIGN_DROPS;d++)n+=c->drops[d].kind==DROP_CROC;return n;}
static void log_ride(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    assert(log_count(&c)==4);
    a.p[0].z=0;a.p[0].vz=0;a.p[0].pose=POSE_IDLE;
    int lx=0,ly=0;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_CROC){lx=c.drops[d].x;ly=c.drops[d].y;break;}
    a.p[0].x=lx-20;a.p[0].y=ly;
    for(int i=0;i<30;i++){a.tick++;campaign_step(&c,&a,none);if(i<10){a.p[0].x=lx-20;a.p[0].y=ly;}} /* standing next to it does nothing */
    assert(!(c.mount_mask&64)&&log_count(&c)==4);
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_CROC){lx=c.drops[d].x;ly=c.drops[d].y;break;}
    a.p[0].x=lx-20;a.p[0].y=ly;a.p[0].z=0;
    uint16_t jump[2]={JUMP|JUMP_EDGE,0};a.tick++;campaign_step(&c,&a,jump);
    for(int i=0;i<16;i++){a.tick++;campaign_step(&c,&a,none);}
    assert((c.mount_mask&64)&&log_count(&c)==3);
    packet(&c,&a);
    /* a jump drops the rider back into the water and the log stays */
    a.tick++;campaign_step(&c,&a,jump);
    assert(!(c.mount_mask&64)&&log_count(&c)==4);
}
int main(void){
    FloodObject o[FLOOD_ROCK_LIMIT];
    assert(campaign_flood_rocks(0,o)==0&&campaign_flood_rocks(30,o)==1&&o[0].x==FLOOD_ROCK_SPAWN_X);
    assert(campaign_flood_rocks(60,o)>=1&&o[0].x==FLOOD_ROCK_SPAWN_X-FLOOD_ROCK_SPEED*30);
    FloodObject f[FLOOD_FIN_LIMIT];assert(campaign_flood_fins(365,f)==0&&campaign_flood_fins(366,f)==1);
    {Campaign w={0};assert(!campaign_world_unlocked(&w,8));w.completed=64;assert(campaign_world_unlocked(&w,8));}
    fish_charge();catfish_stun();bat_grab();log_ride();run(0);run(1);
    puts("PASS: Flooded Castle timeline, hazards, checkpoints, Catfish, clear bit and wire");
    return 0;
}
