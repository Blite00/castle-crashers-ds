#include "campaign.h"
#include "campaign_hubs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int stage_position_open_map(unsigned map,int x,int y){(void)map;return x>=0&&x<3200&&y>=200&&y<=390;}
void stage_set_map(unsigned map){(void)map;}
void campaign_combat_reset(CampaignCombatState *s){memset(s,0,sizeof(*s));}
void campaign_combat_tick(Campaign *c,Arena *a,const uint16_t k[2]){(void)c;(void)k;a->tick++;}
int campaign_pet_place(Campaign *c,unsigned id,int x,int y,int unlock){for(int i=0;i<4;i++)if(!c->drops[i].kind){c->drops[i]=(CampaignDrop){x,y,DROP_PET,id|(unlock?32:0)};return 1;}return 0;}
int campaign_equip(Campaign *c,Arena *a,unsigned h,unsigned slot){(void)a;if(!campaign_weapon_can_equip(&c->owned_weapons[h],slot,c->profiles[h].level))return 0;c->weapons[h]=slot;return 1;}
int main(void){
 Campaign c={0};Arena a={0};uint16_t k[2]={0};a.p[0]=(Fighter){.ready=1,.hp=83};c.profiles[0].level=1;c.profiles[0].gold=47;c.owned_weapons[0]=weapon_set_low(3);
 campaign_hubs_start(&c,&a,HUB_BLACKSMITH,0);assert(a.map_id==8&&c.phase==21&&a.p[0].hp==83&&c.profiles[0].gold==47);
 a.p[0].x=195;a.p[0].y=272;c.timer=0;assert(!campaign_hubs_step(&c,&a,k));assert(a.map_id==9);
 for(int i=0;i<4;i++)assert(!c.drops[i].kind); /* Never grants an unowned pet. */
 c.pets[0].owned=(1u<<17)|(1u<<24);c.pets[0].equipped=17;
 campaign_hubs_start(&c,&a,HUB_ARK,HUB_BLACKSMITH);assert(c.drops[0].kind==DROP_PET&&c.drops[0].value==24&&!c.drops[1].kind);
 a.p[0].x=1879;a.p[0].y=349;campaign_hubs_step(&c,&a,k);assert(!c.pets[0].equipped&&c.drops[1].kind==DROP_PET&&c.drops[1].value==17);
 a.p[0].x=1854;a.p[0].y=265;c.timer=0;campaign_hubs_step(&c,&a,k);assert(a.map_id==8&&a.p[0].x==241);
 a.p[0].x=486;a.p[0].y=281;c.timer=0;campaign_hubs_step(&c,&a,k);assert(a.map_id==10);
 assert(!campaign_hubs_weapon_visible(&c,&a,0)&&campaign_hubs_weapon_visible(&c,&a,1)&&!campaign_hubs_weapon_visible(&c,&a,2));
 a.p[0].x=547;a.p[0].y=270;k[0]=ATTACK_EDGE;campaign_hubs_step(&c,&a,k);assert(c.weapons[0]==1);assert(c.drops[0].kind==DROP_WEAPON&&c.drops[0].value==WEAPON_STARTER);
 c.owned_weapons[0]=weapon_set_low(15);a.p[0].x=971;a.p[0].y=270;campaign_hubs_step(&c,&a,k);assert(c.weapons[0]==1);assert(c.drops[0].kind==DROP_WEAPON&&c.drops[0].value==WEAPON_STARTER); /* Carrot level5 gate retained. */
 c.profiles[0].level=5;campaign_hubs_step(&c,&a,k);assert(c.weapons[0]==2);
 for(int d=0;d<4;d++){c.drops[d]=(CampaignDrop){500,300,DROP_COIN,1};}a.p[0].x=547;campaign_hubs_step(&c,&a,k);assert(c.weapons[0]==2);for(int d=0;d<4;d++)assert(c.drops[d].kind==DROP_COIN);
 a.p[0].x=152;a.p[0].y=264;c.timer=0;campaign_hubs_step(&c,&a,k);assert(a.map_id==8&&a.p[0].x==403);
 a.p[0].x=131;a.p[0].y=332;c.timer=0;assert(campaign_hubs_step(&c,&a,k)==HUB_WORLD_MAP);
 puts("Hub source doors, owned-pet visibility, weapon ownership and level gate pass");return 0;
}



