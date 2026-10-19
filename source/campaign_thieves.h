#ifndef CAMPAIGN_THIEVES_H
#define CAMPAIGN_THIEVES_H
#include "campaign.h"
/* LEVEL4 Thieves' Forest, DS map 13. World x is source x / 2; world y is (source y + 1350) / 2 minus a per-column shear
   (tools/thieves_shear.py). The level script is the f_Main waypoint table of the original, see docs/thieves-forest-source-audit.md. */
#define THIEVES_EVENT_COUNT 12
#define THIEVES_EXIT_DOOR_X 6440
void campaign_thieves_start(Campaign *c,Arena *a);
void campaign_thieves_step(Campaign *c,Arena *a,const uint16_t keys[2]);
const char *campaign_thieves_objective(const Campaign *c);
/* Screen lock for the current encounter (world x). Outside a fight the whole corridor is open. */
void campaign_thieves_leash(const Campaign *c,int *left,int *right);
/* The Troll Mother's health meter value (0 when not fighting her). */
int campaign_thieves_boss_hp(const Campaign *c);
/* Coin bags still lying in the level (marker position in world coordinates). */
int campaign_thieves_bag(const Campaign *c,unsigned index,int *x,int *y);
#define THIEVES_BAG_COUNT 6
/* 1 when a ledge too tall to walk up lies just ahead (the test bot jumps there). */
int campaign_thieves_wall_ahead(int x);
/* World x of level waypoint i (the cutscene clips trigger there). */
int campaign_thieves_waypoint_x(unsigned wp);
#define THIEVES_MOTHER_HEALTH 350
#endif
