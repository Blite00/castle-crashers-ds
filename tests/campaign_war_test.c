#include "campaign.h"
#include "campaign_war.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned combat_calls;
static int test_hit=-1;
static unsigned prop_drops;
int campaign_attack_contact(const Fighter *p){return p->cooldown&&p->hp>0;}
int arena_melee_overlap(const Fighter *p,const Fighter *q){return abs(p->x-q->x)<45&&abs(p->y-q->y)<11;}
void campaign_queue_drop(Campaign *c,int x,int y,unsigned kind,unsigned value){(void)c;(void)x;(void)y;assert(kind==DROP_FOOD&&(value&31)==10&&(value>>5)<=4);prop_drops++;}
int stage_world_width(unsigned map){return map==6?4608:1024;}
int stage_world_height(unsigned map){(void)map;return 448;}
void stage_set_map(unsigned map){(void)map;}
void stage_move(Fighter *p,int x,int y){(void)p;(void)x;(void)y;}
void campaign_cinematic_tick(Campaign *c,Arena *a){(void)c;(void)a;}
int stage_position_open_map(unsigned map,int x,int y){return x>=33&&x<stage_world_width(map)-33&&y>=180&&y<=390;}
void campaign_combat_reset(CampaignCombatState *s){memset(s,0,sizeof(*s));}
void campaign_combat_tick(Campaign *c,Arena *a,const uint16_t keys[2]){(void)keys;if(test_hit>=0){c->enemies[test_hit].hit_seq++;c->enemies[test_hit].hp--;test_hit=-1;}a->tick++;combat_calls++;}
static void dead(Campaign *c){for(int i=0;i<6;i++)if(c->enemies[i].knight_id!=3)c->enemies[i].hp=0;}
static void remove_bodies(Campaign *c){for(int i=0;i<6;i++)if(!c->enemies[i].hp)c->enemies[i].ready=0;}
int main(void){
 Campaign c={0};Arena a={0};uint16_t keys[2]={0};a.p[0]=(Fighter){.ready=1,.hp=83};a.p[1]=(Fighter){.ready=1,.hp=71};c.profiles[0].gold=123;c.potions[0]=2;
 campaign_war_start(&c,&a,6);assert(c.phase==WAR_FIGHT&&c.spawned==5&&c.war.gate_hp==301);assert(c.ally.ready&&c.enemies[5].knight_id==3);assert(a.p[0].hp==83&&c.profiles[0].gold==123&&c.potions[0]==2);
 dead(&c);campaign_war_step(&c,&a,keys);assert(c.group==1&&c.phase==WAR_TRAVEL);assert(c.enemies[0].ready); /* Bodies survive new encounter. */
 a.p[0].x=war_waypoints[3].x;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_FIGHT&&c.spawned==0);
 remove_bodies(&c);campaign_war_step(&c,&a,keys);assert(c.spawned==5);
 /* Both co-op heroes can occupy authored anchors: incoming actors must keep clear. */
 memset(c.enemies,0,sizeof(c.enemies));c.group=1;c.spawned=0;c.phase=WAR_FIGHT;a.p[0].x=818;a.p[0].y=270;a.p[1].x=805;a.p[1].y=246;
 campaign_war_step(&c,&a,keys);assert(c.spawned==5);for(int e=0;e<5;e++)for(int h=0;h<2;h++)assert(abs(c.enemies[e].x-a.p[h].x)>=56||abs(c.enemies[e].y-a.p[h].y)>=28);
 /* Seven-member source encounter queues its last member behind six bodies. */
 memset(c.enemies,0,sizeof(c.enemies));c.ally.ready=0;c.group=4;c.spawned=0;c.phase=WAR_FIGHT;a.p[0].x=1600;a.p[1].x=1400;
 campaign_war_step(&c,&a,keys);assert(c.spawned==6);dead(&c);campaign_war_step(&c,&a,keys);assert(c.spawned==6&&c.phase==WAR_FIGHT);
 c.enemies[0].ready=0;campaign_war_step(&c,&a,keys);assert(c.spawned==7&&c.enemies[0].hp>0);dead(&c);campaign_war_step(&c,&a,keys);assert(c.group==5);
 a.p[0].x=1706;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_TRAVEL&&(c.war.flags&WAR_FLAG_HORSE_DONE)); /* No forced run: control is never taken. */unsigned before=combat_calls;int held=a.p[0].x;
 for(int i=0;i<WAR_HORSE_RELEASE_TICKS;i++){campaign_war_step(&c,&a,keys);}assert(c.phase==WAR_TRAVEL&&combat_calls>before&&a.p[0].x==held);
 a.p[0].x=2415;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_MACHINE_INTRO);
 for(int i=0;i<44;i++){campaign_war_step(&c,&a,keys);}assert(!c.war.machine.ready);
 campaign_war_step(&c,&a,keys);assert(c.war.machine.hp==450);
 for(int i=45;i<126;i++){campaign_war_step(&c,&a,keys);}assert(c.phase==WAR_MACHINE_FIGHT);
 c.war.machine.hp=0;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_MACHINE_DEATH);
 for(int i=0;i<187;i++){campaign_war_step(&c,&a,keys);}assert(c.war.machine.ready);
 campaign_war_step(&c,&a,keys);assert(c.phase==WAR_TRAVEL&&!c.war.machine.ready);
 c.group=9;c.war.gate_hp=1;a.p[0].x=4500;campaign_war_step(&c,&a,keys);assert(a.map_id==6&&a.p[0].x==WAR_GATE_X-25);
 c.war.gate_hp=0;a.p[0].x=4500;campaign_war_step(&c,&a,keys);assert(a.map_id==7&&c.spawned==0&&c.phase==WAR_TRAVEL);
 a.p[0].x=337;campaign_war_step(&c,&a,keys);assert(c.spawned==3&&c.phase==WAR_FIGHT);
 dead(&c);campaign_war_step(&c,&a,keys);a.p[0].x=586;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_BRIDGE_CINEMA);
 for(int i=0;i<46;i++){campaign_war_step(&c,&a,keys);}assert(c.phase==WAR_TRAVEL);
 a.p[0].x=804;campaign_war_step(&c,&a,keys);assert(c.phase==WAR_COMPLETE&&(c.completed&2));
 assert(prop_drops==0);
 /* A single contact cuts authored grass; tree needs three separate attacks. */
 campaign_war_start(&c,&a,6);memset(c.enemies,0,sizeof(c.enemies));c.ally.ready=0;c.phase=WAR_TRAVEL;c.group=9;c.war.flags=WAR_FLAG_HORSE_DONE|WAR_FLAG_MACHINE_DONE;
 a.p[0].x=1911;a.p[0].y=238;a.p[0].cooldown=8;a.p[0].attack_kind=HIT_LIGHT;a.p[0].attack_seq=1;campaign_war_step(&c,&a,keys);assert((c.broken&4)&&prop_drops==1);campaign_war_step(&c,&a,keys);assert(prop_drops==1);
 a.p[0].x=3058;a.p[0].y=239;for(int hit=2;hit<=4;hit++){a.p[0].attack_seq=hit;campaign_war_step(&c,&a,keys);}assert((c.broken&2)&&prop_drops==2);
 /* Wrestlers must wait for slots rather than replacing six bodies. */
 for(int i=0;i<6;i++){c.enemies[i]=(Fighter){.ready=1};}c.ally=(Fighter){.ready=1,.hp=1,.knight_id=3};a.p[0].x=676;a.p[0].y=307;a.p[0].attack_seq=5;campaign_war_step(&c,&a,keys);assert((c.broken&1)&&c.war.wrestle_pending==3);
 c.enemies[0].ready=0;campaign_war_step(&c,&a,keys);assert(c.enemies[0].hp>0&&c.war.wrestle_pending==2);c.enemies[1].ready=0;campaign_war_step(&c,&a,keys);assert(!c.war.wrestle_pending&&c.enemies[1].knight_id==3);
 /* Source sleep is released by a hit, not mere proximity. */
 memset(c.enemies,0,sizeof(c.enemies));c.war.wrestle_pending=0;c.phase=WAR_FIGHT;c.group=7;c.spawned=4;c.timer=0;c.war.flags&=~WAR_FLAG_SLEEP_RELEASED;a.p[0].cooldown=0;
 for(int i=0;i<4;i++){c.enemies[i]=(Fighter){.ready=1,.hp=50,.knight_id=2};}
 for(int i=0;i<90;i++){campaign_war_step(&c,&a,keys);}for(int i=0;i<4;i++)assert(c.enemies[i].knight_id==2);
 test_hit=0;campaign_war_step(&c,&a,keys);assert((c.war.flags&WAR_FLAG_SLEEP_RELEASED)&&!c.enemies[0].knight_id);
 for(int i=0;i<6000;i++){campaign_war_step(&c,&a,keys);}for(int i=0;i<4;i++)assert(!c.enemies[i].knight_id);
 /* Source f_HitDoor door frames, shove and chips. */
 assert(campaign_war_gate_frame(151,1)==1&&campaign_war_gate_frame(142,1)==1&&campaign_war_gate_frame(135,1)==2&&campaign_war_gate_frame(113,1)==3);
 assert(campaign_war_gate_frame(75,1)==4&&campaign_war_gate_frame(37,1)==5&&campaign_war_gate_frame(1,1)==5&&campaign_war_gate_frame(0,1)==6);
 assert(campaign_war_gate_frame(301,2)==1&&campaign_war_gate_frame(0,2)==6);
 assert(campaign_war_gate_chips(1)==0&&campaign_war_gate_chips(2)==1&&campaign_war_gate_chips(3)==2&&campaign_war_gate_chips(4)==2&&campaign_war_gate_chips(5)==3&&campaign_war_gate_chips(6)==5);
 assert(campaign_war_gate_shake(0)==3&&campaign_war_gate_shake(2)==1&&campaign_war_gate_shake(3)==0&&campaign_war_gate_shake(40)==0);
 puts("War sequence: source groups, bodies, seven-member queue, cinematics, machine, gate, bridge pass");return 0;
}







