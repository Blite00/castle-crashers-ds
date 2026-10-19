#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_weapons.h"
/* MAIN f_WeaponStats; LEVEL19 f_InitStoreItems: normal store2.item2/gold32.
   Weapons 10 and 38: LEVEL18 pickups (stats from f_WeaponStats). LEVEL13 sprite407 pickup: weapon_type9. Barbarian EnemyDropWeapon:19.
   Critical divisor100 means one random(100)==0 roll, not100 percent chance. */
static const CampaignWeapon weapons[WEAPON_COUNT]={
 {"Starting sword",1,1,0,0,0,0,0,0},
 {"Skinny Sword",2,1,32,0,-1,1,0,1},
 {"Carrot",9,5,0,0,1,1,0,0},
 {"Barbarian axe",19,1,0,100,0,0,-1,0},
 {"Thief sword",10,5,0,0,-1,0,0,3},
 {"Leaf blade",38,5,0,0,-1,0,3,0},
 {"Troll club",30,1,0,0,-1,1,1,0},
 {"Chest blade",37,1,0,0,2,0,0,-1}, /* LEVEL4 chest weapon_type37: strength +2, agility -1 */
 {"Chest mace",42,1,0,0,0,-1,-1,1}  /* LEVEL4 chest weapon_type42: agility +1, defense -1, magic -1 */
};
const CampaignWeapon *campaign_weapon_info(unsigned slot){return slot<WEAPON_COUNT?&weapons[slot]:0;}
int campaign_weapon_can_equip(const WeaponSet *owned,unsigned slot,unsigned level){return slot<WEAPON_COUNT&&weapon_set_has(owned,slot)&&level>=weapons[slot].level;}
void campaign_weapon_unlock(WeaponSet *owned,unsigned slot){if(slot<WEAPON_COUNT)weapon_set_add(owned,slot);}
unsigned campaign_weapon_cycle(const WeaponSet *owned,unsigned level,unsigned current,int direction){
 if(current>=WEAPON_COUNT)current=0;
 for(unsigned step=1;step<=WEAPON_COUNT;step++){
  unsigned candidate=(current+(direction<0?WEAPON_COUNT-step:step))%WEAPON_COUNT;
  if(campaign_weapon_can_equip(owned,candidate,level))return candidate;
 }
 return current;
}
int campaign_weapon_stat(int base,int bonus){int value=base+bonus;return value<1?1:value;}

int campaign_weapon_max_health(unsigned level,unsigned slot){
 const CampaignWeapon *w=campaign_weapon_info(slot);int defense=campaign_weapon_stat(1,w?w->defense:0);
 int hp=69+28*defense+3*(level>=1&&level<=99?level:1);return hp>409?409:hp;
}
int campaign_weapon_max_mana(unsigned slot){const CampaignWeapon *w=campaign_weapon_info(slot);return 35+5*campaign_weapon_stat(1,w?w->magic:0);}
