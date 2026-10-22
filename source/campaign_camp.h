#ifndef CAMPAIGN_CAMP_H
#define CAMPAIGN_CAMP_H
#include "campaign.h"
#include "../generated/camp_geometry.h"
/* LEVEL20 (DS map 16, Tall Grass Field): a long beach-to-fortress route, 6450 x 448 DS, origin (-4700,-800), world=(source-origin)/2.
   Waypoints WP0..WP21 come from BSP20 (generated/camp_geometry.h) and the encounters from LEVEL20 f_Main (docs/level20-source-audit.md).
   c->group is the next encounter; c->phase is WAR_TRAVEL between encounters, WAR_FIGHT inside one and WAR_COMPLETE at the end. */
enum {CAMP_EV_HUTS=0,CAMP_EV_ROW,CAMP_EV_BOSS,CAMP_EV_PROPELLER,CAMP_EV_LEFT,CAMP_EV_STORY,CAMP_EV_BATS,CAMP_EV_EXIT,CAMP_EVENTS};
/* Sprite 244: the Necromancer's minions carry the princesses away (tick clock of the clip is 12 ticks per baked frame). */
#define CAMP_STORY_TICKS 222
#define CAMP_STORY_X 5640
#define CAMP_STORY_Y 112
void campaign_camp_start(Campaign *c,Arena *a);
void campaign_camp_step(Campaign *c,Arena *a,const uint16_t keys[2]);
const char *campaign_camp_objective(const Campaign *c);
/* 1 while the boss fight (DRUMLIFT) is on. */
unsigned campaign_camp_boss_music(const Campaign *c);
/* Waypoint x of the encounter, and the heroes' leash (world x) while it is on. */
int campaign_camp_event_x(unsigned event);
static inline void campaign_camp_leash(const Campaign *c,int *left,int *right){
    *left=CAMP_EDGE_LEFT_X+10;*right=CAMP_WIDTH-40;
    if(c->phase==WAR_FIGHT&&c->group<CAMP_EVENTS){int x=campaign_camp_event_x(c->group);*left=x-190;*right=x+(c->group==CAMP_EV_BOSS?300:110);if(c->group==CAMP_EV_STORY){*left=x-400;*right=x+300;}}
}
#endif
