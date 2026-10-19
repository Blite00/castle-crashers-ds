#ifndef RENDER_H
#define RENDER_H
#include "arena.h"
#include "campaign.h"
#include "camera.h"
const ArenaCamera *arena_render_camera(void);
int arena_render_is_opaque(void);
const ArenaCamera *arena_render_background_camera(void);
int arena_present_background(void);
void arena_upload_background(void *destination,uint16_t *palette,unsigned map);
void arena_render_invalidate_assets(void);
void arena_render_selection(uint16_t *pixels,unsigned map);
void arena_render_banner(const char *title,const char *keys);
const uint16_t *arena_dirty_left(void);
const uint16_t *arena_dirty_right(void);
void arena_render_world_map(uint16_t *pixels,const Campaign *c,const Arena *a);
void arena_render(const Arena *arena,uint16_t *pixels);
void arena_render_knight_selection(uint16_t *pixels,unsigned id);
typedef struct {unsigned own,room;int knight[2];unsigned level[2];uint8_t ready[2];const char *title,*hint;} ArenaLobby;
void arena_render_lobby(uint16_t *pixels,const ArenaLobby *lobby);
void arena_render_enemy(const Fighter *enemy);
void arena_render_campaign(const Campaign *campaign);
void arena_render_movie(uint16_t *pixels,unsigned map,unsigned kind,unsigned tick);
unsigned arena_render_terrain_rows(void);
unsigned arena_render_terrain_upload_bytes(void);
unsigned arena_render_sprite_decodes(void);
unsigned arena_render_foreground_decodes(void);
void *arena_cinematic_audio_workspace(unsigned bytes);
void *arena_barracks_audio_workspace(unsigned bytes);
#endif
