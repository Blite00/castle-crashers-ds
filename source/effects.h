#ifndef CASTLE_EFFECTS_H
#define CASTLE_EFFECTS_H
#include "arena.h"
#include "raster.h"
void effects_render_view(RasterView *view,const Arena *arena,int low,int high);
void effects_ready_view(RasterView *view,const Arena *arena,int player);
void effects_render_depth(uint16_t *pixels,const Arena *arena,int width,int height,int low,int high);
void effects_render_world(uint16_t *pixels, const Arena *arena, int width, int height);
void effects_render(uint16_t *pixels, const Arena *arena);
void effects_status_view(RasterView *view,const Fighter *fighter);
/* Ticks left of each hero's level-up beam (0 = none); fed from the local feedback state. */
extern uint8_t effects_levelup[2];
#endif
