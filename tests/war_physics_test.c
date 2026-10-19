#include "campaign.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void setup(Campaign*c,Arena*a,int coop){
 memset(c,0,sizeof(*c));arena_init(a);campaign_start(c,a,coop);campaign_war_start(c,a,WAR_MAP);memset(c->enemies,0,sizeof c->enemies);memset(&c->ally,0,sizeof c->ally);
 c->phase=WAR_MACHINE_FIGHT;c->group=5;c->war.machine=(Fighter){.x=2500,.y=290,.hp=WAR_MACHINE_HP(coop?2:1),.ready=1,.facing=-1};c->war.mode=WAR_MACHINE_WINDUP;c->war.mode_timer=1000;for(int b=0;b<3;b++)c->war.barrel_timer[b]=255;c->war.summon_timer=1000;
 a->p[0].x=2410;a->p[0].y=290;a->p[0].facing=1;a->p[1].x=2300;a->p[1].y=300;
}
static void step(Campaign*c,Arena*a,unsigned k0,unsigned k1){uint16_t keys[2]={(uint16_t)k0,(uint16_t)k1};campaign_step(c,a,keys);}
static void volley(Campaign*c,Arena*a,int kind,int owner,int y,int z){a->shots[0]=(Projectile){.x=2400,.y=y,.z=z,.vx=kind==SHOT_ARROW?16:5,.life=40,.owner=owner,.kind=kind};step(c,a,0,0);}
static void horse_to_machine(void){
 for(int coop=0;coop<2;coop++){
  Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,coop);campaign_war_start(&c,&a,WAR_MAP);
  /* The heroes walk on foot from waypoint 10 to the machine; the game never auto-runs them. */
  memset(c.enemies,0,sizeof c.enemies);memset(&c.ally,0,sizeof c.ally);c.phase=WAR_TRAVEL;c.group=5;
  for(int h=0;h<2;h++){a.p[h].x=war_waypoints[10].x+h*15;a.p[h].y=284+h*20;}
  int spawned_machine=0;
  for(int t=0;t<2000;t++){
   int prev=a.p[0].x;step(&c,&a,RIGHT,coop?RIGHT:0);
   assert(c.phase!=WAR_HORSE_CINEMA&&abs(a.p[0].x-prev)<=6); /* Normal walking speed only. */
   if(c.war.machine.ready){spawned_machine=1;assert(c.war.machine.hp==WAR_MACHINE_HP(coop?2:1));}
   if(c.phase==WAR_MACHINE_FIGHT)break;
  }
  assert(spawned_machine&&c.phase==WAR_MACHINE_FIGHT);
  assert(c.war.machine.x<WAR_MACHINE_SPAWN_X); /* Approaches during introduction. */
  CampaignPacket p;campaign_pack(&p,&c,&a,17);assert(campaign_packet_valid(&p));
 }
}
static void movement_check(void){
 Campaign c;Arena a;setup(&c,&a,0);c.war.mode=WAR_MACHINE_TRACK;c.war.mode_timer=400;
 /* Inside the source 75..125 band the machine rests. */
 a.p[0].x=2500-100;int x=c.war.machine.x;step(&c,&a,0,0);assert(c.war.machine.x==x&&c.war.machine.pose==POSE_IDLE&&c.war.machine.facing==-1);
 /* Too far right of the prey: source accelerates by 0.5 source px (0.25 here) per tick up to 4 source px (2 here). */
 a.p[0].x=2250;x=c.war.machine.x;int previous=x,peak=0;
 for(int t=0;t<20;t++){step(&c,&a,0,0);int d=previous-c.war.machine.x;assert(d>=0&&d<=2);if(d>peak)peak=d;previous=c.war.machine.x;}
 assert(c.war.machine.x<x-10&&peak==2&&c.war.speed4==-8&&c.war.machine.pose==POSE_WALK&&c.war.machine.facing==-1);
 /* It never turns to chase from the left; the prey stays on its left in tracking mode. */
 a.p[0].x=2750;x=c.war.machine.x;for(int t=0;t<20;t++)step(&c,&a,0,0);assert(c.war.machine.x>x&&c.war.machine.facing==-1);
 /* Ram left and right at the source 15 px/tick (7-8 here) without tracking vertically. */
 c.war.mode=WAR_MACHINE_LEFT;x=c.war.machine.x;int y=c.war.machine.y;step(&c,&a,0,0);assert(c.war.machine.x<x-6&&c.war.machine.y==y&&c.war.machine.pose==POSE_WALK);
 c.war.mode=WAR_MACHINE_RIGHT;x=c.war.machine.x;step(&c,&a,0,0);assert(c.war.machine.x>x+6&&c.war.machine.pose==POSE_WALK);
 /* A ram run continues past the camera and turns around 90 units beyond it, aiming at a hero. */
 setup(&c,&a,0);c.war.mode=WAR_MACHINE_LEFT;a.p[0].x=2300;a.p[0].y=310;
 for(int t=0;t<400&&c.war.mode==WAR_MACHINE_LEFT;t++)step(&c,&a,0,0);
 assert(c.war.mode==WAR_MACHINE_RIGHT&&c.war.machine.x<2300-212-90+2&&c.war.machine.y==305);
 for(int t=0;t<400&&c.war.mode==WAR_MACHINE_RIGHT;t++)step(&c,&a,0,0);
 assert(c.war.mode==WAR_MACHINE_TRACK&&c.war.mode_timer==400&&c.war.machine.facing==-1);
}
/* Source cannons: three barrels reload every 90 ticks, aimed at players. */
static void cannon_check(void){
 Campaign c;Arena a;setup(&c,&a,1);c.war.mode=WAR_MACHINE_WINDUP;c.war.mode_timer=1000;
 c.war.barrel_timer[0]=1;c.war.barrel_timer[1]=2;c.war.barrel_timer[2]=3;
 unsigned fired=0,mask=0;
 for(int t=0;t<100;t++){a.shots[0].life=a.shots[1].life=0;step(&c,&a,0,0);if(c.war.events&WAR_EVENT_CANNON){fired++;mask|=1u<<c.war.barrel;}}
 assert(fired==6&&mask==7&&c.war.barrel_timer[0]<=90);
}
int main(void){
 movement_check();
 cannon_check();
 horse_to_machine();
 /* Heroes stay inside the source fight leash (WP11 .. WP13+80); the machine remains in the world. */
 for(int mode=WAR_MACHINE_LEFT;mode<=WAR_MACHINE_SUMMON;mode++){
  Campaign bounds;Arena actors;setup(&bounds,&actors,0);bounds.war.mode=mode;bounds.war.machine.x=3400;actors.p[0].x=3279;
  step(&bounds,&actors,0,0);assert(actors.p[0].x<=WAR_MACHINE_LEASH_RIGHT&&bounds.war.machine.x<=WAR_MACHINE_SPAWN_X+60&&bounds.war.machine.x>=WAR_MACHINE_LEASH_LEFT-400);
 }
 /* Machine dimensions extend both range and height without changing human bounds. */
 Fighter swing={.x=100,.y=290,.facing=1,.pose=POSE_ATTACK,.attack_kind=HIT_LIGHT};Fighter target={.x=213,.y=290};
 assert(arena_melee_overlap_body(&swing,&target,130,95)&&!arena_melee_overlap(&swing,&target));target.x=214;assert(!arena_melee_overlap_body(&swing,&target,130,95));
 swing.pose=POSE_AIRATTACK;swing.attack_kind=HIT_AIR;swing.z=80;target.x=120;
 assert(arena_melee_overlap_body(&swing,&target,130,95)&&!arena_melee_overlap(&swing,&target));
 Campaign c;Arena a;setup(&c,&a,1);assert(c.war.machine.hp==450&&c.war.gate_hp==301);
 CampaignPacket packet;campaign_pack(&packet,&c,&a,1);assert(campaign_packet_valid(&packet));CampaignPacket bad=packet;bad.actors[8].hp=451;assert(!campaign_packet_valid(&bad));Arena replica;Campaign other={0};campaign_unpack(&packet,&other,&replica);assert(other.war.machine.hp==450);
 /* Real button input reaches the machine near edge, awards one XP and cannot repeat from its same sequence. */
 unsigned xp=c.profiles[0].xp;step(&c,&a,ATTACK|ATTACK_EDGE,0);for(int i=0;i<10;i++){step(&c,&a,0,0);}
 assert(c.war.machine.hp<450);int hp=c.war.machine.hp;assert(c.profiles[0].xp==xp+1);for(int i=0;i<20;i++){step(&c,&a,0,0);}assert(c.war.machine.hp==hp&&c.profiles[0].xp==xp+1);
 /* Facing away and a separate floor lane must miss. */
 setup(&c,&a,0);a.p[0].facing=-1;step(&c,&a,ATTACK|ATTACK_EDGE,0);for(int i=0;i<12;i++){step(&c,&a,0,0);}assert(c.war.machine.hp==350);
 setup(&c,&a,0);a.p[0].y=330;step(&c,&a,ATTACK|ATTACK_EDGE,0);for(int i=0;i<12;i++){step(&c,&a,0,0);}assert(c.war.machine.hp==350);
 setup(&c,&a,0);volley(&c,&a,SHOT_ARROW,0,290,25);assert(c.war.machine.hp==347&&!a.shots[0].life&&c.profiles[0].xp==1);step(&c,&a,0,0);assert(c.war.machine.hp==347);
 setup(&c,&a,0);volley(&c,&a,SHOT_FIRE,0,290,14);assert(c.war.machine.hp==347&&!a.shots[0].life);
 setup(&c,&a,0);volley(&c,&a,SHOT_ARROW,0,330,25);assert(c.war.machine.hp==350&&a.shots[0].life);
 setup(&c,&a,0);volley(&c,&a,SHOT_ARROW,8,290,25);assert(c.war.machine.hp==350);
 /* Front guard stops the charging machine; reverse-facing guard takes damage. */
 setup(&c,&a,0);a.p[0].x=2420;c.war.mode=WAR_MACHINE_LEFT;hp=a.p[0].hp;step(&c,&a,BLOCK,0);assert(a.p[0].hp==hp&&a.p[0].block_seq);
 setup(&c,&a,0);a.p[0].x=2420;a.p[0].facing=-1;c.war.mode=WAR_MACHINE_LEFT;hp=a.p[0].hp;step(&c,&a,BLOCK,0);assert(a.p[0].hp==hp-2&&a.p[0].invulnerable==24);hp=a.p[0].hp;step(&c,&a,0,0);assert(a.p[0].hp==hp);
 /* Ram cannonballs are lobbed: they rise above the screen, mark the landing spot and burst there, hurting everyone in the blast. */
 for(int near=0;near<2;near++){
  setup(&c,&a,1);int hp0=a.p[0].hp,hp1=a.p[1].hp;a.p[0].x=2500;a.p[0].y=290;a.p[1].x=near?2520:2300;a.p[1].y=290;
  a.shots[0]=(Projectile){.x=2700,.y=290,.z=ARENA_CANNON_Z0,.vx=(2500-2700)/39,.life=48,.owner=8,.kind=SHOT_ARROW,.vz=2500};
  CampaignPacket pk;int peak=0;
  for(int i=0;i<39;i++){step(&c,&a,0,0);if(a.shots[0].z>peak)peak=a.shots[0].z;campaign_pack(&pk,&c,&a,(uint32_t)i);assert(campaign_packet_valid(&pk));assert(a.p[0].hp==hp0&&a.p[1].hp==hp1);}
  assert(peak>150); /* High above the road. */
  step(&c,&a,0,0);if(!(a.shots[0].x==2500&&a.shots[0].z==0&&a.p[0].hp<hp0))fprintf(stderr,"shot x%d z%d life%d vx%d hp %d/%d inv%d ready%d hpz%d\n",a.shots[0].x,a.shots[0].z,a.shots[0].life,a.shots[0].vx,a.p[0].hp,hp0,a.p[0].invulnerable,a.p[0].ready,a.p[0].z);
  assert(a.shots[0].x==2500&&a.shots[0].z==0&&a.p[0].hp<hp0); /* Direct hit. */
  assert(near?a.p[1].hp<hp1:a.p[1].hp==hp1); /* Area damage, not a line. */
  for(int i=0;i<9;i++){step(&c,&a,0,0);}assert(!a.shots[0].life);
 }
 /* A lethal hit saturates at zero and starts the authored death sequence next tick. */
 setup(&c,&a,0);c.war.machine.hp=1;volley(&c,&a,SHOT_ARROW,0,290,25);assert(!c.war.machine.hp);step(&c,&a,0,0);assert(c.phase==WAR_MACHINE_DEATH);
 /* Final gate blocks walking, responds to real melee once, then permits traversal. */
 setup(&c,&a,0);c.phase=WAR_TRAVEL;c.group=WAR_WAVE_COUNT;c.war.flags=WAR_FLAG_HORSE_DONE|WAR_FLAG_MACHINE_DONE;c.war.machine.ready=0;c.war.gate_hp=8;a.p[0].x=WAR_GATE_X-25;a.p[0].y=250;a.p[0].facing=1;
 for(int i=0;i<20;i++){step(&c,&a,RIGHT,0);}assert(a.p[0].x<=WAR_GATE_X-25&&c.war.gate_hp==8);
 step(&c,&a,ATTACK|ATTACK_EDGE,0);for(int i=0;i<30;i++){step(&c,&a,0,0);}assert(c.war.gate_hp==1&&!(c.war.flags&WAR_FLAG_GATE_OPEN));
 /* Arrows remove one health but never perform the door opening callback. */
 c.war.gate_hp=3;
 for(int shot=0;shot<4;shot++){
  a.shots[0]=(Projectile){.x=WAR_GATE_X-35,.y=250,.z=25,.vx=16,.life=40,.owner=0,.kind=SHOT_ARROW};step(&c,&a,0,0);
  assert(!a.shots[0].life&&c.war.gate_hp==(shot?1:2)&&!(c.war.flags&WAR_FLAG_GATE_OPEN));
 }
 step(&c,&a,ATTACK|ATTACK_EDGE,0);for(int i=0;i<10;i++){step(&c,&a,0,0);}assert(!c.war.gate_hp&&(c.war.flags&WAR_FLAG_GATE_OPEN));
 for(int i=0;i<150&&a.map_id==WAR_MAP;i++){step(&c,&a,RIGHT,0);}assert(a.map_id==BRIDGE_MAP);
 puts("War physics: boss melee/projectiles/XP,450HP packet,front shields,death and gate traversal PASS");return 0;}

