/* Renderer-only test builds do not link the level script; the cutscene art and bag markers have nothing to ask. */
#include "campaign_thieves.h"
int campaign_thieves_waypoint_x(unsigned wp){(void)wp;return 0;}
void campaign_thieves_leash(const Campaign *c,int *left,int *right){(void)c;*left=0;*right=0;}
int campaign_thieves_bag(const Campaign *c,unsigned index,int *x,int *y){(void)c;(void)index;(void)x;(void)y;return 0;}
#include "campaign_deer.h"
int campaign_deer_guide(const Campaign *c){(void)c;return DEER_GUIDE_START;}
unsigned campaign_deer_stage(const Campaign *c){(void)c;return DEER_STAGE_INTRO;}
int campaign_deer_monster_key(const Campaign *c,int *x){(void)c;*x=0;return -1;}
unsigned campaign_deer_logs(const Campaign *c,DeerLog out[DEER_LOG_LIMIT]){(void)c;(void)out;return 0;}
unsigned campaign_deer_shake(const Campaign *c){(void)c;return 0;}
int campaign_deer_camera_x(const Campaign *c){(void)c;return DEER_GUIDE_START;}
void campaign_tally_values(const Campaign *c,unsigned hero,unsigned out[3]){(void)c;(void)hero;out[0]=7;out[1]=120;out[2]=45;}
#include "campaign_flood.h"
unsigned campaign_flood_rocks(unsigned timer,FloodObject out[FLOOD_ROCK_LIMIT]){(void)timer;(void)out;return 0;}
unsigned campaign_flood_fins(unsigned timer,FloodObject out[FLOOD_FIN_LIMIT]){(void)timer;(void)out;return 0;}
