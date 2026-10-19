#ifndef CASTLE_HUD_H
#define CASTLE_HUD_H
#include <stdint.h>
#include "arena.h"
void hud_render(uint16_t *pixels, const Arena *arena);
void hud_render_item(uint16_t *pixels, const Fighter *player);
void hud_render_shovel(uint16_t *pixels,int equipped);
void hud_render_potions(uint16_t *pixels,unsigned count,int equipped);
int hud_item_hit(int x,int y);
unsigned hud_progress_percent(unsigned level,unsigned xp);
void hud_render_progress(uint16_t *pixels,unsigned player,unsigned level,unsigned xp);
void hud_render_level_notice(uint16_t *pixels,unsigned level,const char *combo_keys,unsigned timer);
/* Original boss meter, drawn inside the bottom-screen info panel below its title. */
#define BOSS_METER_LEFT 51
#define BOSS_METER_TOP 80
void hud_render_boss(uint16_t *pixels,unsigned hp,unsigned max_hp);
void hud_render_weapon(uint16_t *pixels,unsigned slot);
#endif
