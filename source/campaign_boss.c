#pragma GCC optimize("Os") /* Once-per-tick encounter logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_boss.h"
#include "boss_gold.h"
#include "campaign_scene.h"
#include "stage.h"
#include <stdlib.h>
#include <string.h>
/* The large actor is deliberately outside the nine-humanoid combat loop.
   It shares actor8/the WarState slot and never allocates another sprite cache. */
static unsigned rnd(Campaign *c,unsigned n){c->rng=c->rng*1664525u+1013904223u;return (c->rng>>8)%n;}
static void move(Fighter*p,int dx,int dy){int x=p->x,y=p->y;p->x+=dx;p->y+=dy;stage_move(p,x,y);}
static void mode(Campaign*c,unsigned n){c->war.mode=n;c->war.mode_timer=0;c->war.machine.attack_kind=n;c->war.machine.animation=0;}
unsigned campaign_boss_buried(const Campaign*c,unsigned h){return h<2&&c->phase==BOSS_FIGHT?c->break_hp[14+h]:0;}
unsigned campaign_boss_buried_frame(const Campaign*c,unsigned h){
 if(h>1||c->phase!=BOSS_FIGHT)return 0;
 unsigned age=c->break_hit_seq[15][h],remaining=c->break_hp[14+h];
 return age>=30?age+8:remaining?(age<29?age+1:38-remaining):0;
}
static void bury(Campaign*c,Arena*a){
 Fighter*m=&c->war.machine;
 for(unsigned h=0;h<2;h++){
  Fighter*p=&a->p[h];
  if(!p->ready||p->hp<=0||p->z>8||campaign_boss_buried(c,h)||abs(p->x-m->x)>70||abs(p->y-m->y)>27)continue;
  int damage=(30*(120-campaign_attributes(c,h).defense)+50)/100;
  if(arena_campaign_strike(m,p,damage,0,m->facing,0,1)&&p->hp>0){
   c->break_hp[14+h]=BOSS_BURY_WIGGLES;c->break_hit_seq[14][h]=0;c->break_hit_seq[15][h]=0;
   p->z=p->vz=p->knockback=p->hitstun=p->cooldown=p->freeze=p->burn=p->queued=p->queue_timer=0;
   p->pose=POSE_IDLE;p->animation=0;p->attack_kind=HIT_NONE;p->invulnerable=2;
  }
 }
}
static void buried_inputs(Campaign*c,Arena*a,const uint16_t keys[2],uint16_t filtered[2]){
 for(unsigned h=0;h<2;h++)if(campaign_boss_buried_frame(c,h)){
  Fighter*p=&a->p[h];filtered[h]=0;p->invulnerable=2;
  if(c->break_hit_seq[15][h]>=30){if(++c->break_hit_seq[15][h]>=34){c->break_hit_seq[15][h]=0;p->invulnerable=24;p->z=1;p->vz=6;p->pose=POSE_JUMP;p->animation=0;}continue;}
  unsigned direction=keys[h]&(LEFT|RIGHT),expected=(c->break_hp[14+h]&1)?LEFT:RIGHT;
  /* PLAYER1680 has29 entry frames, then right/left on frames30..37. */
  if(c->break_hit_seq[15][h]<29)c->break_hit_seq[15][h]++;
  else if(direction==expected){
   c->break_hp[14+h]--;p->animation++;
   if(!c->break_hp[14+h])c->break_hit_seq[15][h]=30;
  }
  c->break_hit_seq[14][h]=direction;
 }
}
static void phase(Campaign*c,unsigned n){c->phase=n;c->timer=0;if(n==BOSS_KISS)c->war.shot_timer=0;if(n!=BOSS_FIGHT)for(unsigned h=0;h<2;h++){c->break_hp[14+h]=0;c->break_hit_seq[14][h]=c->break_hit_seq[15][h]=0;}}
static void safe_place(Fighter*p,int x,int y){
 for(int d=0;d<100;d++)for(int s=-1;s<=1;s+=2)if(stage_position_open_map(BOSS_MAP,x,y+d*s)){p->x=x;p->y=y+d*s;return;}
}
void campaign_boss_start(Campaign*c,Arena*a){
 a->map_id=BOSS_MAP;a->round=BOSS_MAP;a->winner=0;a->running=1;stage_set_map(BOSS_MAP);campaign_scene_arena(a);
 memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));memset(a->shots,0,sizeof(a->shots));
 memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;campaign_combat_reset(&c->combat);
 memset(c->revive,0,sizeof(c->revive));memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->pickup_timer,0,sizeof(c->pickup_timer));
 memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));c->broken=0;c->break_tick=0;c->npc_ticks=0;c->paused=0;c->group=c->spawned=c->kills=0;c->story_flags&=6;
 phase(c,BOSS_INTRO);c->war.gate_hp=BOSS_HEALTH(a->p[1].ready?2:1);c->war.summon_timer=60;c->war.barrel_timer[0]=90;c->war.barrel_timer[1]=c->war.barrel_timer[2]=0;c->war.barrel=0;c->war.target=a->p[1].ready?rnd(c,2):0;
 c->war.machine=(Fighter){.x=808,.y=315,.hp=c->war.gate_hp,.facing=-1,.pose=POSE_WALK};
 for(int h=0;h<2;h++)if(a->p[h].ready){campaign_scene_fighter(&a->p[h]);safe_place(&a->p[h],214-h*22,316+h*20);c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};}
}
/* Source punch and reverse-spike emit one hit, not damage every contact tick.
   Its 20/10/30/10 powers are defense-scaled using existing campaign formula. */
