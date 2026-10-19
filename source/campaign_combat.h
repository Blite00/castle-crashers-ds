#ifndef CAMPAIGN_COMBAT_H
#define CAMPAIGN_COMBAT_H
#include "arena.h"
/* Host-only decisions; clients receive the resulting actor poses/positions. */
typedef struct {
    uint16_t observed_attack[6];
    uint8_t death_age[7];
    uint8_t guard[6],shot_wait[6],ranged[6];
    uint8_t windup[6],recovery[6],target_clock[6],mercy[3],pressure[3],ally_windup,ally_recovery;
} CampaignCombatState;
static inline int campaign_npc_visible(const Fighter *p){
    if(!p->ready)return 0;
    if(p->hp||p->z||p->pose!=POSE_DEAD)return 1;
    unsigned frame=p->animation;
    if(frame>=44)return 0;
    if(frame>=33)return frame&1;
    return frame!=19&&frame!=23&&frame!=26&&frame!=29&&frame!=32;
}
void campaign_combat_reset(CampaignCombatState *state);
void campaign_combat_step(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],uint8_t landed_hits[2]);
void campaign_combat_step_attributes(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2],uint8_t landed_hits[2]);
void campaign_combat_step_weapons(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],const uint8_t weapons[2],uint8_t landed_hits[2]);
#endif
