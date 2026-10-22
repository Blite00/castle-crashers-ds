#ifndef STAGE_H
#define STAGE_H
#include "arena.h"
#define STAGE_ARENA_COUNT 3
#define STAGE_MAP_COUNT 17
#define FOREST_MAP 12
#define THIEVES_MAP 13
#define DEER_MAP 14
#define FLOOD_MAP 15
#define CAMP_MAP 16
#define CAMPAIGN_WORLD_WIDTH 2448
#define CAMPAIGN_WORLD_HEIGHT 448
#define CAMPAIGN_SHOP_WIDTH 1024
#define CAMPAIGN_SHOP_HEIGHT 448
#define CAMPAIGN_BARRACKS_WIDTH 1280
#define CAMPAIGN_BARRACKS_HEIGHT 448
int stage_min_x(void);
int stage_max_x(void);
int stage_world_width(unsigned map);
int stage_world_height(unsigned map);
int stage_camera_min_x(unsigned map);
void stage_set_map(unsigned map);
int stage_position_open_map(unsigned map,int x,int y);
const char *stage_name(unsigned map);
int stage_position_open(int x,int y);
void stage_move(Fighter *fighter,int from_x,int from_y);
int stage_segment_blocked(int from_x,int from_y,int to_x,int to_y);
int stage_projectile_blocked(int from_x,int to_x,int y);
int stage_min_y(void);
int stage_max_y(void);
/* Scripted screen-shake amplitude requested this tick (world units); camera.c consumes it. */
extern int stage_shake;
#endif