static void strike(Campaign*c,Arena*a,int range,int lane,int power,int launch,int magic,int unblockable,int behind){
 Fighter*m=&c->war.machine;
 for(unsigned h=0;h<2;h++){
  Fighter*p=&a->p[h];int dx=(p->x-m->x)*m->facing*(behind?-1:1);
  if(!p->ready||p->hp<=0||dx< -16||dx>range||abs(p->y-m->y)>lane||p->z>80)continue;
  ArenaCampaignAttributes s=campaign_attributes(c,h);int damage=(power*(120-s.defense)+50)/100;if(damage<1)damage=1;
  if(arena_campaign_strike(m,p,damage,launch,m->facing*(behind?-1:1),magic,unblockable)){p->invulnerable=24;if(magic)p->burn=ARENA_BURN_TICKS;}
 }
 /* LEVEL7 sets damage_all=true; MAIN f_EnemyAttack checks the enemy array
    as well as heroes. Preserve normal airborne death/blink/drop handling. */
 if(!magic&&(c->war.mode==BOSS_PUNCH||c->war.mode==BOSS_SPIKES))for(unsigned e=0;e<6;e++){
  Fighter*p=&c->enemies[e];int dx=(p->x-m->x)*m->facing*(behind?-1:1);
  if(!p->ready||p->hp<=0||dx< -16||dx>range||abs(p->y-m->y)>lane||p->z>80)continue;
  if(arena_campaign_strike(m,p,power,launch,m->facing,0,unblockable)&&!p->hp){c->kills++;c->combat.death_age[e]=0;}
 }
}
static int shielded(const Campaign*c,int x){
 const Fighter*m=&c->war.machine;int facing=m->facing;
 if(c->war.mode==BOSS_SPIKES&&c->war.mode_timer>=12&&c->war.mode_timer<65)facing=-facing;
 return (x-m->x)*facing<0;
}
static void boss_damage(Campaign*c,Arena*a){
 Fighter*m=&c->war.machine;
 for(unsigned h=0;h<2;h++){
  Fighter*p=&a->p[h];if(!p->ready||p->hp<=0||!campaign_attack_contact(p)||p->attack_kind==HIT_NONE||c->war.hit_seq[h]==p->attack_seq||!arena_melee_overlap_body(p,m,60,95))continue;
  c->war.hit_seq[h]=p->attack_seq;
  if(shielded(c,p->x)){m->block_seq++;continue;}
  ArenaCampaignAttributes s=campaign_attributes(c,h);unsigned damage=(30+c->profiles[h].level+10*s.strength)/10;
  m->hp=m->hp>(int)damage?m->hp-damage:0;m->hit_seq++;m->flash=4;campaign_award_hit(c,a,h);
 }
 for(int i=0;i<2;i++){
  Projectile*p=&a->shots[i];if(!p->life||p->owner<0||p->owner>1||abs(p->x-m->x)>32+abs(p->vx)||abs(p->y-m->y)>22||p->z>100)continue;
  unsigned h=p->owner;p->life=0;
  if(shielded(c,p->x-p->vx)){m->block_seq++;continue;}
  ArenaCampaignAttributes s=campaign_attributes(c,h);unsigned damage=p->kind==SHOT_ARROW?2+s.agility:(30+c->profiles[h].level+4*s.magic)/10;
  m->hp=m->hp>(int)damage?m->hp-damage:0;m->hit_seq++;m->flash=4;campaign_award_hit(c,a,h);
 }
}
/* Source f_BarbarianBoss: one reinforcement at most every 60 walking ticks, capped at 1 above 70% boss
   health, 2 at or below 70% and 3 at or below 40%. Timers only run while he walks or stands. */
