#ifndef CONTENT_H
#define CONTENT_H
#include <stdint.h>
/* Content registries are addressed by small stable ids. Ownership sets are fixed-size bit sets so adding content never
 * changes a struct layout: 128 weapons, 64 pets and 64 levels today (bytes on the wire and in the save file). */
#define CONTENT_WEAPON_IDS 128u
#define CONTENT_PET_IDS 64u
#define CONTENT_LEVEL_IDS 64u
typedef struct {uint32_t w[CONTENT_WEAPON_IDS/32];} WeaponSet;
static inline int weapon_set_has(const WeaponSet *s,unsigned id){return id<CONTENT_WEAPON_IDS&&((s->w[id>>5]>>(id&31))&1u);}
static inline void weapon_set_add(WeaponSet *s,unsigned id){if(id<CONTENT_WEAPON_IDS)s->w[id>>5]|=1u<<(id&31);}
static inline WeaponSet weapon_set_low(uint32_t mask){WeaponSet s={{mask,0,0,0}};return s;}
static inline int weapon_set_equal(const WeaponSet *a,const WeaponSet *b){return a->w[0]==b->w[0]&&a->w[1]==b->w[1]&&a->w[2]==b->w[2]&&a->w[3]==b->w[3];}
static inline WeaponSet weapon_set_union(WeaponSet a,WeaponSet b){for(unsigned i=0;i<CONTENT_WEAPON_IDS/32;i++)a.w[i]|=b.w[i];return a;}
#endif
