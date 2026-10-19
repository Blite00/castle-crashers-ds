#include "campaign.h"
#include "stage.h"
#include "campaign_combat.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void legal(Fighter *p,int x,unsigned map){p->x=x;for(int y=0;y<stage_world_height(map);y++)if(stage_position_open_map(map,x,y)){p->y=y;return;}assert(0);}
static uint16_t walk_right(const Fighter *p,unsigned map){
    /* Look ahead along the corridor and walk around vertical wall ends. */
    int ahead=p->x+24;if(ahead>stage_world_width(map)-34)ahead=stage_world_width(map)-34;
    int lo=0,hi=stage_world_height(map)-1;
    while(lo<hi&&!stage_position_open_map(map,ahead,lo))lo++;
    while(hi>lo&&!stage_position_open_map(map,ahead,hi))hi--;
    int target=(lo+hi)/2;
    /* Enter the first stair along its top surface, not through the cliff face. */
    if(map==3&&p->x<540)target=lo+22;
    return RIGHT|(p->y<target-3?DOWN:p->y>target+3?UP:0);
}
static void valid(Campaign *c,Arena *a){
    CampaignPacket p;campaign_pack(&p,c,a,a->tick);if(!campaign_packet_valid(&p)){fprintf(stderr,"Invalid scene%u phase%u tick%u\n",a->map_id,c->phase,a->tick);assert(0);}
    Arena replica;Campaign other={0};campaign_sync_cycle(c,a,&other,&replica,0);CampaignPacket repeat;campaign_pack(&repeat,&other,&replica,a->tick);
    assert(!memcmp(&p,&repeat,sizeof(p)));
    for(unsigned h=0;h<2;h++)assert(!!campaign_potion_equipped(c,h)==!!campaign_potion_equipped(&other,h));
}
/* Older route assertions explicitly traverse the barracks exit first. */
static void start_keep(Campaign *c,Arena *a,int coop){
    campaign_start(c,a,coop);c->phase=KEEP_BARRACKS_TRAVEL;
    for(int h=0;h<2;h++){a->p[h].x=1150;a->p[h].y=150;}
    uint16_t idle[2]={0};campaign_step(c,a,idle);assert(a->map_id==3&&c->phase==KEEP_INTRO);
}
int main(void){
    for(int coop=0;coop<2;coop++){
        Campaign room={0};Arena team;arena_init(&team);campaign_start(&room,&team,coop);
        assert(team.map_id==5&&room.phase==KEEP_BARRACKS_INTRO&&team.p[1].ready==coop);
        uint16_t k[2]={RIGHT,RIGHT};int x=team.p[0].x;
        for(int t=0;t<486;t++){campaign_step(&room,&team,k);assert(team.p[0].x==x);valid(&room,&team);}
        assert(room.phase==KEEP_BARRACKS_TRAVEL);
        /* Full input traversal includes the authored upstairs exit. */
        int waypoint[2]={0,0};
        for(int t=0;t<800&&team.map_id==5;t++){
            for(int h=0;h<2;h++){
                Fighter *p=&team.p[h];
                if(waypoint[h]==0&&p->y>=298)waypoint[h]=1;
                if(waypoint[h]==1&&p->x>=900)waypoint[h]=2;
                if(waypoint[h]==2&&p->y<=265)waypoint[h]=3;
                k[h]=waypoint[h]==0?DOWN:waypoint[h]==1?RIGHT:waypoint[h]==2?UP:RIGHT;
                if(waypoint[h]==3&&p->x>=1100){if(p->y<155)k[h]|=DOWN;else if(p->y>160)k[h]|=UP;}
            }
            campaign_step(&room,&team,k);valid(&room,&team);
        }
        if(team.map_id!=3)fprintf(stderr,"Barracks exit blocked %d,%d partner%d,%d\n",team.p[0].x,team.p[0].y,team.p[1].x,team.p[1].y);
        assert(team.map_id==3&&room.phase==KEEP_INTRO);
        campaign_start(&room,&team,coop);room.timer=31;k[0]=ATTACK_EDGE;k[1]=0;campaign_step(&room,&team,k);assert(room.phase==KEEP_BARRACKS_TRAVEL);
        for(int h=0;h<2;h++){team.p[h].x=515;team.p[h].y=250;}
        k[0]=k[1]=ATTACK_EDGE;campaign_step(&room,&team,k);assert(room.potions[0]==1&&room.potions[1]==(unsigned)coop);valid(&room,&team);
        room.potions[0]=0;campaign_step(&room,&team,k);assert(room.potions[0]==0); /* One tutorial gift. */
        room.profiles[0].gold=37;room.profiles[0].xp=22;room.potions[0]=1;team.p[0].hp=73;
        for(int h=0;h<2;h++){team.p[h].x=1150;team.p[h].y=150;}k[0]=k[1]=0;campaign_step(&room,&team,k);
        assert(team.map_id==3&&room.profiles[0].gold==37&&room.profiles[0].xp==22&&room.potions[0]==1&&team.p[0].hp==73);valid(&room,&team);
    }
    {
        Campaign room={0};Arena team;arena_init(&team);campaign_start(&room,&team,0);room.phase=KEEP_BARRACKS_TRAVEL;
        uint16_t k[2]={0};
        for(int b=1;b<=5;b++){
            team.p[0].x=campaign_barrel_positions[b-1][0]+26;team.p[0].y=campaign_barrel_positions[b-1][1];team.p[0].facing=-1;
            for(int strike=1;strike<=3;strike++){
                team.p[0].pose=POSE_ATTACK;team.p[0].attack_kind=HIT_LIGHT;team.p[0].cooldown=8;team.p[0].animation=1;team.p[0].attack_seq=b*4+strike;
                campaign_step(&room,&team,k);assert(room.break_hp[b]==3-strike);
                campaign_step(&room,&team,k);assert(room.break_hp[b]==3-strike);
            }
            assert(room.broken&(1u<<b));
        }
        /* Snapshot mask reserves exactly cart+five barrel bits. */
        team.p[0].x=336;team.p[0].y=285;valid(&room,&team);CampaignPacket pkt;campaign_pack(&pkt,&room,&team,1);pkt.countdown|=64;assert(!campaign_packet_valid(&pkt));
    }

    /* Traverse the entire original route using input, not section teleports. */
    {
        Arena walk;CampaignCombatState state={0};arena_init(&walk);walk.map_id=3;walk.p[1].ready=0;legal(&walk.p[0],60,3);walk.p[0].y+=10;
        Fighter empty[6]={{0}},ally={0};uint8_t targets[6]={0},levels[2]={1,1},hits[2];uint16_t right[2]={RIGHT,0};
        for(int tick=0;tick<1400;tick++){
            int x=walk.p[0].x,y=walk.p[0].y;right[0]=walk_right(&walk.p[0],3);
            campaign_combat_step(&state,&walk,empty,&ally,targets,right,levels,hits);
            assert(walk.p[0].x-x>=0&&walk.p[0].x-x<=8);
            if(abs(walk.p[0].y-y)>8)fprintf(stderr,"Floor jump x%d->%d y%d->%d\n",x,walk.p[0].x,y,walk.p[0].y);
            assert(abs(walk.p[0].y-y)<=8);
        }
        if(walk.p[0].x<2380)fprintf(stderr,"Blocked continuous walk x%d y%d\n",walk.p[0].x,walk.p[0].y);
        assert(walk.p[0].x>=2380&&stage_position_open_map(3,walk.p[0].x,walk.p[0].y));
    }

    for(int coop=0;coop<2;coop++){
        Arena a;Campaign c={0};arena_init(&a);start_keep(&c,&a,coop);
        assert(a.p[0].hp==100&&a.p[1].ready==coop&&c.phase==KEEP_INTRO);
        for(int tick=0;tick<5000&&!(c.completed&1);tick++){
            uint16_t keys[2]={0};
            if(c.phase==KEEP_FIGHT||c.phase==KEEP_STORE_FIGHT){
                for(int e=0;e<6;e++)if(c.enemies[e].ready&&c.enemies[e].hp){c.enemies[e].hp=1;c.enemies[e].burn=30;}
            }else if(c.phase==KEEP_TRAVEL||c.phase==KEEP_STORE){keys[0]=walk_right(&a.p[0],a.map_id);if(coop)keys[1]=walk_right(&a.p[1],a.map_id);}
            int old_x=a.p[0].x,old_map=a.map_id,old_kills=c.kills,old_group=c.group;
            campaign_step(&c,&a,keys);if(!((a.map_id==old_map&&abs(a.p[0].x-old_x)<=16)||(old_map==3&&a.map_id==4)||(old_map==4&&a.map_id==6&&(c.completed&1)))){printf("Move %d -> %d map%d -> %d phase%d\n",old_x,a.p[0].x,old_map,a.map_id,c.phase);return 1;}if(coop)valid(&c,&a);
            if(c.completed&1)assert(old_kills==19&&old_group==4);
        }
        assert(!a.winner&&(c.completed&1)&&a.map_id==WAR_MAP&&c.phase==WAR_FIGHT);
        c.profiles[0].xp=195;c.profiles[0].level=2;c.profiles[0].gold=20;start_keep(&c,&a,coop);
        assert(c.profiles[0].xp==195&&c.profiles[0].gold==20&&a.p[0].hp==103);
    }
    /* Entering a subsequent encounter preserves old bodies and their clocks. */
    {
        Arena a;Campaign c={0};arena_init(&a);start_keep(&c,&a,0);
        c.phase=KEEP_FIGHT;c.group=0;c.spawned=3;c.story_flags=1;
        legal(&a.p[0],900,3);uint16_t keys[2]={0};
        for(int i=0;i<3;i++)c.enemies[i]=(Fighter){.x=820+i*20,.y=360,.ready=1,.pose=POSE_DEAD};
        campaign_step(&c,&a,keys);
        assert(c.group==1&&c.phase==KEEP_FIGHT&&c.spawned==3);
        for(int i=0;i<3;i++)assert(c.enemies[i].ready&&!c.enemies[i].hp&&c.enemies[i].animation==1);
        for(int i=3;i<6;i++)assert(c.enemies[i].ready&&c.enemies[i].hp);
        valid(&c,&a);
        /* All six slots occupied: defer only the new spawn, never erase a corpse. */
        start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;c.group=1;c.story_flags=1;
        legal(&a.p[0],900,3);
        for(int i=0;i<6;i++)c.enemies[i]=(Fighter){.x=820+i*20,.y=360,.ready=1,.pose=POSE_DEAD};
        campaign_step(&c,&a,keys);assert(c.phase==KEEP_FIGHT&&!c.spawned);
        for(int t=2;t<44;t++){
            campaign_step(&c,&a,keys);assert(!c.spawned);
            for(int i=0;i<6;i++)assert(c.enemies[i].ready&&!c.enemies[i].hp&&c.enemies[i].animation==t);
        }
        campaign_step(&c,&a,keys);assert(c.spawned==3&&c.phase==KEEP_FIGHT);
        assert(c.enemies[0].hp&&c.enemies[0].x==1198);
        assert(c.enemies[1].hp&&c.enemies[1].x==1248);
        assert(c.enemies[2].hp&&c.enemies[2].x==1298);
        for(int i=3;i<6;i++)assert(!c.enemies[i].ready);
        valid(&c,&a);
    }
    Arena a;Campaign c={0};arena_init(&a);start_keep(&c,&a,1);c.phase=KEEP_TRAVEL;
    a.p[0].hp=30;c.drop_age[0]=19;c.drops[0]=(CampaignDrop){a.p[0].x,a.p[0].y,DROP_FOOD,10};uint16_t keys[2]={0};campaign_step(&c,&a,keys);assert(a.p[0].hp==40&&!c.drops[0].kind);assert(c.pickup_kind[0]==DROP_FOOD&&c.pickup_value[0]==10&&c.pickup_timer[0]==30&&c.pickup_seq[0]==1);valid(&c,&a);
    c.drop_age[0]=19;c.drops[0]=(CampaignDrop){a.p[0].x,a.p[0].y,DROP_BAG,5};campaign_step(&c,&a,keys);assert(c.profiles[0].gold==5);assert(c.pickup_kind[0]==DROP_BAG&&c.pickup_value[0]==5&&c.pickup_seq[0]==2);valid(&c,&a);
    c.profiles[0].xp=190;a.map_id=3;c.phase=KEEP_FIGHT;c.group=0;c.spawned=3;
    legal(&a.p[0],820,a.map_id);a.p[0].y+=15;a.p[0].facing=1;a.p[0].pose=POSE_ATTACK;a.p[0].cooldown=7;a.p[0].animation=2;a.p[0].attack_kind=HIT_LIGHT;
    c.enemies[0]=(Fighter){.x=a.p[0].x+40,.y=a.p[0].y,.hp=50,.ready=1,.facing=-1,.mana=40};
    for(int t=0;t<3;t++)campaign_step(&c,&a,keys);
    assert(c.profiles[0].xp>=191&&c.profiles[0].level==2&&a.p[0].hp==103);
    start_keep(&c,&a,1);c.phase=KEEP_TRAVEL;a.p[1].hp=0;a.p[1].x=a.p[0].x+20;a.p[1].y=a.p[0].y;
    keys[0]=BLOCK;for(int t=0;t<65;t++)campaign_step(&c,&a,keys);assert(a.p[1].hp>0);
    unsigned rng=123;
    for(int t=0;t<30000;t++){
        rng=rng*1664525u+1013904223u;keys[0]=(rng>>12)&INPUT_MASK;keys[1]=(rng>>20)&INPUT_MASK;
        campaign_step(&c,&a,keys);valid(&c,&a);if(a.winner)start_keep(&c,&a,1);
    }
    start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;keys[0]=keys[1]=0;
    for(int event=0;event<40;event++){
        c.drop_age[0]=19;c.drops[0]=(CampaignDrop){a.p[0].x,a.p[0].y,DROP_COIN,1};
        campaign_step(&c,&a,keys);assert(c.pickup_seq[0]==((event+1)&31));valid(&c,&a);
    }
    start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;legal(&a.p[0],760,3);keys[0]=keys[1]=0;
    campaign_step(&c,&a,keys);
    assert(c.phase==KEEP_FIGHT&&c.enemies[0].x==869&&c.enemies[0].y==364);
    assert(c.enemies[1].x==919&&c.enemies[2].x==969&&c.enemies[2].y==401);
    for(int group=1;group<4;group++){
        start_keep(&c,&a,0);c.group=group;c.phase=KEEP_TRAVEL;c.story_flags=1;
        legal(&a.p[0],group==1?870:group==2?1545:1960,3);c.timer=95;
        campaign_step(&c,&a,keys);
        assert(c.phase==KEEP_FIGHT);
        assert(c.enemies[0].x==(group==1?1198:group==2?1543:2292));
    }
    c.timer=27;campaign_step(&c,&a,keys);
    assert(c.enemies[2].x==2023&&c.enemies[2].y==176&&c.enemies[2].z==35);
    c.timer=40;campaign_step(&c,&a,keys);
    assert(c.enemies[3].x==2146&&c.enemies[3].y==177&&c.enemies[3].z==45);
    CampaignPacket p;campaign_pack(&p,&c,&a,1);p.actors[2].pose=POSE_COUNT;assert(!campaign_packet_valid(&p));
    /* Pan phases freeze actors and reliably return to the unmodified route. */
    start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;c.group=2;legal(&a.p[0],1422,3);keys[0]=keys[1]=0;
    campaign_step(&c,&a,keys);assert(c.phase==KEEP_PAN_IN&&(c.story_flags&1));int pan_x=a.p[0].x;
    for(int t=0;t<18;t++){campaign_step(&c,&a,keys);assert(a.p[0].x==pan_x);valid(&c,&a);}
    assert(c.phase==KEEP_ABDUCTION);c.timer=31;keys[0]=ATTACK_EDGE;campaign_step(&c,&a,keys);assert(c.phase==KEEP_PAN_OUT);
    keys[0]=0;for(int t=0;t<18;t++){campaign_step(&c,&a,keys);}assert(c.phase==KEEP_FIGHT&&c.spawned==3);
    assert(a.p[0].x==pan_x&&c.enemies[0].x==1543&&c.enemies[1].x==1533&&c.enemies[2].x==1553);
    /* A co-op leader may already occupy the original entrance while the
       trailing partner triggers the movie. Neither can receive a body spawn. */
    start_keep(&c,&a,1);c.group=2;c.phase=KEEP_PAN_OUT;c.timer=17;c.story_flags=1;
    a.p[0].x=1543;a.p[0].y=368;a.p[1].x=1600;a.p[1].y=368;
    campaign_step(&c,&a,keys);assert(c.phase==KEEP_FIGHT&&c.spawned==3);
    assert(a.p[0].x==1543&&a.p[1].x==1600);
    for(int e=0;e<3;e++){
        assert(stage_position_open_map(3,c.enemies[e].x,c.enemies[e].y));
        for(int h=0;h<2;h++)assert(abs(c.enemies[e].x-a.p[h].x)>=56||abs(c.enemies[e].y-a.p[h].y)>=28);
    }
    valid(&c,&a);
    /* Corpses can occupy all six slots through the camera sequence; wait for
       their removal instead of overwriting them or skipping the encounter. */
    start_keep(&c,&a,0);c.group=2;c.phase=KEEP_PAN_OUT;c.timer=17;
    for(int e=0;e<6;e++){c.enemies[e]=(Fighter){.ready=1,.hp=0,.pose=POSE_DEAD,.x=1400,.y=368};c.combat.death_age[e]=1;}
    campaign_step(&c,&a,keys);assert(c.phase==KEEP_FIGHT&&c.spawned==0);
    for(int t=0;t<120&&!c.spawned;t++)campaign_step(&c,&a,keys);
    assert(c.spawned==3&&c.phase==KEEP_FIGHT);
    /* One map boundary preserves health, currency, XP and inventory. */
    c.group=4;c.kills=13;c.phase=KEEP_TRAVEL;c.profiles[0].gold=72;c.profiles[0].xp=17;c.potions[0]=1;a.p[0].hp=44;legal(&a.p[0],2381,3);
    campaign_step(&c,&a,keys);assert(a.map_id==4&&c.phase==KEEP_STORE_FIGHT&&c.spawned==6&&c.ally.ready&&c.ally.hp==60);
    assert(c.profiles[0].gold==72&&c.profiles[0].xp==17&&c.potions[0]==1&&a.p[0].hp==44);
    /* Optimized placement keeps the old nearest-Y result and lower-Y tie. */
    static const int store_anchors[6][2]={{639,274},{589,294},{664,309},{689,279},{739,319},{759,274}};
    for(int e=0;e<6;e++){
        int nearest=-1,distance=300;
        for(int yy=0;yy<stage_world_height(4);yy++)if(stage_position_open_map(4,store_anchors[e][0],yy)&&abs(yy-store_anchors[e][1])<distance){nearest=yy;distance=abs(yy-store_anchors[e][1]);}
        assert(c.enemies[e].x==store_anchors[e][0]&&c.enemies[e].y==nearest);
    }
    for(int e=0;e<6;e++){assert(c.enemies[e].ready&&c.enemies[e].hp==50);c.enemies[e].hp=1;c.enemies[e].burn=30;}
    campaign_step(&c,&a,keys);assert(c.phase==KEEP_STORE&&c.kills==19);valid(&c,&a);
    /* Shop press edges charge once, cap inventory, and reject insufficient gold. */
    memset(c.drops,0,sizeof(c.drops));c.ally.ready=0;legal(&a.p[0],617,4);a.p[0].y+=10;
    keys[0]=ATTACK;campaign_step(&c,&a,keys);assert(c.potions[0]==2&&c.profiles[0].gold==60);assert(c.pickup_kind[0]==DROP_POTION&&c.pickup_value[0]==2&&c.pickup_timer[0]==30);valid(&c,&a);
    for(int t=0;t<5;t++){campaign_step(&c,&a,keys);}assert(c.potions[0]==2&&c.profiles[0].gold==60);
    keys[0]=ATTACK|ATTACK_EDGE;for(int t=0;t<5;t++){campaign_step(&c,&a,keys);}assert(c.potions[0]==5&&c.profiles[0].gold==24);
    c.potions[0]=0;c.profiles[0].gold=11;campaign_step(&c,&a,keys);assert(!c.potions[0]&&c.profiles[0].gold==11);
    /* Inventory starts unselected. Y cannot use a potion until its slot is tapped. */
    c.potions[0]=2;a.p[0].hp=30;keys[0]=BOW;campaign_step(&c,&a,keys);assert(a.p[0].hp==30&&c.potions[0]==2&&!a.p[0].bow_equipped);
    keys[0]=EQUIP;campaign_step(&c,&a,keys);assert(campaign_potion_equipped(&c,0)&&!a.p[0].bow_equipped);valid(&c,&a);
    campaign_step(&c,&a,keys);assert(campaign_potion_equipped(&c,0));
    keys[0]=BOW;campaign_step(&c,&a,keys);assert(a.p[0].hp==campaign_max_health(c.profiles[0].level)&&c.potions[0]==1);
    a.p[0].hp=70;campaign_step(&c,&a,keys);assert(a.p[0].hp==70&&c.potions[0]==1);
    keys[0]=EQUIP|EQUIP_EDGE;campaign_step(&c,&a,keys);assert(!campaign_potion_equipped(&c,0));
    keys[0]=BOW|BOW_EDGE;campaign_step(&c,&a,keys);assert(a.p[0].hp==70&&c.potions[0]==1);
    keys[0]=EQUIP|EQUIP_EDGE;campaign_step(&c,&a,keys);assert(campaign_potion_equipped(&c,0));
    a.p[0].hp=campaign_max_health(c.profiles[0].level);keys[0]=BOW|BOW_EDGE;campaign_step(&c,&a,keys);assert(c.potions[0]==1);
    a.p[0].hp=70;campaign_step(&c,&a,keys);assert(!c.potions[0]&&!campaign_potion_equipped(&c,0));valid(&c,&a);
    keys[0]=EQUIP|EQUIP_EDGE;campaign_step(&c,&a,keys);assert(!a.p[0].bow_equipped&&!campaign_potion_equipped(&c,0));
    legal(&a.p[0],966,4);keys[0]=0;campaign_step(&c,&a,keys);assert(c.phase==KEEP_DOOR);
    for(int t=0;t<18;t++){campaign_step(&c,&a,keys);}assert((c.completed&1)&&!a.winner&&a.map_id==WAR_MAP&&c.phase==WAR_FIGHT);valid(&c,&a);
    /* Original hay cart requires four distinct hit contacts, not four held frames. */
    start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;legal(&a.p[0],880,3);a.p[0].y=333;a.p[0].facing=1;
    for(int strike=1;strike<=4;strike++){
        a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].cooldown=8;a.p[0].animation=1;a.p[0].attack_seq=strike;
        campaign_step(&c,&a,keys);assert(c.break_hp[0]==4-strike);
        campaign_step(&c,&a,keys);assert(c.break_hp[0]==4-strike);
    }
    assert(c.broken==1&&c.break_tick);valid(&c,&a);
    start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;
    for(int arrow=0;arrow<4;arrow++){
        a.shots[0]=(Projectile){.x=880,.y=333,.z=25,.vx=16,.life=30,.owner=0,.kind=SHOT_ARROW};
        campaign_step(&c,&a,keys);assert(c.break_hp[0]==3-arrow&&!a.shots[0].life);
    }
    assert(c.broken==1);valid(&c,&a);
    /* Sample the original conditional loot rolls; healing is no longer guaranteed. */
    unsigned counts[8]={0},foods[5]={0};c.rng=0x91367;
    for(int roll=0;roll<120000;roll++){unsigned value,kind=campaign_roll_drop(&c,&value);counts[kind]++;if(kind==DROP_FOOD)foods[value>>5]++;if(kind==DROP_FOOD)assert((value&31)==10&&(value>>5)<5);if(kind==DROP_CHICKEN)assert(value==50);if(kind==DROP_BAG)assert(value==5);if(kind==DROP_GEM)assert(value>=6&&value<=16);}
    assert(counts[DROP_NONE]>9000&&counts[DROP_NONE]<11000&&counts[DROP_FOOD]>11500&&counts[DROP_FOOD]<13500&&counts[DROP_CHICKEN]>2100&&counts[DROP_CHICKEN]<2800);
    campaign_pack(&p,&c,&a,1);p.completed=12;assert(!campaign_packet_valid(&p));
    /* Weapons swap on the ground; purchases need space for the old weapon. */
    memset(&c,0,sizeof(c));arena_init(&a);start_keep(&c,&a,0);a.map_id=4;stage_set_map(4);c.phase=KEEP_STORE;
    a.p[0].x=675;a.p[0].y=260;c.profiles[0].gold=40;a.p[0].hp=100;
    keys[0]=ATTACK_EDGE;keys[1]=0;
    for(int d=0;d<CAMPAIGN_DROPS;d++)c.drops[d]=(CampaignDrop){400,260,DROP_WEAPON,WEAPON_STARTER};
    campaign_step(&c,&a,keys);assert(c.weapons[0]==WEAPON_STARTER&&c.profiles[0].gold==40);
    memset(c.drops,0,sizeof(c.drops));campaign_step(&c,&a,keys);
    assert(c.pickup_kind[0]==DROP_WEAPON&&c.pickup_value[0]==WEAPON_SKINNY&&c.pickup_timer[0]==30&&c.pickup_seq[0]==1);
    assert(c.weapons[0]==WEAPON_SKINNY&&c.owned_weapons[0].w[0]==3&&c.profiles[0].gold==8&&a.p[0].hp==100);
    assert(c.drops[0].kind==DROP_WEAPON&&c.drops[0].value==WEAPON_STARTER);valid(&c,&a);
    keys[0]=0;campaign_step(&c,&a,keys);keys[0]=ATTACK_EDGE;campaign_step(&c,&a,keys);
    assert(c.weapons[0]==WEAPON_STARTER&&c.drops[0].value==WEAPON_SKINNY&&c.profiles[0].gold==8);
    campaign_step(&c,&a,keys);assert(c.weapons[0]==WEAPON_SKINNY&&c.profiles[0].gold==8);
    /* Equipment persists into replay, but choosing another knight starts a new inventory. */
    campaign_start(&c,&a,0);assert(c.weapons[0]==WEAPON_SKINNY&&c.owned_weapons[0].w[0]==3&&a.p[0].hp==128);valid(&c,&a);
    a.p[0].knight_id=1;campaign_start(&c,&a,0);assert(c.weapons[0]==0&&c.owned_weapons[0].w[0]==1&&a.p[0].hp==100);
    c.phase=KEEP_BARRACKS_TRAVEL;c.drop_age[0]=19;c.drops[0]=(CampaignDrop){a.p[0].x,a.p[0].y,DROP_WEAPON,WEAPON_CARROT};
    keys[0]=ATTACK_EDGE;campaign_step(&c,&a,keys);assert(c.weapons[0]==0&&c.owned_weapons[0].w[0]==1&&c.drops[0].value==WEAPON_CARROT);valid(&c,&a);
    c.profiles[0].level=5;campaign_step(&c,&a,keys);assert(c.weapons[0]==WEAPON_CARROT&&c.drops[0].value==WEAPON_STARTER&&c.owned_weapons[0].w[0]==5);valid(&c,&a);
    campaign_pack(&p,&c,&a,1);p.profiles[0].level=0;assert(!campaign_packet_valid(&p));campaign_pack(&p,&c,&a,0);p.actors[0].hp=(int16_t)((p.actors[0].hp&2047)|(15<<11));assert(!campaign_packet_valid(&p)); /* weapon id 15 is not registered */
    campaign_pack(&p,&c,&a,1);p.profiles[0].knight&=~16u;assert(!campaign_packet_valid(&p));
    for(int food=0;food<5;food++)assert(foods[food]>2100&&foods[food]<2800);
    assert(counts[DROP_WEAPON]>800&&counts[DROP_WEAPON]<1200);
    /* Source EnemyDie waits until the corpse vanishes, then every item owns
       its pickup/bounce/blink/despawn timeline. Six bodies never overwrite4. */
    memset(&c,0,sizeof(c));arena_init(&a);start_keep(&c,&a,0);c.phase=KEEP_TRAVEL;
    keys[0]=keys[1]=0;
    for(unsigned seed=1;;seed++){
        Campaign probe={.rng=seed};int all=1;
        for(int i=0;i<6;i++){unsigned value;unsigned kind=campaign_roll_drop(&probe,&value);if(!kind||kind==DROP_WEAPON)all=0;}
        if(all){c.rng=seed;break;}
    }
    for(int i=0;i<6;i++){c.enemies[i]=(Fighter){.x=869+i*4,.y=364,.ready=1,.pose=POSE_DEAD,.facing=1};c.combat.death_age[i]=42;}
    campaign_step(&c,&a,keys);
    for(int i=0;i<4;i++)assert(!c.drops[i].kind);
    campaign_step(&c,&a,keys);
    assert(c.pending_count==2);
    for(int i=0;i<6;i++)assert(!c.enemies[i].ready);
    for(int i=0;i<4;i++)assert(c.drops[i].kind&&c.drop_age[i]==0);
    CampaignDrop saved_drop=c.drops[0];
    for(int tick=0;tick<20;tick++)campaign_step(&c,&a,keys);
    assert(!memcmp(&saved_drop,&c.drops[0],sizeof(saved_drop))&&c.drop_age[0]==20&&c.pending_count==2);
    c.drop_age[0]=302;c.drop_age[1]=150;campaign_step(&c,&a,keys);
    assert(c.pending_count==1&&c.drop_age[0]==0&&c.drop_age[1]==151);
    campaign_pack(&p,&c,&a,99);assert(campaign_packet_valid(&p));
    Campaign recovered={0};Arena other;campaign_unpack(&p,&recovered,&other);
    for(int i=0;i<4;i++){assert(recovered.drop_age[i]==c.drop_age[i]);assert(!memcmp(&recovered.drops[i],&c.drops[i],sizeof(CampaignDrop)));}
    p.drops[0].x|=(int16_t)0xc000;assert(!campaign_packet_valid(&p));
    /* Frame20 starts pickup; different item ages do not share a global clock. */
    memset(c.drops,0,sizeof(c.drops));c.pending_count=0;
    c.drops[0]=(CampaignDrop){a.p[0].x,a.p[0].y,DROP_COIN,1};c.drop_age[0]=0;
    unsigned gold=c.profiles[0].gold;
    for(int tick=0;tick<18;tick++)campaign_step(&c,&a,keys);
    assert(c.drops[0].kind&&c.profiles[0].gold==gold);
    campaign_step(&c,&a,keys);assert(!c.drops[0].kind&&c.profiles[0].gold==gold+1);
    c.drops[0]=(CampaignDrop){869,364,DROP_COIN,1};
    static const unsigned hidden[]={262,266,270,274,278,281,284,287,290,292,294,296,298,300,302};
    for(unsigned frame=1;frame<304;frame++){
        int visible=1;for(unsigned j=0;j<sizeof(hidden)/sizeof(hidden[0]);j++)if(hidden[j]==frame)visible=0;
        c.drop_age[0]=frame-1;assert(campaign_drop_visible(&c,0)==visible);
    }
    c.drop_age[0]=5;assert(campaign_drop_height(&c,0)==50);
    c.drop_age[0]=19;assert(!campaign_drop_height(&c,0));
    c.drops[0].kind=DROP_WEAPON;c.drops[0].value=WEAPON_STARTER;c.drop_age[0]=0;
    for(int tick=0;tick<400;tick++)campaign_step(&c,&a,keys);
    assert(c.drops[0].kind==DROP_WEAPON&&!c.drop_age[0]);
    puts("PASS: continuous full-route walking, no section teleport, solo/co-op completion,19 enemies, source XP/level-up, food/gold/revive/replay,30000 compact snapshots");
}
