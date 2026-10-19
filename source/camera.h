#ifndef CAMERA_H
#define CAMERA_H
#include "arena.h"
typedef struct { int x,y,width,height,ready; uint16_t round; } ArenaCamera;
void camera_update(ArenaCamera *camera,const Arena *arena);
/* Apply after scripted camera overrides too: streaming never reads outside art. */
void camera_clamp_world(ArenaCamera *camera,const Arena *arena);
/* Constrain scrolling, not fighter motion. Bounds are visible world edges. */
void camera_limit_horizontal(ArenaCamera *camera,const Arena *arena,int left,int right);
#endif
