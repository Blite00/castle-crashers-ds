#include "campaign.h"
#include "campaign_camp.h"
#include "stage.h"
#include "species.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* LEVEL20: start, encounters at their waypoints, the boss, the ladder to the top deck, the exit door, clear bit and wire. */
#define LADDER_X 5099
static uint16_t steer(const Fighter *p,int x,int y){return (x-p->x>3?RIGHT:x-p->x< -3?LEFT:0)|(y-p->y>3?DOWN:y-p->y< -3?UP:0);}
static int band_mid(int x,int y){
    if(!camp_floor_open(x,y)){for(int d=1;d<60;d++){if(camp_floor_open(x,y-d)){y-=d;break;}if(camp_floor_open(x,y+d)){y+=d;break;}}}
    int lo=y,hi=y;while(lo>0&&camp_floor_open(x,lo-1))lo--;while(hi<447&&camp_floor_open(x,hi+1))hi++;return (lo+hi)/2;
}
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=255;campaign_camp_start(c,a);
}
static void packet(const Campaign *c,const Arena *a){
    CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,1);assert(campaign_packet_valid(&p));
    campaign_unpack(&p,&guest,&remote);assert(remote.map_id==CAMP_MAP&&guest.group==c->group&&guest.phase==c->phase);
}
static void run(int coop){
    Campaign c;Arena a;begin(&c,&a,coop);
    assert(a.map_id==CAMP_MAP&&c.phase==WAR_TRAVEL&&stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y));
    uint16_t none[2]={0,0};unsigned fights=0,seen[CAMP_EVENTS]={0};
    for(unsigned t=0;t<40000&&c.phase!=WAR_COMPLETE;t++){
        /* a stand-in for the players: walk right on the plain, wipe whatever is alive 30 ticks into each fight */
        int x=a.p[0].x,tx=c.group<CAMP_EV_EXIT?campaign_camp_event_x(c.group)+5:CAMP_DOOR3_X;
        if(c.phase==WAR_TRAVEL&&c.group<CAMP_EV_EXIT&&x<tx){int ox=a.p[0].x,oy=a.p[0].y;a.p[0].x+=3;stage_move(&a.p[0],ox,oy);if(a.p[0].x==ox){int dir=(t/40)&1?2:-2;a.p[0].y+=dir;if(!stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y))a.p[0].y=oy;}}
        if(a.p[0].x>=LADDER_X-8&&a.p[0].y>200&&c.group>=CAMP_EV_STORY){a.p[0].x=CAMP_SL_TOP0_X;a.p[0].y=CAMP_SL_TOP0_Y;} /* the ladder is tested separately */
        if(c.phase==WAR_TRAVEL&&c.group>=CAMP_EV_EXIT){a.p[0].x=CAMP_DOOR3_X-30;a.p[0].y=CAMP_DOOR3_Y;assert(stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y));}
        if(a.p[1].ready&&abs(a.p[0].x-a.p[1].x)>200){int dir=a.p[0].x>a.p[1].x?1:-1,ox=a.p[1].x,oy=a.p[1].y;a.p[1].x+=dir*3;a.p[1].y=a.p[0].y;stage_move(&a.p[1],ox,oy);if(!stage_position_open_map(CAMP_MAP,a.p[1].x,a.p[1].y)){a.p[1].x=ox;a.p[1].y=oy;}} /* the partner trails the leader */
        a.tick++;campaign_step(&c,&a,none);
        if(c.phase==WAR_FIGHT){seen[c.group]=1;if(c.timer%60==59||c.group==CAMP_EV_BOSS){for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.enemies[i].hp>0){c.enemies[i].hp=0;c.enemies[i].pose=POSE_DEAD;c.kills++;}}fights++;}
        for(int h=0;h<2;h++)if(a.p[h].ready&&a.p[h].hp<60)a.p[h].hp=120;
        if(t%113==0)packet(&c,&a);
        for(int h=0;h<2;h++)if(a.p[h].ready)assert(stage_position_open_map(CAMP_MAP,a.p[h].x,a.p[h].y));
    }
    assert(c.phase==WAR_COMPLETE&&(c.completed&256)&&fights>0);
    for(unsigned e=0;e<CAMP_EV_EXIT;e++)assert(seen[e]);
}
static void ladder(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0},up[2]={UP,0},down[2]={DOWN,0};
    c.group=CAMP_EV_EXIT;
    a.p[0].x=LADDER_X;a.p[0].y=CAMP_SL_BOT0_Y+6;a.p[0].z=0;
    assert(stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y)&&stage_position_open_map(CAMP_MAP,CAMP_SL_TOP0_X,CAMP_SL_TOP0_Y));
    /* the ladder shaft is walkable: up reaches the top deck, and every step is a legal snapshot position */
    for(int i=0;i<300&&a.p[0].y>CAMP_SL_TOP0_Y+4;i++){a.tick++;campaign_step(&c,&a,up);packet(&c,&a);assert(stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y));}
    assert(a.p[0].y<=CAMP_SL_TOP0_Y+4);
    /* the top deck reaches the exit door by walking */
    for(int i=0;i<3000&&a.p[0].x<CAMP_DOOR3_X-40;i++){uint16_t right[2]={(uint16_t)(steer(&a.p[0],a.p[0].x+100,band_mid(a.p[0].x+30,a.p[0].y))|(a.p[0].x<5130?UP:0)),0};a.tick++;campaign_step(&c,&a,right);}
    assert(a.p[0].x>=CAMP_DOOR3_X-40);
    a.p[0].x=LADDER_X;a.p[0].y=CAMP_SL_TOP0_Y;
    for(int i=0;i<300&&a.p[0].y<CAMP_SL_BOT0_Y;i++){a.tick++;campaign_step(&c,&a,down);packet(&c,&a);}
    assert(a.p[0].y>=CAMP_SL_BOT0_Y-4);(void)none;
}
static void actors(void){
    Campaign c;Arena a;uint16_t none[2]={0,0};
    /* propeller bears descend from 80 to the ground and then fight */
    begin(&c,&a,0);a.p[0].x=campaign_camp_event_x(CAMP_EV_PROPELLER);a.p[0].y=288;c.group=CAMP_EV_PROPELLER;c.phase=WAR_TRAVEL;
    unsigned maxz=0,flying=0,landed=0;
    for(int t=0;t<200;t++){a.tick++;campaign_step(&c,&a,none);packet(&c,&a);
        for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.species[i]==SPECIES_BEAR){if(c.enemies[i].pose==POSE_DIG){flying++;if((unsigned)c.enemies[i].z>maxz)maxz=(unsigned)c.enemies[i].z;}else if(flying&&!c.enemies[i].z)landed=1;}}
    assert(maxz>=70&&flying>40&&landed);
    /* bats fly left on a straight line at height 25 and leave the screen */
    begin(&c,&a,0);a.p[0].x=campaign_camp_event_x(CAMP_EV_BATS);a.p[0].y=118;c.group=CAMP_EV_BATS;c.phase=WAR_TRAVEL;
    unsigned seen=0,x0=0;
    for(int t=0;t<900&&c.phase!=WAR_COMPLETE;t++){a.tick++;campaign_step(&c,&a,none);packet(&c,&a);
        for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.species[i]==SPECIES_BAT){seen++;assert(c.enemies[i].z==25&&c.enemies[i].facing==-1);if(!x0)x0=(unsigned)c.enemies[i].x;}
        for(int h=0;h<2;h++)if(a.p[h].ready&&a.p[h].hp<60)a.p[h].hp=120;}
    assert(seen>0&&x0>5936&&c.group>CAMP_EV_BATS); /* every bat crossed and left, the wave ended*/
    /* the boss dances while the party walks to its marks, and cannot be hurt until then */
    begin(&c,&a,1);a.p[0].x=campaign_camp_event_x(CAMP_EV_BOSS);a.p[0].y=288;a.p[1].x=a.p[0].x-20;a.p[1].y=292;c.group=CAMP_EV_BOSS;c.phase=WAR_TRAVEL;
    int danced=0;
    for(int t=0;t<135;t++){a.tick++;campaign_step(&c,&a,none);packet(&c,&a);
        if(c.timer<119)for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.species[i]==SPECIES_BEAR&&c.enemies[i].pose==POSE_CAST){danced=1;assert(c.enemies[i].invulnerable>0);}}
    assert(danced&&abs(a.p[0].x-CAMP_P1_4_X)<=3);
}
/* A scripted player who fights what comes and walks on: fighting with the real enemy AI, the ladder and the exit, end to end. */
static void bot_run(int coop){
    Campaign c;Arena a;begin(&c,&a,coop);c.completed=255;
    unsigned events=0;
    for(unsigned t=0;t<300000&&a.map_id==CAMP_MAP;t++){
        uint16_t keys[2]={0,0};
        for(int h=0;h<2;h++){
            Fighter *p=&a.p[h];if(!p->ready)continue;
            if(p->hp<80)p->hp=campaign_health(&c,h);
            const Fighter *q=0;int best=9999;
            for(int e=0;e<6;e++)if(c.enemies[e].ready&&c.enemies[e].hp>0&&c.species[e]!=SPECIES_BAT){int d=abs(p->x-c.enemies[e].x)+abs(p->y-c.enemies[e].y);if(d<best){q=&c.enemies[e];best=d;}}
            if(q&&best<260&&!(p->y<200&&q->y>200)){keys[h]=steer(p,q->x+(q->x>p->x?-30:30),q->y);if(abs(p->y-q->y)<12&&abs(p->x-q->x)<55&&t%11==0)keys[h]|=ATTACK|ATTACK_EDGE;}
            else if(p->y>118&&p->x>=LADDER_X-60&&p->x<=5112&&c.group>=CAMP_EV_STORY){if(abs(p->x-LADDER_X)<8)keys[h]=UP;else keys[h]=steer(p,LADDER_X,p->y>CAMP_SL_BOT0_Y+3?p->y:CAMP_SL_BOT0_Y+6);}
            else keys[h]=(uint16_t)(steer(p,p->x+100,band_mid(p->x+30>=CAMP_WIDTH?p->x:p->x+30,p->y))|(p->x>=5100&&p->x<5135?UP:0));
            if(a.p[1].ready&&h==1&&a.p[0].hp>0&&abs(p->x-a.p[0].x)>150)keys[h]=steer(p,a.p[0].x,a.p[0].y);
        }
        if(c.phase==WAR_FIGHT)events|=1u<<c.group;
        a.tick++;campaign_step(&c,&a,keys);
        if(t%17==0)packet(&c,&a);
    }
    assert(events==(1u<<CAMP_EV_EXIT)-1&&c.phase==CAMPAIGN_WORLD_MAP&&c.group==9&&(c.completed&256));
}
static void magic_test(void){
    Campaign c;Arena a;uint16_t none[2]={0,0};
    /* a level 3 bear casts: wind balls (the machine slot) hurt, and a tornado (pose THROW) spins into the target */
    begin(&c,&a,0);a.p[0].x=3000;a.p[0].y=288;c.group=CAMP_EV_LEFT;c.phase=WAR_TRAVEL;
    unsigned balls=0,tornadoes=0,hurt=0;int hp0=a.p[0].hp;
    for(int t=0;t<1500;t++){
        if(c.phase==WAR_TRAVEL&&c.group==CAMP_EV_LEFT){a.p[0].x=campaign_camp_event_x(CAMP_EV_LEFT);a.p[0].y=288;}
        a.tick++;campaign_step(&c,&a,none);packet(&c,&a);
        if(c.war.machine.ready&&c.war.machine.attack_kind==0)balls++;
        for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.species[i]==SPECIES_BEAR&&c.enemies[i].pose==POSE_THROW&&c.enemies[i].hp>0)tornadoes++;
        if(a.p[0].hp<hp0){hurt++;hp0=a.p[0].hp;}
        if(a.p[0].hp<60)a.p[0].hp=120,hp0=120;
        for(int i=0;i<6;i++)if(c.enemies[i].ready&&c.enemies[i].hp>0&&c.enemies[i].hp<c.enemies[i].hp+1&&c.group==CAMP_EV_LEFT&&c.phase==WAR_FIGHT&&t>1400)c.enemies[i].hp=0;
    }
    assert(balls>0&&tornadoes>0&&hurt>0);
}
/* The animal orbs appear near their spots once their encounter is behind the party, are shared on pickup and are not placed again. */
static void orbs(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    static const struct {int x;unsigned group,id;} orb[3]={{1610,CAMP_EV_ROW,PET_HAWKSTER},{2562,CAMP_EV_PROPELLER,PET_RAMMY},{4823,CAMP_EV_STORY,PET_BURLY}};
    for(int i=0;i<3;i++){
        begin(&c,&a,0);c.group=orb[i].group-1;c.phase=WAR_TRAVEL;a.p[0].x=orb[i].x;a.p[0].y=288;
        if(!stage_position_open_map(CAMP_MAP,a.p[0].x,a.p[0].y))a.p[0].y=286;
        a.tick++;campaign_step(&c,&a,none);
        int seen=0;for(int d=0;d<CAMPAIGN_DROPS;d++)seen|=c.drops[d].kind==DROP_PET&&(c.drops[d].value&31)==orb[i].id;
        assert(!seen); /* the encounter is not over yet */
        c.group=orb[i].group;c.phase=WAR_TRAVEL;a.p[0].x=orb[i].x;a.tick++;campaign_step(&c,&a,none);
        seen=0;for(int d=0;d<CAMPAIGN_DROPS;d++)seen|=c.drops[d].kind==DROP_PET&&(c.drops[d].value&31)==orb[i].id;
        if(!seen)fprintf(stderr,"orb %d x %d y %d group %u phase %u\n",i,a.p[0].x,a.p[0].y,c.group,c.phase);
        assert(seen);
    }
    /* Rammy rams the enemy in front of it every 60 ticks, Burly Bear and Hawkster strike near them. */
    static const struct {unsigned id;int dx,period,damage;} pet[3]={{PET_RAMMY,40,60,12},{PET_BURLY,25,90,16},{PET_HAWKSTER,30,75,8}};
    for(int i=0;i<3;i++){
        begin(&c,&a,0);c.phase=WAR_TRAVEL;c.group=CAMP_EV_ROW-1;a.p[0].x=400;a.p[0].y=300;a.p[0].facing=1;
        c.pets[0].equipped=(uint8_t)pet[i].id;c.pets[0].owned=1ull<<pet[i].id;c.pet_followers[0]=(PetFollower){.x=400,.y=300,.facing=1};
        Fighter e={.ready=1,.hp=100,.x=400+pet[i].dx,.y=300,.facing=-1,.knight_id=1};c.enemies[0]=e;
        for(int t=0;t<pet[i].period-1;t++){c.enemies[0].invulnerable=0;a.tick++;campaign_combat_tick(&c,&a,none);c.pet_followers[0].x=400;c.pet_followers[0].y=300;c.enemies[0].x=400+pet[i].dx;c.enemies[0].y=300;}
        assert(c.enemies[0].hp>=100-3); /* nothing yet */
        c.enemies[0].invulnerable=0;a.tick++;campaign_combat_tick(&c,&a,none);
        assert(c.enemies[0].hp<=100-pet[i].damage+3&&c.enemies[0].hp>=100-pet[i].damage-3);
    }
}
/* The wooden barricades shoot arrows at heroes walking up to them from the west. */
static void barricades(void){
    Campaign c;Arena a;begin(&c,&a,0);uint16_t none[2]={0,0};
    c.group=CAMP_EV_LEFT;c.phase=WAR_TRAVEL;a.p[0].x=3800;a.p[0].y=270;a.p[0].hp=100;
    int fired=0,hurt=0,hp=a.p[0].hp;
    for(int t=0;t<400;t++){a.tick++;campaign_step(&c,&a,none);if(c.war.machine.ready&&c.war.machine.attack_kind==1)fired=1;if(a.p[0].hp<hp){hurt=1;break;}a.p[0].x=3800;a.p[0].y=250+(t/30)%20;}
    assert(fired&&hurt);
}
int main(void){
    {Campaign w={0};assert(!campaign_world_unlocked(&w,9));w.completed=128;assert(campaign_world_unlocked(&w,9));}
    ladder();orbs();barricades();actors();magic_test();run(0);run(1);bot_run(0);bot_run(1);
    puts("PASS: Level 20 encounters, boss, ladder, exit door, clear bit and wire");
    return 0;
}
