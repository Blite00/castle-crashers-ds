#ifndef CAMPAIGN_FOREST_H
#define CAMPAIGN_FOREST_H
#include "campaign.h"
/* LEVEL18 Forest Entrance, DS map 12. World=(source-origin)/2 with origin (-256,-736), 2304x448.
   Source waypoints WP0..WP8 and portals are converted in docs/forest-entrance-source-audit.md. */
#define FOREST_WIDTH 2304
#define FOREST_CINEMA_TICKS 165
#define FOREST_EXIT_X 2120
enum {FOREST_WAVE_CROC=0,FOREST_WAVE_AMBUSH,FOREST_WAVE_ARCHERS,FOREST_WAVE_FINAL,FOREST_WAVE_EXIT};
void campaign_forest_start(Campaign *c,Arena *a);
void campaign_forest_step(Campaign *c,Arena *a,const uint16_t keys[2]);
/* LEVEL18 sprite244: weapon10 is a shovel secret at source (359.5,-340.5); weapon38 lies at (3177,-207). */
#define FOREST_DIG_X 308
#define FOREST_DIG_Y 198
/* The level's one f_RegisterTree fruit tree, sprite244 depth94 at source (3236,-243). */
#define FOREST_TREE_X 1746
#define FOREST_TREE_Y 247
int campaign_forest_dig(Campaign *c,const Arena *a,unsigned hero);
/* Source f_SetLeash bounds for the current encounter, in world x (WP0 183, WP2 628, WP4 1050, WP6 1789). */
static inline void campaign_forest_leash(const Campaign *c,int *left,int *right){
    *left=88;*right=FOREST_WIDTH-71; /* Heroes stop short of the left edge of the art so they are never half off screen. */
    if(c->phase==WAR_FIGHT||c->phase==WAR_FOREST_CINEMA){
        if(c->group<=FOREST_WAVE_AMBUSH){*left=183;*right=628;}
        else{*left=1050;*right=1789;}
    }
}
const char *campaign_forest_objective(const Campaign *c);
#endif
