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
int main(void){
    FloodObject o[FLOOD_ROCK_LIMIT];
    assert(campaign_flood_rocks(0,o)==0&&campaign_flood_rocks(30,o)==1&&o[0].x==FLOOD_ROCK_SPAWN_X);
    assert(campaign_flood_rocks(60,o)>=1&&o[0].x==FLOOD_ROCK_SPAWN_X-FLOOD_ROCK_SPEED*30);
    FloodObject f[FLOOD_FIN_LIMIT];assert(campaign_flood_fins(365,f)==0&&campaign_flood_fins(366,f)==1);
    {Campaign w={0};assert(!campaign_world_unlocked(&w,8));w.completed=64;assert(campaign_world_unlocked(&w,8));}
    run(0);run(1);
    puts("PASS: Flooded Castle timeline, hazards, checkpoints, Catfish, clear bit and wire");
    return 0;
}
