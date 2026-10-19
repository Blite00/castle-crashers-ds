#ifndef CASTLE_AUDIO_H
#define CASTLE_AUDIO_H
#include "arena.h"
#include "campaign.h"
enum AudioMode { AUDIO_SILENT, AUDIO_MENU, AUDIO_ARENA, AUDIO_CAMPAIGN, AUDIO_BARRACKS, AUDIO_BARRACKS_QUIET, AUDIO_WAR, AUDIO_BLACKSMITH, AUDIO_WEAPONS, AUDIO_ARK, AUDIO_WORLDMAP, AUDIO_BARBARIAN_BOSS, AUDIO_FOREST, AUDIO_THIEVES, AUDIO_DEER, AUDIO_FLOOD };
void audio_init(void);
/* Call only after the visible menu; NitroFS also works with native ROM media. */
int audio_prepare_assets(void);
int audio_assets_available(void);
void audio_set_mode(enum AudioMode mode);
void audio_update(const Arena *arena);
void audio_toggle(void);
/* Menu cues: 0 move, 1 confirm, 2 back. */
void audio_menu_sound(unsigned kind);
int audio_enabled(void);
void audio_update_campaign(const Campaign *campaign,const Arena *arena);
void audio_update_enemy(const Fighter *enemy);
#endif
