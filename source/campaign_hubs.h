#ifndef CAMPAIGN_HUBS_H
#define CAMPAIGN_HUBS_H
#include "arena.h"
struct Campaign;
enum { HUB_BLACKSMITH=8,HUB_ARK=9,HUB_FROG=10,HUB_EXPLORE=21 };
enum { HUB_STAY,HUB_WORLD_MAP };
#define HUB_BLACKSMITH_WIDTH 768
#define HUB_ARK_WIDTH 2816
#define HUB_FROG_WIDTH 3200
/* Original source portal and rack anchors, translated at half scale. */
void campaign_hubs_start(struct Campaign *campaign,Arena *arena,unsigned map,unsigned from_map);
int campaign_hubs_step(struct Campaign *campaign,Arena *arena,const uint16_t keys[2]);
/* Returns supported slot or -1. Does not imply ownership or eligibility. */
int campaign_hubs_weapon_at(const Fighter *hero);
int campaign_hubs_weapon_visible(const struct Campaign *campaign,const Arena *arena,unsigned slot);
const char *campaign_hubs_objective(unsigned map);
#endif
