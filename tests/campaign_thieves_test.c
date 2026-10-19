#include "campaign.h"
#include "campaign_thieves.h"
#include "campaign_thieves_scenes.h"
#include "campaign_breaks.h"
#include "campaign_save.h"
#include "species.h"
#include "pets.h"
#include "stage.h"
#include "../generated/thieves_geometry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* LEVEL4 Thieves' Forest: encounter order, leashes, species, wire format, unlock and completion. */
static uint16_t steer(const Fighter *p,int x,int y){return (x-p->x>3?RIGHT:x-p->x< -3?LEFT:0)|(y-p->y>3?DOWN:y-p->y< -3?UP:0);}
static unsigned living(const Campaign *c){unsigned n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=23;campaign_thieves_start(c,a);
}
/* Unit tests jump around the level, so they skip the intro clip. */
static void begin_skip(Campaign *c,Arena *a,int coop){begin(c,a,coop);c->spawned=0;c->timer=0;}
static void keys_for(const Campaign *c,Arena *a,unsigned t,uint16_t keys[2]){
    keys[0]=keys[1]=0;
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready)continue;
        const Fighter *q=0;int best=9999;
        for(int e=0;e<6;e++)if(c->enemies[e].ready&&c->enemies[e].hp>0){int d=abs(p->x-c->enemies[e].x)+abs(p->y-c->enemies[e].y);if(c->species[e]==SPECIES_TROLL_MOTHER)d-=400; /* Players go for the mother. */if(d<best){q=&c->enemies[e];best=d;}}
        if(q){keys[h]=steer(p,q->x+(q->x>p->x?-30:30),q->y);if(abs(p->y-q->y)<12&&abs(p->x-q->x)<55&&t%11==0)keys[h]|=ATTACK|ATTACK_EDGE;}
        else{int lo=0,hi=447;for(int k=0;k<=48;k+=8){int l=thieves_floor_min((unsigned)(p->x+k)),u=thieves_floor_max((unsigned)(p->x+k));if(l>lo)lo=l;if(u<hi)hi=u;}keys[h]=steer(p,p->x+100,(lo+hi)/2);if(campaign_thieves_wall_ahead(p->x)&&!p->z)keys[h]|=JUMP|JUMP_EDGE;} /* Look ahead so a step in the floor is taken before reaching it. */
        if(p->hp<60)p->hp=campaign_health(c,h);
    }
}
static void packet(const Campaign *c,const Arena *a,unsigned t){
    CampaignPacket p;campaign_pack(&p,c,a,t);
    if(!campaign_packet_valid(&p)){fprintf(stderr,"invalid thieves packet t=%u phase=%u group=%u spawned=%u%c",t,c->phase,c->group,c->spawned,10);for(int e=0;e<6;e++)if(c->enemies[e].ready)fprintf(stderr,"e%d x%d y%d z%d pose%d anim%d hp%d%c",e,c->enemies[e].x,c->enemies[e].y,c->enemies[e].z,c->enemies[e].pose,c->enemies[e].animation,c->enemies[e].hp,10);assert(0);}
}
static void full_run(int coop){
    Campaign c;Arena a;begin(&c,&a,coop);
    assert(a.map_id==THIEVES_MAP&&c.phase==WAR_TRAVEL&&c.group==0&&stage_position_open_map(THIEVES_MAP,a.p[0].x,a.p[0].y));
    unsigned events_seen=0,species_seen=0,max_living=0,boss_seen=0;int max_x=0;
    for(unsigned t=0;t<400000&&a.map_id==THIEVES_MAP;t++){
        uint16_t keys[2];keys_for(&c,&a,t,keys);
        if(c.phase==WAR_FIGHT)events_seen|=1u<<c.group;
        for(int e=0;e<6;e++)if(c.enemies[e].ready&&c.enemies[e].hp>0)species_seen|=1u<<c.species[e];
        if(living(&c)>max_living)max_living=living(&c);
        if(campaign_thieves_boss_hp(&c))boss_seen=1;
        campaign_step(&c,&a,keys);
        for(int h=0;h<2;h++)if(a.p[h].ready&&a.p[h].x>max_x)max_x=a.p[h].x;
        if(t%9==0)packet(&c,&a,t);
    }
    if(events_seen!=(1u<<THIEVES_EVENT_COUNT)-1||c.phase!=CAMPAIGN_WORLD_MAP||c.group!=6||!(c.completed&32))fprintf(stderr,"coop=%d events=%x species=%x phase=%u group=%u completed=%llu x=%d%c",coop,events_seen,species_seen,c.phase,c.group,(unsigned long long)c.completed,max_x,10);
    if(events_seen!=(1u<<THIEVES_EVENT_COUNT)-1||c.phase!=CAMPAIGN_WORLD_MAP){for(int e=0;e<6;e++)if(c.enemies[e].ready)fprintf(stderr," e%d sp%u hp%d x%d y%d z%d pose%d inv%d%c",e,c.species[e],c.enemies[e].hp,c.enemies[e].x,c.enemies[e].y,c.enemies[e].z,c.enemies[e].pose,c.enemies[e].invulnerable,10);fprintf(stderr,"p0 %d %d hp%d%c",a.p[0].x,a.p[0].y,a.p[0].hp,10);}
    assert(events_seen==(1u<<THIEVES_EVENT_COUNT)-1&&c.phase==CAMPAIGN_WORLD_MAP&&c.group==6&&(c.completed&32));
    assert((species_seen&(1u<<SPECIES_TROLL))&&(species_seen&(1u<<SPECIES_IMP))&&(species_seen&(1u<<SPECIES_TROLL_MOTHER))&&boss_seen);
}
static void scenes_test(void){
    Campaign k;Arena b;begin(&k,&b,0);uint16_t right[2]={RIGHT,0},none[2]={0,0};
    assert(thieves_scene_playing(&k)==THIEVES_SCENE_INTRO);for(int t=0;t<60;t++)campaign_step(&k,&b,right);
    assert(thieves_scene_frozen(&k)&&b.p[0].x==THIEVES_P1_START_X-(100-59)*2&&b.p[0].pose==POSE_WALK); /* The knights run in behind the carriers; the keys do nothing. */
    for(int t=0;t<120;t++)campaign_step(&k,&b,none);
    assert(thieves_scene_playing(&k)<0&&k.phase==WAR_TRAVEL&&abs(b.p[0].x-THIEVES_P1_START_X)<8);
    /* The owl clip starts when the heroes reach waypoint 2, holds the screen at waypoint 3 and shakes at its explosion frames. */
    b.p[0].x=campaign_thieves_waypoint_x(2)+5;campaign_step(&k,&b,none);
    assert(thieves_scene_playing(&k)==THIEVES_SCENE_OWL);int left,rightlimit;campaign_thieves_leash(&k,&left,&rightlimit);assert(rightlimit==campaign_thieves_waypoint_x(3));
    k.timer=101;assert(thieves_scene_shake(&k)==2);k.timer=104;assert(thieves_scene_shake(&k)==2);k.timer=120;assert(thieves_scene_shake(&k)==0);
    k.timer=thieves_scenes[THIEVES_SCENE_OWL].release-1;campaign_step(&k,&b,none);assert(thieves_scene_playing(&k)<0);
    CampaignPacket p;campaign_pack(&p,&k,&b,1);assert(campaign_packet_valid(&p));
    /* The bear clip freezes the heroes inside its cinema window only. */
    k.group=8;k.phase=WAR_TRAVEL;k.spawned=0;k.timer=0;for(int i=0;i<6;i++)memset(&k.enemies[i],0,sizeof k.enemies[i]);
    b.p[0].x=campaign_thieves_waypoint_x(17)+5;campaign_step(&k,&b,none);assert(thieves_scene_playing(&k)==THIEVES_SCENE_BEAR);
    k.timer=100;assert(thieves_scene_frozen(&k));k.timer=250;assert(!thieves_scene_frozen(&k));
}
/* Grass tufts and the three treasure chests can be cut and opened; chests give the two level weapons and a chicken. */
static void break_test(void){
    Campaign k;Arena b;begin_skip(&k,&b,0);uint16_t none[2]={0,0},hit[2]={ATTACK|ATTACK_EDGE,0};
    unsigned n;const BreakPoint *t=campaign_break_table(THIEVES_MAP,&n);assert(n==BREAK_THIEVES_COUNT);
    int grass=-1,chest[3]={-1,-1,-1},c=0;
    for(unsigned i=0;i<n;i++){if(t[i].kind){chest[c++]=(int)i;}else if(grass<0)grass=(int)i;}
    assert(c==3&&grass>=0);
    for(int tuft=0;tuft<2;tuft++){
        int i=tuft?chest[0]:grass;int swings=tuft?3:1;
        b.p[0].x=t[i].x-25;b.p[0].y=t[i].y-6;b.p[0].facing=1;b.p[0].z=0;
        while(!stage_position_open_map(THIEVES_MAP,b.p[0].x,b.p[0].y))b.p[0].y--;
        for(int s=0;s<swings;s++){for(int tick=0;tick<30;tick++){campaign_step(&k,&b,tick==0?hit:none);b.p[0].x=t[i].x-25;b.p[0].y=t[i].y-6;while(!stage_position_open_map(THIEVES_MAP,b.p[0].x,b.p[0].y))b.p[0].y--;b.p[0].hp=500;}}
        assert(campaign_break_cut(&k,(unsigned)i));
        CampaignPacket p;Campaign g={0};Arena r;campaign_pack(&p,&k,&b,(uint32_t)i);assert(campaign_packet_valid(&p));campaign_unpack(&p,&g,&r);assert(campaign_break_cut(&g,(unsigned)i));
    }
    int weapon=0;for(int d=0;d<CAMPAIGN_DROPS;d++)weapon|=k.drops[d].kind==DROP_WEAPON&&k.drops[d].value==WEAPON_37;
    assert(weapon);
}
int main(void){
    scenes_test();
    break_test();
    full_run(0);full_run(1);
    /* The Forest Entrance link bends through the original path points. */
    {Campaign w={0};Arena wa;arena_init(&wa);campaign_start(&w,&wa,0);campaign_world_map(&w,&wa);w.completed=31;w.group=w.spawned=5;
     assert(campaign_world_choose(&w,6)&&campaign_world_travelling(&w));int hit1=0,hit2=0;
     for(unsigned t=0;t<600&&campaign_world_travelling(&w);t++){campaign_world_tick(&w,&wa);int x,y;campaign_world_position(&w,&x,&y);
      if(abs(x-628)<=4&&abs(y-220)<=4)hit1=1;
      if(abs(x-611)<=4&&abs(y-202)<=4)hit2=1;}
     assert(!campaign_world_travelling(&w)&&hit1&&hit2);}
    /* Level pickups: the Troll and Dino orbs, weapon 30 and the coin bags appear near the heroes and are collected once. */
    {Campaign k;Arena b;begin_skip(&k,&b,0);uint16_t none[2]={0,0},act[2]={ATTACK|ATTACK_EDGE,0};
     k.group=THIEVES_EVENT_COUNT-1;k.profiles[0].level=1;b.p[0].x=THIEVES_TROLLPET_X-120;b.p[0].y=THIEVES_TROLLPET_Y;campaign_step(&k,&b,none);
     int orb=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(k.drops[d].kind==DROP_PET&&(k.drops[d].value&31)==PET_TROLL)orb=d;assert(orb>=0);
     b.p[0].x=THIEVES_TROLLPET_X;b.p[0].y=THIEVES_TROLLPET_Y;k.drop_age[orb]=0;for(int t=0;t<4;t++){b.p[0].x=THIEVES_TROLLPET_X;b.p[0].y=THIEVES_TROLLPET_Y;campaign_step(&k,&b,none);}
     assert(pets_owned(&k.pets[0],PET_TROLL)&&k.pets[0].equipped==PET_TROLL);
     /* The Troll heals one percent of maximum health every 300 ticks. */
     int maximum=campaign_health(&k,0);b.p[0].hp=maximum-20;for(int t=0;t<305;t++){b.p[0].x=THIEVES_TROLLPET_X;b.p[0].y=THIEVES_TROLLPET_Y;campaign_step(&k,&b,none);}
     assert(b.p[0].hp>maximum-20);
     b.p[0].x=THIEVES_WEAPON_PT_X-120;b.p[0].y=THIEVES_WEAPON_PT_Y;campaign_step(&k,&b,none);
     int w=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(k.drops[d].kind==DROP_WEAPON&&k.drops[d].value==WEAPON_TROLL)w=d;if(w<0){for(int d=0;d<CAMPAIGN_DROPS;d++)fprintf(stderr,"drop%d kind%u value%u x%d y%d%c",d,k.drops[d].kind,k.drops[d].value,k.drops[d].x,k.drops[d].y,10);fprintf(stderr,"hero x%d owned%d%c",b.p[0].x,weapon_set_has(&k.owned_weapons[0],WEAPON_TROLL),10);}
     assert(w>=0);
     b.p[0].x=THIEVES_WEAPON_PT_X;b.p[0].y=THIEVES_WEAPON_PT_Y;campaign_step(&k,&b,act);campaign_step(&k,&b,none);
     assert(k.weapons[0]==WEAPON_TROLL&&weapon_set_has(&k.owned_weapons[0],WEAPON_TROLL));
     Campaign guest={0};Arena remote;campaign_sync_cycle(&k,&b,&guest,&remote,0);assert(guest.weapons[0]==WEAPON_TROLL&&guest.pets[0].equipped==PET_TROLL);
     unsigned gold=k.profiles[0].gold;int bx,by;assert(campaign_thieves_bag(&k,2,&bx,&by));
     b.p[0].x=bx;b.p[0].y=by;campaign_step(&k,&b,none);assert(k.profiles[0].gold==gold+5&&!campaign_thieves_bag(&k,2,&bx,&by));
     campaign_step(&k,&b,none);assert(k.profiles[0].gold==gold+5);}
    /* Panicked thieves bolt left; imps cover ground faster than thieves. */
    {Campaign k;Arena b;begin_skip(&k,&b,0);uint16_t none[2]={0,0};
     k.group=10;b.p[0].x=thieves_waypoints[22][0];b.p[0].y=thieves_waypoints[22][1];campaign_step(&k,&b,none);campaign_step(&k,&b,none);
     assert(k.phase==WAR_FIGHT&&k.war.runner_mask);int slot=0;while(!(k.war.runner_mask&(1u<<slot)))slot++;int x0=k.enemies[slot].x;
     for(int t=0;t<20;t++){campaign_step(&k,&b,none);}
     assert(k.enemies[slot].x<x0);}
    /* The mother tosses out trolls, charges every 20 damage and takes her brood with her. */
    {Campaign k;Arena b;begin_skip(&k,&b,0);uint16_t none[2]={0,0};
     k.group=11;b.p[0].x=thieves_waypoints[23][0];b.p[0].y=thieves_waypoints[23][1];for(int t=0;t<3;t++)campaign_step(&k,&b,none);
     int m=-1;for(int i=0;i<6;i++)if(k.species[i]==SPECIES_TROLL_MOTHER)m=i;assert(m>=0&&k.enemies[m].hp==THIEVES_MOTHER_HEALTH&&campaign_thieves_boss_hp(&k)==THIEVES_MOTHER_HEALTH);
     unsigned trolls=0;for(int t=0;t<130;t++){b.p[0].hp=500;campaign_step(&k,&b,none);}
     for(int i=0;i<6;i++){trolls+=k.species[i]==SPECIES_TROLL&&k.enemies[i].ready;}
     assert(trolls>=2);
     k.enemies[m].hp=300;int mx=k.enemies[m].x;campaign_step(&k,&b,none);assert(k.war.gate_hp>0&&k.enemies[m].invulnerable); /* A wind-up warns first, standing still. */
     mx=k.enemies[m].x;for(int t=0;t<23;t++){campaign_step(&k,&b,none);assert(k.enemies[m].x==mx);}
     campaign_step(&k,&b,none);
     assert(k.war.shot_timer>0);
     for(int t=0;t<20;t++){campaign_step(&k,&b,none);}
     assert(abs(k.enemies[m].x-mx)>40);
     k.enemies[m].hp=0;campaign_step(&k,&b,none);campaign_step(&k,&b,none);
     for(int i=0;i<6;i++){if(i!=m)assert(!k.enemies[i].ready||k.enemies[i].hp<=0);}}
    /* The node unlocks only after Forest Entrance and travel reaches it from there. */
    Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,0);
    c.completed=15;assert(!campaign_world_unlocked(&c,6));c.completed=31;assert(campaign_world_unlocked(&c,6));
    campaign_world_map(&c,&a);c.group=c.spawned=5;assert(campaign_world_choose(&c,6)&&campaign_world_travelling(&c));
    for(unsigned t=0;t<400&&campaign_world_travelling(&c);t++){campaign_world_tick(&c,&a);packet(&c,&a,t);}
    assert(!campaign_world_travelling(&c)&&c.group==6);
    assert(campaign_world_enter(&c,&a,6)&&a.map_id==THIEVES_MAP&&c.phase==WAR_TRAVEL);
    /* The new clear bit survives the save format. */
    CampaignCharacter save;c.completed=63;campaign_character_get(&save,&c,0);assert(save.cleared==63&&campaign_character_valid(&save));
    puts("Thieves' Forest: events, species, boss, leashes, co-op, wire packets, unlock, save bit and world-map exit pass");
    return 0;
}
