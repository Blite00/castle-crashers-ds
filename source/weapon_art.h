#ifndef WEAPON_ART_H
#define WEAPON_ART_H
#include <stdint.h>
#include "castle_art.h"
#include "weapon_art_shared.h"
/* Per-weapon art lives in nitrofiles/weapons/w<slot>.bin and streams in when a hero equips the weapon.
 * Three pack slots: heroes 0 and 1 hold their own weapon; slot 2 serves the ally and any other body that
 * only ever wears the starter weapon. The same calls work for any number of weapons in the catalogue. */
#define WEAPON_ART_SLOTS 3
#ifndef WEAPON_PACK_PREFIX
#ifdef ARM9
#define WEAPON_PACK_PREFIX "nitro:/weapons/"
#else
#define WEAPON_PACK_PREFIX "nitrofiles/weapons/"
#endif
#endif
typedef struct {uint16_t palette[32];uint8_t pixels[1024];} WeaponIcon;
/* Load the pack for a weapon into a slot now (normally lazy on first draw). Returns 1 when usable. */
int weapon_art_load(unsigned slot,unsigned weapon);
/* Weapon whose pack is loaded in the slot, or 255. */
unsigned weapon_art_loaded(unsigned slot);
/* Forget every loaded pack (the next draw reloads). */
void weapon_art_reset(void);
/* Held-weapon patch over knight frame `id` (0..1215). Weapons below WEAPON_PACK_FIRST_HELD reuse the base pixels. */
ArtFrame weapon_art_frame(unsigned slot,unsigned id,unsigned weapon);
int weapon_art_apply(unsigned slot,unsigned id,unsigned weapon,uint8_t *out);
/* Heavy3/stomp and dash keyframes. */
ArtFrame weapon_art_combo_frame(unsigned slot,unsigned sample,unsigned color,unsigned weapon);
int weapon_art_combo_decode(unsigned slot,unsigned sample,unsigned color,unsigned weapon,uint8_t *out);
ArtFrame weapon_art_dash_frame(unsigned slot,unsigned sample,unsigned color,unsigned weapon);
int weapon_art_dash_decode(unsigned slot,unsigned sample,unsigned color,unsigned weapon,uint8_t *out);
/* Self-contained inventory icon (own palette); cached, NULL when unavailable. */
const WeaponIcon *weapon_icon(unsigned weapon);
#endif
