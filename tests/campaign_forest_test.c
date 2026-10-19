#include "campaign.h"
#include "campaign_forest.h"
#include "campaign_save.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* LEVEL18 Forest Entrance: wave order, cutscene, leashes, wire format and the world-map unlock. */
static uint16_t steer(const Fighter *p,int x,int y){return (x-p->x>3?RIGHT:x-p->x< -3?LEFT:0)|(y-p->y>3?DOWN:y-p->y< -3?UP:0);}
static unsigned living(const Campaign *c){unsigned n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=7;campaign_forest_start(c,a);
}
static void keys_for(const Campaign *c,Arena *a,unsigned t,uint16_t keys[2]){
    keys[0]=keys[1]=0;
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready)continue;
        const Fighter *q=0;int best=9999;
        for(int e=0;e<6;e++)if(c->enemies[e].ready&&c->enemies[e].hp>0){int d=abs(p->x-c->enemies[e].x)+abs(p->y-c->enemies[e].y);if(d<best){q=&c->enemies[e];best=d;}}
        if(q&&c->phase!=WAR_FOREST_CINEMA){keys[h]=steer(p,q->x+(q->x>p->x?-30:30),q->y);if(abs(p->y-q->y)<12&&abs(p->x-q->x)<55&&t%11==0)keys[h]|=ATTACK|ATTACK_EDGE;}
        else keys[h]=steer(p,p->x+100,p->x<1100?260:p->x<1900?285:294); /* Follow the road's floor band. */
        if(p->hp<60)p->hp=campaign_health(c,h);
    }
}
static void packet(const Campaign *c,const Arena *a,unsigned t){
    CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,t);
    if(!campaign_packet_valid(&p)){fprintf(stderr,"invalid forest packet t=%u phase=%u group=%u spawned=%u\n",t,c->phase,c->group,c->spawned);assert(0);}
    campaign_unpack(&p,&guest,&remote);
    if(remote.map_id!=a->map_id||guest.phase!=c->phase||guest.group!=c->group)fprintf(stderr,"roundtrip map=%u/%u phase=%u/%u group=%u/%u t=%u\n",remote.map_id,a->map_id,guest.phase,c->phase,guest.group,c->group,t);
    assert(remote.map_id==a->map_id&&guest.phase==c->phase&&guest.group==c->group);
}
static void full_run(int coop){
    Campaign c;Arena a;begin(&c,&a,coop);
    assert(a.map_id==FOREST_MAP&&c.phase==WAR_TRAVEL&&c.group==FOREST_WAVE_CROC&&a.p[0].x==176&&stage_position_open_map(FOREST_MAP,a.p[0].x,a.p[0].y));
    unsigned seen=0,max_ambush=0,wave_a=0,cinema_ticks=0,landed=0;int max_x_wave_a=0;
    for(unsigned t=0;t<30000&&a.map_id==FOREST_MAP;t++){
        uint16_t keys[2];keys_for(&c,&a,t,keys);
        unsigned phase=c.phase,group=c.group;
        if(phase==WAR_FIGHT&&group==FOREST_WAVE_CROC){seen|=1;if(living(&c)>wave_a)wave_a=living(&c);for(int h=0;h<2;h++)if(a.p[h].ready&&a.p[h].x>max_x_wave_a)max_x_wave_a=a.p[h].x;}
        if(phase==WAR_FOREST_CINEMA){seen|=2;cinema_ticks++;}
        if(phase==WAR_FIGHT&&group==FOREST_WAVE_AMBUSH){seen|=4;}
        if(phase==WAR_FIGHT&&group==FOREST_WAVE_ARCHERS)seen|=8;
        if(phase==WAR_FIGHT&&group==FOREST_WAVE_FINAL)seen|=16;
        campaign_step(&c,&a,keys);
        if(phase==WAR_FOREST_CINEMA&&c.phase==WAR_FOREST_CINEMA)for(int h=0;h<2;h++)if(a.p[h].ready)assert(a.p[h].invulnerable>0); /* Held and protected. */
        if(a.map_id==FOREST_MAP){
            unsigned barbs=0,airborne=0,beefy=0;for(int e=0;e<6;e++)if(c.enemies[e].ready&&c.enemies[e].hp>0){barbs+=c.enemies[e].knight_id==0;airborne+=c.enemies[e].z>0;beefy+=c.enemies[e].hp>=150;}
            if(barbs>max_ambush)max_ambush=barbs;
            if(airborne)landed|=1;
            if(beefy)landed|=2;
            if(t%7==0)packet(&c,&a,t);
        }
    }
    if(seen!=31||c.phase!=CAMPAIGN_WORLD_MAP||c.group!=5||!(c.completed&16))fprintf(stderr,"coop=%d seen=%u phase=%u group=%u completed=%llu x=%d\n",coop,seen,c.phase,c.group,(unsigned long long)c.completed,a.p[0].x);
    if(c.phase!=CAMPAIGN_WORLD_MAP){for(int e=0;e<6;e++)if(c.enemies[e].ready)fprintf(stderr,"e%d hp%d x%d y%d z%d pose%d inv%d sp%u\n",e,c.enemies[e].hp,c.enemies[e].x,c.enemies[e].y,c.enemies[e].z,c.enemies[e].pose,c.enemies[e].invulnerable,c.spawned);fprintf(stderr,"p0 %d,%d hp%d wp%d mm%u\n",a.p[0].x,a.p[0].y,a.p[0].hp,c.weapons[0],c.mount_mask);}
    assert(seen==31&&a.map_id==FOREST_MAP&&c.phase==CAMPAIGN_WORLD_MAP&&c.group==5&&(c.completed&16));
    assert(wave_a==(coop?5u:3u)&&max_x_wave_a<=628+3); /* Source leash WP0..WP2. */
    assert(cinema_ticks==FOREST_CINEMA_TICKS&&max_ambush>=3&&(landed&1)&&(landed&2));
    packet(&c,&a,1);
}
/* The crocodile outlives its rider; a hero jumps on it, rides, and jumps off again. */
static void mount_test(void){
    Campaign c,g;Arena a,r;begin(&c,&a,0);uint16_t none[2]={0,0};
    c.phase=WAR_FIGHT;c.group=FOREST_WAVE_CROC;c.timer=0;c.spawned=1;
    Fighter rider={.hp=50,.ready=1,.facing=-1,.knight_id=1,.x=600,.y=240};c.enemies[0]=rider;c.mount_mask=1;
    campaign_step(&c,&a,none);
    c.enemies[0].hp=0;campaign_step(&c,&a,none);
    int croc=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_CROC)croc=d;
    assert(croc>=0&&!(c.mount_mask&1));
    CampaignPacket p;campaign_pack(&p,&c,&a,1);assert(campaign_packet_valid(&p));
    a.p[0].x=c.drops[croc].x-10;a.p[0].y=c.drops[croc].y;a.p[0].z=0;
    uint16_t jump[2]={JUMP|JUMP_EDGE,0};
    for(int t=0;t<6;t++){campaign_step(&c,&a,none);}
    assert(!(c.mount_mask&64)&&a.p[0].z>0&&a.p[0].pose==POSE_JUMP); /* walking into the crocodile starts the leap onto it */
    for(int t=0;t<12;t++){campaign_step(&c,&a,none);}
    assert((c.mount_mask&64)&&a.p[0].z==0);
    int left=0;for(int d=0;d<CAMPAIGN_DROPS;d++)left+=c.drops[d].kind==DROP_CROC;assert(left==0);
    campaign_pack(&p,&c,&a,2);assert(campaign_packet_valid(&p));memset(&g,0,sizeof g);campaign_unpack(&p,&g,&r);assert(g.mount_mask&64);
    campaign_step(&c,&a,none);campaign_step(&c,&a,jump);
    assert(!(c.mount_mask&64));left=0;for(int d=0;d<CAMPAIGN_DROPS;d++)left+=c.drops[d].kind==DROP_CROC;assert(left==1);
    for(int t=0;t<30;t++){campaign_step(&c,&a,none);}
    assert(!(c.mount_mask&64)); /* a dismounted hero standing beside it does not climb straight back on */
    for(int t=0;t<60;t++){campaign_step(&c,&a,none);}
    assert(c.mount_mask&64);
    campaign_pack(&p,&c,&a,3);assert(campaign_packet_valid(&p));
}
int main(void){
    mount_test();
    full_run(0);full_run(1);
    /* The node unlocks only after the Barbarian Boss and travelling reaches it from the boss. */
    Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,0);
    c.completed=3;assert(!campaign_world_unlocked(&c,5));c.completed=4;assert(campaign_world_unlocked(&c,5));
    campaign_world_map(&c,&a);c.group=c.spawned=4;assert(campaign_world_choose(&c,5)&&campaign_world_travelling(&c));
    int near_junction=0;
    for(unsigned t=0;t<400&&campaign_world_travelling(&c);t++){
        campaign_world_tick(&c,&a);packet(&c,&a,t);
        int wx,wy;campaign_world_position(&c,&wx,&wy);if(abs(wx-593)<=4&&abs(wy-313)<=4)near_junction=1;
    }
    assert(!campaign_world_travelling(&c)&&c.group==5&&near_junction); /* The original path bends at the junction. */
    int fx,fy;campaign_world_position(&c,&fx,&fy);assert(fx==645&&fy==241);
    c.completed=7;assert(campaign_world_enter(&c,&a,5)&&a.map_id==FOREST_MAP&&c.phase==WAR_TRAVEL);
    /* The new clear bit survives the save format and the wire. */
    CampaignCharacter save;c.completed=23;campaign_character_get(&save,&c,0);assert(save.cleared==23&&campaign_character_valid(&save));
    Campaign restored={0};campaign_character_put(&save,&restored,0);assert(restored.completed==23);
    /* The Owlet orb sits at the road's end, is collected, and survives the wire and the save. */
    begin(&c,&a,0);int orb=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_PET&&(c.drops[d].value&31)==PET_OWLET)orb=d;
    assert(orb>=0);a.p[0].x=c.drops[orb].x;a.p[0].y=c.drops[orb].y;c.drop_age[orb]=0;
    {uint16_t none[2]={0,0};int ox=c.drops[orb].x,oy=c.drops[orb].y;for(int t=0;t<5;t++){a.p[0].x=ox;a.p[0].y=oy;campaign_step(&c,&a,none);}}
    assert(pets_owned(&c.pets[0],PET_OWLET)&&c.pets[0].equipped==PET_OWLET);
    {CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,&c,&a,3);assert(campaign_packet_valid(&p));memset(&guest,0,sizeof guest);campaign_sync_cycle(&c,&a,&guest,&remote,0);
     assert(guest.pets[0].equipped==PET_OWLET&&guest.profiles[0].level==c.profiles[0].level);}
    campaign_character_get(&save,&c,0);assert(save.pet==PET_OWLET&&campaign_character_valid(&save));
    begin(&c,&a,0);c.pets[0].owned=1u<<PET_OWLET;{int n=0;campaign_forest_start(&c,&a);for(int d=0;d<CAMPAIGN_DROPS;d++)n+=c.drops[d].kind==DROP_PET;assert(n==0);}
    /* Weapon 38 lies on the road; weapon 10 is a shovel secret. Both need level 5 and cross the wire and save. */
    begin(&c,&a,0);c.profiles[0].level=5;{
        int leaf=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_WEAPON&&c.drops[d].value==WEAPON_LEAF)leaf=d;
        assert(leaf>=0);uint16_t none[2]={0,0},act[2]={ATTACK|ATTACK_EDGE,0};
        a.p[0].x=c.drops[leaf].x;a.p[0].y=c.drops[leaf].y;campaign_step(&c,&a,act);campaign_step(&c,&a,none);
        assert(c.weapons[0]==WEAPON_LEAF&&weapon_set_has(&c.owned_weapons[0],WEAPON_LEAF));
        c.tools[0]=1;a.p[0].x=FOREST_DIG_X;a.p[0].y=FOREST_DIG_Y;c.dig_timer[0]=1;for(int t=0;t<90;t++){a.p[0].x=FOREST_DIG_X;a.p[0].y=FOREST_DIG_Y;campaign_step(&c,&a,none);}
        int thief=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_WEAPON&&c.drops[d].value==WEAPON_THIEF)thief=d;
        assert(thief>=0);a.p[0].x=c.drops[thief].x;a.p[0].y=c.drops[thief].y;campaign_step(&c,&a,act);campaign_step(&c,&a,none);
        assert(c.weapons[0]==WEAPON_THIEF&&weapon_set_has(&c.owned_weapons[0],WEAPON_LEAF)&&weapon_set_has(&c.owned_weapons[0],WEAPON_THIEF));
        CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,&c,&a,9);assert(campaign_packet_valid(&p));memset(&guest,0,sizeof guest);campaign_sync_cycle(&c,&a,&guest,&remote,0);
        assert(guest.weapons[0]==WEAPON_THIEF&&remote.p[0].hp==a.p[0].hp);
        c.weapons[0]=WEAPON_LEAF;campaign_sync_cycle(&c,&a,&guest,&remote,32);assert(guest.weapons[0]==WEAPON_LEAF);
        campaign_character_get(&save,&c,0);assert(weapon_set_equal(&save.owned,&c.owned_weapons[0])&&save.weapon==WEAPON_LEAF);Campaign back={0};campaign_character_put(&save,&back,0);assert(back.weapons[0]==WEAPON_LEAF&&weapon_set_equal(&back.owned_weapons[0],&c.owned_weapons[0]));
    }
    /* The first thief of the opening wave rides the crocodile; the flag crosses the wire and ends with its rider. */
    begin(&c,&a,0);{uint16_t none[2]={0,0};a.p[0].x=400;a.p[0].y=250;int seen=0;
     for(int t=0;t<400&&!seen;t++){a.p[0].x=400;a.p[0].y=250;a.p[0].hp=999;campaign_step(&c,&a,none);if(c.mount_mask)seen=1;}
     assert(seen&&(c.mount_mask&(c.mount_mask-1))==0);
     CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,&c,&a,4);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&remote);assert(guest.mount_mask==c.mount_mask);
     unsigned slot=0;while(!(c.mount_mask&(1u<<slot)))slot++;assert(c.enemies[slot].knight_id==1);
     c.enemies[slot].hp=0;campaign_step(&c,&a,none);assert(!c.mount_mask);}
    /* Owlet fetches the level's fruit tree fruit once and brings it to its owner. */
    begin(&c,&a,0);c.pets[0]=(PetProfile){1u<<PET_OWLET,PET_OWLET};
    {uint16_t none[2]={0,0};unsigned food=0;a.p[0].x=1650;a.p[0].y=250;
     for(int t=0;t<400;t++){a.p[0].x=1650;a.p[0].y=250;campaign_step(&c,&a,none);for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_FOOD)food++;if(food)break;}
     assert(c.fruit_taken&&food&&!c.owlet_state[0]);
     memset(c.drops,0,sizeof c.drops);food=0;
     for(int t=0;t<400;t++){a.p[0].x=1650;a.p[0].y=250;campaign_step(&c,&a,none);for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_FOOD)food++;}
     assert(!food); /* One tree, one fruit. */
     begin(&c,&a,0);c.pets[0]=(PetProfile){1u<<PET_OWLET,PET_OWLET};a.p[0].x=300;a.p[0].y=250;
     for(int t=0;t<400;t++){a.p[0].x=300;a.p[0].y=250;campaign_step(&c,&a,none);}
     assert(!c.fruit_taken); /* The tree must be on screen. */
    }
    puts("Forest Entrance: waves, ambush cutscene, leashes, co-op, wire packets, unlock, save bit and world-map exit pass");
    return 0;
}