static void reinforcement(Campaign*c,Arena*a){
 if(c->war.summon_timer){c->war.summon_timer--;return;}
 unsigned alive=0,cap=c->war.machine.hp*10<=c->war.gate_hp*4?3:c->war.machine.hp*10<=c->war.gate_hp*7?2:1;
 for(int i=0;i<6;i++)alive+=c->enemies[i].ready&&c->enemies[i].hp>0;
 if(alive>=cap)return;
 for(int i=0;i<6;i++)if(!c->enemies[i].ready){
  int x=(c->war.runner_spawned++&1)?690:185,y=290+(int)rnd(c,50),safe=1;
  for(int h=0;h<2;h++)if(a->p[h].ready&&abs(a->p[h].x-x)<80&&abs(a->p[h].y-y)<35)safe=0;
  if(!safe){c->war.summon_timer=30;return;}
  c->enemies[i]=(Fighter){.hp=a->p[1].ready?55:45,.mana=40,.ready=1,.facing=x<400?1:-1,.invulnerable=12};safe_place(&c->enemies[i],x,y);
  /* LEVEL7 gives this reinforcement a specific food at spawn while
     the boss is above20% HP. Keep intent until its own corpse disappears. */
  c->break_hp[i]=c->war.machine.hp>c->war.gate_hp/5?2+rnd(c,5):0;
  c->combat.death_age[i]=c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;c->combat.target_clock[i]=0;c->war.summon_timer=60;return;
 }
}
/* f_PlayersInFront: a living hero on the boss's facing side within the given reach and lane. */
static int in_front(const Campaign*c,const Arena*a,int reach,int lane){
 const Fighter*m=&c->war.machine;
 for(unsigned h=0;h<2;h++){const Fighter*p=&a->p[h];int d=(p->x-m->x)*m->facing;if(p->ready&&p->hp>0&&d>0&&d<=reach&&abs(p->y-m->y)<=lane)return 1;}
 return 0;
}
static void fight(Campaign*c,Arena*a){
 WarState*w=&c->war;Fighter*m=&w->machine;if(m->flash)m->flash--;
 boss_damage(c,a);
 if(!m->hp){phase(c,BOSS_DEATH);mode(c,BOSS_DIE);m->pose=POSE_DEAD;c->completed|=4;
  for(int i=0;i<6;i++)if(c->enemies[i].ready&&c->enemies[i].hp){c->enemies[i].hp=0;c->enemies[i].pose=POSE_AIRHURT;c->enemies[i].z=1;c->enemies[i].vz=8;}
  return;}
 unsigned h=w->target;if(h>1||!a->p[h].ready||a->p[h].hp<=0)h=a->p[0].hp>0?0:1;w->target=h;Fighter*p=&a->p[h];
 w->mode_timer++;m->animation=w->mode_timer;m->attack_kind=w->mode;m->pose=POSE_IDLE;
 if(w->mode<=BOSS_WALK){
  int dx=p->x-m->x,dy=p->y-m->y,old_x=m->x,old_y=m->y;m->facing=dx<0?-1:1;
  if(abs(dx)>70)move(m,m->facing,0);
  if(abs(dy)>6&&!(a->tick&1))move(m,0,dy<0?-1:1);
  /* Keep the walk pose across the half-speed depth cadence, but never
     walk in place at attack distance or against a blocked boundary. */
  int walking=m->x!=old_x||m->y!=old_y||
      (abs(dy)>6&&(a->tick&1)&&stage_position_open(m->x,m->y+(dy<0?-1:1)));
  m->pose=walking?POSE_WALK:POSE_IDLE;m->attack_kind=walking?BOSS_WALK:BOSS_IDLE;
  m->animation=walking?(a->tick%36):0;
  /* Source scheduler (raw LEVEL7 bytecode): timers run only while walking or standing. */
  reinforcement(c,a);
  uint8_t*action=&w->barrel_timer[0],*punch=&w->barrel_timer[1],*fire=&w->barrel_timer[2];
  if(*punch)(*punch)--;
  if(*fire)(*fire)--;
  if(abs(dx)<=72&&abs(dy)<=6){
   /* Reaching his spot: half of the time at or below 50% health he turns and breathes fire, otherwise he punches. */
   if(m->hp*2<=(int)w->gate_hp&&rnd(c,2)==1){if(!*fire){*fire=90;mode(c,BOSS_FIRE);return;}}
   else if(!*punch){*punch=60;mode(c,BOSS_PUNCH);return;}
  }
  if(*action)(*action)--;
  /* Every 90 walking ticks alternate: shield slam toward a hero in front, then reverse spikes. */
  if(!*action){
   if(!w->barrel&&in_front(c,a,225,10)){w->barrel=1;*action=90;mode(c,BOSS_SLAM);return;}
   if(w->barrel&&in_front(c,a,250,5)){w->barrel=0;*action=90;mode(c,BOSS_SPIKES);return;}
  }
 }else if(w->mode==BOSS_PUNCH){m->pose=POSE_ATTACK;if(w->mode_timer==26)strike(c,a,92,23,20,13,0,0,0);if(w->mode_timer>=36)mode(c,BOSS_IDLE);
 }else if(w->mode==BOSS_SPIKES){m->pose=POSE_HEAVY;if(w->mode_timer==35)strike(c,a,115,20,10,5,0,0,0);if(w->mode_timer>=68)mode(c,BOSS_IDLE);
 }else if(w->mode==BOSS_SLAM){m->pose=POSE_HEAVY2;
  if(w->mode_timer==37){move(m,m->facing*123,0);bury(c,a);}
  if(w->mode_timer==38){move(m,m->facing*19,0);bury(c,a);}
  if(w->mode_timer==66)move(m,m->facing*61,0);
  if(w->mode_timer>=91){m->facing=-m->facing;mode(c,BOSS_IDLE);}
 }else if(w->mode==BOSS_FIRE){m->pose=POSE_CAST;
  if(w->mode_timer>=108&&w->mode_timer<142){if(w->mode_timer%3==0)strike(c,a,140,25,10,3,1,0,0);if(!(w->mode_timer&1))move(m,-m->facing*2,0);}
  if(w->mode_timer>=142)mode(c,BOSS_IDLE);
 }
 /* Solid boss body only at the floor; no teleport or horizontal correction
    for heroes passing in front/behind on another depth lane. */
 for(int h2=0;h2<2;h2++){Fighter*q=&a->p[h2];if(!campaign_boss_buried(c,h2)&&q->ready&&q->hp>0&&q->z<28&&abs(q->x-m->x)<32&&abs(q->y-m->y)<15)move(q,q->x<m->x?-2:2,0);}
}
/* All source chest drops share a deterministic seed and a 29-bit collected
   mask. No pickup array, extra radio bytes, allocations, or delayed queue. */
