#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_hubs.h"
#include "stage.h"
#include "campaign_scene.h"
#include <stdlib.h>
#include <string.h>
typedef struct {int16_t x,y;} HubPoint;
static const HubPoint smith_start[2]={{222,335},{193,328}},smith_ark[2]={{241,305},{212,298}},smith_frog[2]={{403,307},{438,317}};
static const HubPoint ark_start[2]={{1742,312},{1725,295}},frog_start[2]={{231,263},{203,269}};
static const HubPoint barns[2]={{1272,272},{1378,358}};
static const uint8_t barn_ids[2]={PET_SNOOT,PET_MR_BUDDY};
/* Source room15 has w2/w9/w19; starting weapon1 has no authored rack. */
static const HubPoint racks[4]={{0,0},{547,248},{971,249},{571,248}};
static void legal(Fighter *p,const Arena *a,HubPoint q){
 for(int radius=0;radius<160;radius++)for(int side=-1;side<=1;side+=2){int y=q.y+radius*side;if(stage_position_open_map(a->map_id,q.x,y)){p->x=q.x;p->y=y;return;}}
 p->x=q.x;p->y=q.y;
}
int campaign_hubs_weapon_visible(const Campaign *c,const Arena *a,unsigned slot){
 if(slot==0||slot>=4)return 0; /* Only the room's authored racks. */
 for(int h=0;h<2;h++)if(a->p[h].ready&&weapon_set_has(&c->owned_weapons[h],slot))return 1;
 return 0;
}
int campaign_hubs_weapon_at(const Fighter *p){
 int best=-1,distance=26;
 for(int slot=1;slot<4;slot++){int d=abs(p->x-racks[slot].x);if(d<distance&&abs(p->y-racks[slot].y)<40){distance=d;best=slot;}}
 return best;
}
void campaign_hubs_start(Campaign *c,Arena *a,unsigned map,unsigned from){
 if(map<HUB_BLACKSMITH||map>HUB_FROG)return;
 a->map_id=map;a->round=map;a->winner=0;a->running=1;stage_set_map(map);campaign_scene_arena(a);
 c->phase=HUB_EXPLORE;c->timer=30;c->group=c->spawned=c->kills=0;
 c->broken=c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
 memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));
 memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));memset(&c->war,0,sizeof(c->war));
 memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
 memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
 const HubPoint *start=map==HUB_ARK?ark_start:map==HUB_FROG?frog_start:from==HUB_ARK?smith_ark:from==HUB_FROG?smith_frog:smith_start;
 for(int h=0;h<2;h++)if(a->p[h].ready){campaign_scene_fighter(&a->p[h]);legal(&a->p[h],a,start[h]);c->pet_followers[h]=(PetFollower){.x=a->p[h].x-15,.y=a->p[h].y-2,.facing=1};}
 if(map==HUB_ARK)for(int n=0;n<2;n++){
  unsigned id=barn_ids[n];int owned=0,carried=0;
  for(int h=0;h<2;h++)if(a->p[h].ready){owned|=pets_owned(&c->pets[h],id);carried|=c->pets[h].equipped==id;}
  if(owned&&!carried)campaign_pet_place(c,id,barns[n].x,barns[n].y,0);
 }
}
static int near(const Fighter *p,int x,int y){return p->ready&&p->hp>0&&p->z<15&&abs(p->x-x)<24&&abs(p->y-y)<24;}
int campaign_hubs_step(Campaign *c,Arena *a,const uint16_t keys[2]){
 uint16_t filtered[2]={keys[0],keys[1]};
 if(a->map_id==HUB_FROG)for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){
  int slot=campaign_hubs_weapon_at(&a->p[h]);
  if(slot>=0&&campaign_hubs_weapon_visible(c,a,(unsigned)slot)&&(keys[h]&ATTACK_EDGE)){
   int empty=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(!c->drops[d].kind){empty=d;break;}
   unsigned old=c->weapons[h];
   if(old!=(unsigned)slot&&empty>=0&&campaign_equip(c,a,h,(unsigned)slot)){c->drops[empty]=(CampaignDrop){a->p[h].x,a->p[h].y,DROP_WEAPON,old};c->drop_age[empty]=0;}
   filtered[h]&=~(ATTACK|ATTACK_EDGE);
  }
 }
 if(a->map_id==HUB_ARK)for(int h=0;h<2;h++){
  Fighter *p=&a->p[h];unsigned id=c->pets[h].equipped;
  if(id&&p->ready&&p->hp>0&&abs(p->x-1879)<35&&abs(p->y-349)<25){
   for(int n=0;n<2;n++)if(barn_ids[n]==id&&campaign_pet_place(c,id,barns[n].x,barns[n].y,0)){c->pets[h].equipped=0;memset(&c->pet_followers[h],0,sizeof(c->pet_followers[h]));break;}
  }
 }
 campaign_combat_tick(c,a,filtered);
 if(c->timer){c->timer--;return HUB_STAY;}
 for(int h=0;h<2;h++){
  Fighter *p=&a->p[h];
  if(a->map_id==HUB_BLACKSMITH){
   if(near(p,131,332))return HUB_WORLD_MAP;
   if(near(p,195,272)){campaign_hubs_start(c,a,HUB_ARK,HUB_BLACKSMITH);return HUB_STAY;}
   if(near(p,486,281)){campaign_hubs_start(c,a,HUB_FROG,HUB_BLACKSMITH);return HUB_STAY;}
  }else if(a->map_id==HUB_ARK){
   if(near(p,1854,265)){campaign_hubs_start(c,a,HUB_BLACKSMITH,HUB_ARK);return HUB_STAY;}
  }else if(a->map_id==HUB_FROG&&near(p,152,264)){campaign_hubs_start(c,a,HUB_BLACKSMITH,HUB_FROG);return HUB_STAY;}
 }
 return HUB_STAY;
}
const char *campaign_hubs_objective(unsigned map){return map==HUB_ARK?"Animal Ark: approach owned pets":map==HUB_FROG?"A at an owned weapon to equip":"Animal Ark / weapons / world map";}



