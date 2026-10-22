#ifndef CAMPAIGN_WEAPONS_H
#define CAMPAIGN_WEAPONS_H
#include <stdint.h>
#include "content.h"
enum { WEAPON_STARTER,WEAPON_SKINNY,WEAPON_CARROT,WEAPON_BARBARIAN,WEAPON_THIEF,WEAPON_LEAF,WEAPON_TROLL,WEAPON_37,WEAPON_42,WEAPON_LOBSTER,WEAPON_UMBRELLA,WEAPON_WRENCH,WEAPON_COUNT };
typedef struct {
 const char *name;uint8_t source_id,level,price,critical_divisor;
 int8_t strength,defense,magic,agility;
} CampaignWeapon;
const CampaignWeapon *campaign_weapon_info(unsigned slot);
int campaign_weapon_can_equip(const WeaponSet *owned,unsigned slot,unsigned level);
void campaign_weapon_unlock(WeaponSet *owned,unsigned slot);
unsigned campaign_weapon_cycle(const WeaponSet *owned,unsigned level,unsigned current,int direction);
int campaign_weapon_stat(int base,int bonus);
int campaign_weapon_max_health(unsigned level,unsigned slot);
int campaign_weapon_max_mana(unsigned slot);
#endif
