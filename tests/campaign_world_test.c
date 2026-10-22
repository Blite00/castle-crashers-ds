#include "campaign.h"
#include "campaign_save.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void packet(const Campaign *c,const Arena *a){CampaignPacket p;Campaign copy={0};Arena actors;campaign_pack(&p,c,a,77);if(!campaign_packet_valid(&p))fprintf(stderr,"invalid wire map%u phase%u group%u pos%d,%d\n",a->map_id,c->phase,c->group,a->p[0].x,a->p[0].y);assert(campaign_packet_valid(&p));campaign_unpack(&p,&copy,&actors);assert(copy.completed==c->completed&&copy.phase==c->phase&&actors.map_id==a->map_id);}
static void walk_axis(Campaign *c,Arena *a,int value,int axis,unsigned expected_map){
 for(int ticks=0;ticks<5000&&a->map_id==expected_map&&c->phase!=CAMPAIGN_WORLD_MAP;ticks++){
  Fighter *p=&a->p[0];int delta=value-(axis?p->y:p->x);if(delta>=-2&&delta<=2)return;
  uint16_t keys[2]={(uint16_t)(axis?(delta<0?UP:DOWN):(delta<0?LEFT:RIGHT)),0};campaign_step(c,a,keys);
  assert(stage_position_open_map(a->map_id,a->p[0].x,a->p[0].y));
 }
}
static void walk_door(Campaign *c,Arena *a,int x,int y,unsigned map,unsigned destination){
 walk_axis(c,a,map==10?275:340,1,map);walk_axis(c,a,x,0,map);walk_axis(c,a,y,1,map);
 if(destination&&a->map_id!=destination)fprintf(stderr,"door %u to%u stopped map%u at%d,%d target%d,%d phase%u\n",map,destination,a->map_id,a->p[0].x,a->p[0].y,x,y,c->phase);
 if(destination)assert(a->map_id==destination);else assert(c->phase==CAMPAIGN_WORLD_MAP);
 packet(c,a);
}
static void guest_preserved(const Campaign *c,const Arena *a){
 CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,99);assert(campaign_packet_valid(&p));campaign_sync_cycle(c,a,&guest,&remote,0);
 assert(remote.p[1].ready&&remote.p[1].knight_id==a->p[1].knight_id);
 assert(guest.profiles[1].gold==77&&guest.profiles[1].level==5&&guest.profiles[1].xp==10);
 assert(guest.weapons[1]==2&&guest.owned_weapons[1].w[0]==(7&15)&&guest.tools[1]==3);
 assert(guest.pets[1].equipped==PET_SNOOT);
 static const uint8_t stats[4]={1,2,0,1};assert(!memcmp(guest.stats[1],stats,4));
 assert(guest.phase==c->phase&&guest.group==c->group&&remote.map_id==a->map_id);
}
static void coop_world(void){
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);
 c.profiles[1].level=5;c.profiles[1].gold=77;c.profiles[1].xp=10;c.owned_weapons[1]=weapon_set_low(7);c.weapons[1]=2;
 c.stats[1][0]=1;c.stats[1][1]=2;c.stats[1][3]=1;c.tools[1]=3;c.pets[1]=(PetProfile){1u<<PET_SNOOT,PET_SNOOT};
 campaign_world_map(&c,&a);
 for(unsigned node=0;node<CAMPAIGN_WORLD_NODES;node++){c.group=c.spawned=node;guest_preserved(&c,&a);}
 assert(!campaign_world_enter(&c,&a,3)&&!campaign_world_enter(&c,&a,4));guest_preserved(&c,&a);
 a.p[0].hp=a.p[1].hp=0;assert(!campaign_world_enter(&c,&a,4)&&!a.p[0].hp&&!a.p[1].hp);
 assert(campaign_world_enter(&c,&a,0));assert(a.p[0].hp>0&&a.p[1].hp>0);guest_preserved(&c,&a);
 campaign_hubs_start(&c,&a,HUB_ARK,HUB_BLACKSMITH);guest_preserved(&c,&a);for(int d=0;d<4;d++)assert(c.drops[d].kind!=DROP_PET); /* Carried pet not duplicated for host. */
 campaign_hubs_start(&c,&a,HUB_FROG,HUB_BLACKSMITH);guest_preserved(&c,&a);
 campaign_world_map(&c,&a);assert(campaign_world_enter(&c,&a,1));guest_preserved(&c,&a);assert(a.p[1].ready&&a.map_id==5);
 c.completed|=1;campaign_world_map(&c,&a);assert(campaign_world_enter(&c,&a,3));guest_preserved(&c,&a);assert(a.map_id==6);
 campaign_war_start(&c,&a,7);guest_preserved(&c,&a);c.completed|=2;campaign_world_map(&c,&a);guest_preserved(&c,&a);
 assert(campaign_world_enter(&c,&a,4)&&a.map_id==BOSS_MAP);
}
static void map_walk_check(void){
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);
 a.p[0].knight_id=1;a.p[1].knight_id=3;c.completed=8;campaign_world_map(&c,&a);
 assert(c.group==1&&c.spawned==1&&!campaign_world_travelling(&c));
 int hero_x=a.p[0].x,hero_y=a.p[0].y,x,y,prior;
 campaign_world_position(&c,&x,&y);prior=x;
 assert(campaign_world_choose(&c,2));assert(!campaign_world_choose(&c,0));
 c.paused=1;unsigned paused_tick=a.tick;campaign_world_tick(&c,&a);assert(!c.timer&&a.tick==paused_tick);c.paused=0;
 assert(!campaign_world_enter(&c,&a,2));
 for(int t=0;t<29;t++){
  campaign_world_tick(&c,&a);campaign_world_position(&c,&x,&y);assert(x>=prior&&x-prior<=2);prior=x;
  CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,&c,&a,t);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&remote);
  int rx,ry;campaign_world_position(&guest,&rx,&ry);assert(rx==x&&ry==y);assert(remote.p[0].knight_id==1&&remote.p[1].knight_id==3);
  assert(a.p[0].x==hero_x&&a.p[0].y==hero_y);
 }
 assert(!campaign_world_travelling(&c)&&x==276&&y==306);
 assert(campaign_world_enter(&c,&a,2)&&a.map_id==4&&c.phase==KEEP_STORE_FIGHT);packet(&c,&a);
 assert(a.p[1].ready&&a.p[0].knight_id==1&&a.p[1].knight_id==3);
 c.paused=1;Campaign frozen=c;Arena frozen_arena=a;uint16_t keys[2]={ATTACK,ATTACK};campaign_step(&c,&a,keys);assert(!memcmp(&c,&frozen,sizeof(c))&&!memcmp(&a,&frozen_arena,sizeof(a)));c.paused=0;
 campaign_world_map(&c,&a);assert(c.group==2&&c.spawned==2);
 assert(campaign_world_choose(&c,0));while(campaign_world_travelling(&c)){campaign_world_tick(&c,&a);packet(&c,&a);}
 assert(campaign_world_enter(&c,&a,0));campaign_world_map(&c,&a);assert(c.group==0);
 CampaignPacket bad;Campaign paused_copy={0};Arena paused_arena;c.paused=1;campaign_pack(&bad,&c,&a,1);assert(campaign_packet_valid(&bad)&&bad.running==11);campaign_unpack(&bad,&paused_copy,&paused_arena);assert(paused_copy.paused&&paused_arena.running==1);bad.running=32;assert(!campaign_packet_valid(&bad));c.paused=0;campaign_pack(&bad,&c,&a,1);bad.spawned=10;assert(!campaign_packet_valid(&bad));bad.spawned=0;bad.timer=1;assert(!campaign_packet_valid(&bad));
}
static void continuous_chapters(void){
 for(unsigned ending=0;ending<2;ending++){
  Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);
  c.profiles[1].level=5;c.profiles[1].gold=77;c.profiles[1].xp=10;c.owned_weapons[1]=weapon_set_low(7);c.weapons[1]=2;
  c.stats[1][0]=1;c.stats[1][1]=2;c.stats[1][3]=1;c.tools[1]=3;c.pets[1]=(PetProfile){1u<<PET_SNOOT,PET_SNOOT};
  c.completed|=8;campaign_world_map(&c,&a);assert(campaign_world_enter(&c,&a,2));
  a.p[0].hp=66;a.p[1].hp=77;a.p[0].mana=19;a.p[1].mana=23;a.p[0].knight_id=2;a.p[1].knight_id=3;
  c.phase=ending?KEEP_ENDING:KEEP_DOOR;c.timer=ending?89:17;
  uint16_t keys[2]={0};campaign_step(&c,&a,keys);
  assert(a.running&&!a.winner&&a.map_id==WAR_MAP&&c.phase==WAR_FIGHT&&(c.completed&1));
  assert(a.p[0].hp==66&&a.p[1].hp==77&&a.p[0].mana==19&&a.p[1].mana==23);
  assert(a.p[0].knight_id==2&&a.p[1].knight_id==3);guest_preserved(&c,&a);
  CampaignPacket p;Campaign remote={0};Arena guests;campaign_pack(&p,&c,&a,4);assert(campaign_packet_valid(&p));campaign_unpack(&p,&remote,&guests);
  assert(guests.p[0].hp==66&&guests.p[1].hp==77&&guests.map_id==WAR_MAP&&!guests.winner);
 }
}
static void reset_check(void){
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);c.menu_ack[0]=7;c.menu_ack[1]=12;c.completed=3;c.owned_weapons[1]=weapon_set_low(3);c.weapons[1]=1;
 for(int room=6;room<=10;room++){
  c.broken=511;c.break_tick=77;c.story_flags=255;c.dig_timer[0]=19;c.revive[1]=30;c.pickup_timer[0]=30;c.item_held[0]=255;c.break_hp[2]=9;
  a.clash_timer=50;a.countdown=9;a.p[0].cooldown=20;a.p[0].hitstun=19;a.p[0].freeze=20;a.p[0].burn=30;a.p[0].knockback=5;a.p[0].queued=ATTACK;a.p[0].z=42;a.p[0].pose=POSE_HURT;
  int hp=a.p[0].hp;
  if(room<8)campaign_war_start(&c,&a,room);else campaign_hubs_start(&c,&a,room,0);
  assert(!c.broken&&!c.break_tick&&c.story_flags==6&&!c.dig_timer[0]&&!c.revive[1]&&!c.pickup_timer[0]&&!c.item_held[0]&&!c.break_hp[2]);
  assert(!a.clash_timer&&!a.countdown&&!a.p[0].cooldown&&!a.p[0].hitstun&&!a.p[0].freeze&&!a.p[0].burn&&!a.p[0].knockback&&!a.p[0].queued&&!a.p[0].z&&a.p[0].hp==hp);
  assert(c.menu_ack[0]==7&&c.menu_ack[1]==12&&c.completed==3&&c.owned_weapons[1].w[0]==3&&c.weapons[1]==1);packet(&c,&a);
 }
}
static void stress_snapshots(void){
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);uint32_t r=12345;
 for(int visit=0;visit<30;visit++){
  a.p[0].hp=a.p[1].hp=100;campaign_war_start(&c,&a,6);
  if(visit%3){
   memset(c.enemies,0,sizeof(c.enemies));memset(&c.ally,0,sizeof(c.ally));c.group=5;c.spawned=0;c.timer=0;
   c.phase=visit%3==1?WAR_HORSE_CINEMA:WAR_MACHINE_INTRO;
   for(int h=0;h<2;h++){a.p[h].x=(visit%3==1?1706:2415)+h*25;a.p[h].y=284+h*20;}
  }
  for(int tick=0;tick<1000;tick++){
   uint16_t keys[2];for(int h=0;h<2;h++){
    r=r*1664525u+1013904223u;unsigned v=r>>24;
    keys[h]=(v&1?LEFT:RIGHT)|(v&2?UP:DOWN);
    if(v&4)keys[h]|=ATTACK|ATTACK_EDGE;
    if(v&8)keys[h]|=HEAVY|HEAVY_EDGE;
    if(v&16)keys[h]|=BLOCK;
    if(v&32)keys[h]|=JUMP|JUMP_EDGE;
    if(a.p[h].hp<=0){a.p[h].hp=100;a.p[h].pose=POSE_IDLE;a.p[h].hitstun=0;}
   }
   a.winner=0;campaign_step(&c,&a,keys);
   CampaignPacket p;campaign_pack(&p,&c,&a,tick);
   if(!campaign_packet_valid(&p)){fprintf(stderr,"stress visit%d tick%d phase%u timer%u hero%d,%d z%d burn%d\n",visit,tick,c.phase,c.timer,a.p[0].x,a.p[0].y,a.p[0].z,a.p[0].burn);for(int i=0;i<9;i++)fprintf(stderr," actor%d %d,%d hp%d flags%u z%u anim%u kind%u\n",i,p.actors[i].x,p.actors[i].y,p.actors[i].hp,p.actors[i].flags,p.actors[i].z,p.actors[i].animation,p.actors[i].attack_kind);assert(0);}
   Campaign guest={0};Arena remote;campaign_unpack(&p,&guest,&remote);assert(remote.p[1].ready&&guest.phase==c.phase);
  }
 }
}
static void sequential_locks(void){
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);campaign_world_map(&c,&a);
 assert(campaign_world_unlocked(&c,0)&&campaign_world_unlocked(&c,1));
 for(unsigned node=2;node<CAMPAIGN_WORLD_NODES;node++){
  assert(!campaign_world_unlocked(&c,node));Campaign before=c;Arena actors=a;
  assert(!campaign_world_enter(&c,&a,node));assert(!memcmp(&c,&before,sizeof c)&&!memcmp(&a,&actors,sizeof a));
 }
 /* Selection/travel never unlocks entry. Guests receive the same lock state. */
 assert(campaign_world_choose(&c,2));while(campaign_world_travelling(&c))campaign_world_tick(&c,&a);
 assert(!campaign_world_enter(&c,&a,2));packet(&c,&a);
 /* Finish Home Castle through its actual continuous transition. */
 a.map_id=3;stage_set_map(3);c.phase=KEEP_TRAVEL;c.group=4;c.spawned=0;
 for(int h=0;h<2;h++){a.p[h].x=2385+h*2;for(int y=0;y<stage_world_height(3);y++)if(stage_position_open_map(3,a.p[h].x,y)){a.p[h].y=y;break;}}
 uint16_t keys[2]={0};campaign_step(&c,&a,keys);
 assert(c.completed==8&&a.map_id==4&&c.phase==KEEP_STORE_FIGHT);packet(&c,&a);
 assert(campaign_world_unlocked(&c,2)&&!campaign_world_unlocked(&c,3)&&!campaign_world_unlocked(&c,4));
 CampaignCharacter save;campaign_character_get(&save,&c,0);assert(save.cleared==8&&campaign_character_valid(&save));
 Campaign restored={0};campaign_character_put(&save,&restored,0);assert(restored.completed==8&&campaign_world_unlocked(&restored,2)&&!campaign_world_unlocked(&restored,3));
 campaign_world_map(&c,&a);assert(campaign_world_enter(&c,&a,2));
 /* Completing Keep opens War while preserving Home Castle progress. */
 c.phase=KEEP_DOOR;c.timer=17;campaign_step(&c,&a,keys);assert(c.completed==9&&a.map_id==WAR_MAP);packet(&c,&a);
 assert(campaign_world_unlocked(&c,3)&&!campaign_world_unlocked(&c,4));
 /* Older saves may only contain a later bit: never relock predecessors. */
 for(unsigned flags=1;flags<=7;flags++){
  c.completed=flags;assert(campaign_world_unlocked(&c,2)&&campaign_world_unlocked(&c,3));
  assert(campaign_world_unlocked(&c,4)==!!(flags&6));
 }
}
int main(void){
 sequential_locks();
 map_walk_check();continuous_chapters();reset_check();stress_snapshots();
 coop_world();
 Campaign c={0};Arena a;arena_init(&a);a.p[1].ready=0;campaign_start(&c,&a,0);campaign_world_map(&c,&a);assert(!c.completed);packet(&c,&a);
 assert(!campaign_world_enter(&c,&a,3));assert(!campaign_world_enter(&c,&a,4));assert(c.phase==CAMPAIGN_WORLD_MAP);
 assert(campaign_world_enter(&c,&a,0));packet(&c,&a);
 walk_door(&c,&a,195,272,8,9);for(int i=0;i<4;i++)assert(c.drops[i].kind!=DROP_PET);
 walk_door(&c,&a,1854,265,9,8);walk_door(&c,&a,486,281,8,10);walk_door(&c,&a,152,264,10,8);walk_door(&c,&a,131,332,8,0);
 assert(campaign_world_enter(&c,&a,1));assert(a.map_id==5&&c.phase==KEEP_BARRACKS_INTRO);
 /* The full Keep combat route has its own test. Exercise its real final transition. */
 c.phase=KEEP_DOOR;c.timer=17;uint16_t keys[2]={0};campaign_step(&c,&a,keys);assert((c.completed&1)&&!a.winner&&a.map_id==WAR_MAP&&c.phase==WAR_FIGHT);packet(&c,&a);
 campaign_world_map(&c,&a);packet(&c,&a);assert(campaign_world_enter(&c,&a,3));assert(a.map_id==6&&c.phase==WAR_FIGHT);packet(&c,&a);
 /* War wave/machine route is independently tested; exercise bridge completion and save. */
 campaign_war_start(&c,&a,7);memset(c.enemies,0,sizeof(c.enemies));c.group=1;c.phase=WAR_TRAVEL;c.war.flags=WAR_FLAG_BRIDGE_STORY_DONE;a.p[0].x=804;a.p[0].y=270;
 campaign_step(&c,&a,keys);assert(c.phase==BOSS_INTRO&&a.map_id==BOSS_MAP&&c.completed==3);packet(&c,&a);campaign_world_map(&c,&a);packet(&c,&a);
 CampaignCharacter saved;campaign_character_get(&saved,&c,0);assert(saved.cleared==3&&campaign_character_valid(&saved));Campaign restored={0};campaign_character_put(&saved,&restored,0);assert(restored.completed==3);
 assert(campaign_world_enter(&c,&a,4)&&a.map_id==BOSS_MAP); /* Replay the unlocked boss from the map. */
 puts("World flow: source hub doors walked, chapter unlocks, radio snapshots and saved clear bits pass");return 0;
}