static void collect_gold(Campaign*c,Arena*a){
 for(unsigned i=0;i<BOSS_GOLD_COUNT;i++){
  BossGold gold;if(!campaign_boss_gold(c,i,&gold)||gold.age<19)continue;
  for(unsigned h=0;h<2;h++){
   Fighter*p=&a->p[h];if(!p->ready||p->hp<=0||p->z>=KEEP_PICKUP_MAX_Z||abs(p->x-gold.x)>KEEP_PICKUP_RANGE_X||abs(p->y-gold.y)>KEEP_PICKUP_RANGE_Y)continue;
   c->war.hit_seq[i/16]|=1u<<(i%16);unsigned total=c->profiles[h].gold+gold.value;c->profiles[h].gold=total>65535?65535:total;
   c->pickup_kind[h]=gold.kind;c->pickup_value[h]=gold.value;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;break;
  }
 }
}
static void treasure(Campaign*c){
 c->war.flags|=BOSS_CHEST;if(c->timer>=10)c->war.flags|=BOSS_CHEST_OPEN;
 if(c->timer>=320)phase(c,BOSS_PRINCESS);
}
static void duel_start(Campaign*c,Arena*a){
 if(!a->p[1].ready){
  /* A disconnected co-op guest cannot turn a required duel into a solo kiss.
     The room owner handles the lost connection; keep the release pending. */
  if(c->war.gate_hp==BOSS_HEALTH(2))return;
  c->group=0;phase(c,BOSS_KISS);return;
 }
 phase(c,BOSS_DUEL);campaign_scene_arena(a);memset(a->shots,0,sizeof(a->shots));a->countdown=83;
 for(int h=0;h<2;h++){Fighter*p=&a->p[h];p->hp=campaign_health(c,h);p->mana=campaign_mana(c,h);campaign_scene_fighter(p);safe_place(p,340+h*160,310);p->facing=h?-1:1;}
}
void campaign_boss_step(Campaign*c,Arena*a,const uint16_t keys[2]){
 if(c->phase==BOSS_COMPLETE)return;
 stage_set_map(BOSS_MAP);
 c->timer++;
 if(c->phase>=BOSS_TREASURE)collect_gold(c,a);
 if(c->phase==BOSS_INTRO){a->tick++;campaign_cinematic_tick(c,a);
  if(c->timer==179){c->war.machine.ready=1;mode(c,BOSS_WALK);}
  if(c->timer>=179){Fighter*m=&c->war.machine;int old_x=m->x;if(m->x>608)move(m,-2,0);int walking=m->x!=old_x;m->pose=walking?POSE_WALK:POSE_IDLE;m->attack_kind=walking?BOSS_WALK:BOSS_IDLE;m->animation=walking?a->tick%36:0;}
  if(c->timer>=299){phase(c,BOSS_FIGHT);mode(c,BOSS_IDLE);}return;
 }
 if(c->phase==BOSS_DUEL){
  if(!a->p[0].ready||!a->p[1].ready)return;
  uint16_t filtered[2]={keys[0],keys[1]};uint8_t levels[2]={c->profiles[0].level,c->profiles[1].level};
  ArenaCampaignAttributes attributes[2]={campaign_attributes(c,0),campaign_attributes(c,1)};
  for(unsigned h=0;h<2;h++){
   Fighter*p=&a->p[h];filtered[h]&=~(EQUIP|EQUIP_EDGE);
   if(!(c->tools[h]&2))filtered[h]&=~(BOW|BOW_EDGE);
   if(campaign_potion_equipped(c,h)){
    int pressed=(keys[h]&BOW_EDGE)||((keys[h]&BOW)&&!c->item_held[h]);
    filtered[h]&=~(BOW|BOW_EDGE);
    if(pressed&&p->hp>0&&p->hp<campaign_health(c,h)){unsigned old=p->hp;c->potions[h]--;p->hp=campaign_health(c,h);c->pickup_kind[h]=DROP_FOOD;c->pickup_value[h]=p->hp-old>255?255:p->hp-old;c->pickup_timer[h]=30;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;if(!c->potions[h])c->story_flags&=~(2u<<h);}
   }
   c->item_held[h]=!!(keys[h]&BOW);
  }
  arena_campaign_duel_step(a,filtered,levels,attributes);
  if(a->winner==3){a->winner=0;duel_start(c,a);return;} /* A draw awards no kiss. */
  if(a->winner){c->group=a->winner==2?1:0;a->winner=0;phase(c,BOSS_KISS);}return;}
 if(c->phase==BOSS_KISS){a->tick++;campaign_cinematic_tick(c,a);
  for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp<=0){Fighter*fallen=&a->p[h];if(fallen->z){fallen->z+=fallen->vz;if(fallen->z>80)fallen->z=80;fallen->vz--;if(fallen->vz< -16)fallen->vz=-16;if(fallen->vz>10)fallen->vz=10;if(fallen->z<=0){fallen->z=fallen->vz=0;fallen->pose=POSE_DEAD;}}else fallen->pose=POSE_DEAD;}
  Fighter*p=&a->p[c->group&1];
  if(c->timer==1){p->hitstun=p->freeze=p->burn=p->knockback=p->cooldown=0;p->queued=0;p->queue_timer=0;}
  if(c->timer<=65){
   int dx=BOSS_PRINCESS_X-BOSS_KISS_OFFSET_X-p->x,dy=BOSS_PRINCESS_Y+1-p->y;
   if(!c->war.shot_timer){
    /* Finish a combat jump with gravity before walking. Otherwise a winning
       jump attack leaves the knight suspended at its last PvP height. */
    if(p->z>0){p->z+=p->vz;if(p->z>80)p->z=80;p->vz--;if(p->vz< -16)p->vz=-16;if(p->vz>10)p->vz=10;if(p->z<=0)p->z=p->vz=0;p->pose=POSE_JUMP;p->animation=0;}
    else if(abs(dx)>2||abs(dy)>2){p->vz=0;move(p,dx>2?2:dx< -2?-2:0,dy>2?2:dy< -2?-2:0);p->pose=POSE_WALK;p->animation=a->tick%16;}
    else{p->z=p->vz=0;c->war.shot_timer=2;p->pose=POSE_IDLE;p->animation=0;}
   }else{p->z=p->vz=0;p->pose=POSE_IDLE;p->animation=0;}
   if(c->timer==65&&c->war.shot_timer!=2)c->timer=64;
   p->facing=1;
  }else{p->z=p->vz=0;p->pose=POSE_IDLE;p->animation=0;}
  if(c->timer>=207){phase(c,BOSS_COMPLETE);a->winner=1;}return;
 }
 uint16_t filtered[2]={keys[0],keys[1]};int buried_x[2],buried_y[2];
 for(unsigned h=0;h<2;h++){buried_x[h]=a->p[h].x;buried_y[h]=a->p[h].y;}
 buried_inputs(c,a,keys,filtered);campaign_combat_tick(c,a,filtered);
 for(unsigned h=0;h<2;h++)if(campaign_boss_buried_frame(c,h)){Fighter*p=&a->p[h];p->x=buried_x[h];p->y=buried_y[h];p->z=p->vz=p->knockback=0;p->pose=POSE_IDLE;p->animation=BOSS_BURY_WIGGLES-c->break_hp[14+h];}
 if(c->phase==BOSS_FIGHT){fight(c,a);return;}
 if(c->phase==BOSS_DEATH){c->war.machine.animation=c->timer;if(c->timer>=73){c->war.machine.ready=0;phase(c,BOSS_TREASURE);c->war.flags|=BOSS_CHEST;c->war.hit_seq[0]=c->war.hit_seq[1]=0;c->npc_ticks=rnd(c,256);}return;}
 if(c->phase==BOSS_TREASURE){treasure(c);return;}
 if(c->phase==BOSS_PRINCESS){Fighter post={.x=BOSS_PRINCESS_X,.y=BOSS_PRINCESS_Y,.ready=1,.hp=1};
  for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp&&campaign_attack_contact(&a->p[h])&&arena_melee_overlap_body(&a->p[h],&post,30,100)){c->spawned=h;c->war.flags|=BOSS_RESCUED;phase(c,BOSS_RELEASE);break;}
  for(int i=0;i<2&&c->phase==BOSS_PRINCESS;i++){Projectile*p=&a->shots[i];if(p->life&&p->kind==SHOT_ARROW&&p->z<100&&p->owner>=0&&p->owner<2&&abs(p->x-post.x)<20+abs(p->vx)&&abs(p->y-post.y)<20){c->spawned=p->owner;p->life=0;c->war.flags|=BOSS_RESCUED;phase(c,BOSS_RELEASE);}}
 }else if(c->phase==BOSS_RELEASE&&c->timer>=33)duel_start(c,a);
}
const char *campaign_boss_objective(const Campaign*c){
 switch(c->phase){case BOSS_INTRO:return "The Barbarian Boss";case BOSS_FIGHT:return "Defeat the Barbarian Boss";case BOSS_DEATH:return "The boss is defeated";case BOSS_TREASURE:return "Collect the treasure";case BOSS_PRINCESS:return "Cut the princess free";case BOSS_RELEASE:return "The princess is free";case BOSS_DUEL:return "Fight for the princess's kiss";case BOSS_KISS:return "The princess is rescued";default:return "Barbarian Boss complete";}
}
