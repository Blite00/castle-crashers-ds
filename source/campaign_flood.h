#ifndef CAMPAIGN_FLOOD_H
#define CAMPAIGN_FLOOD_H
#include "campaign.h"
#include "../generated/flood_geometry.h"
/* LEVEL23 Flooded Castle, DS map 15: a swim down a flooded corridor, 1000x448 world, origin (-600,-560), world=(source-origin)/2.
   Rocks, fins and the king boat are pure functions of Campaign.timer so the guest renders them from the snapshot; only heroes and
   the fish, bats and the Catfish are simulated. The enemy timeline (c->troll_timer[0]) stalls at the four checkpoints
   until every enemy is dead; c->timer keeps counting. c->group: 0 swim, 1 boss. */
#define FLOOD_ROCK_LIMIT 4
#define FLOOD_FIN_LIMIT 4
#define FLOOD_ROCK_SPEED 4          /* source 12 per tick, scaled for the DS view */
#define FLOOD_FIN_SPEED 6
typedef struct {int16_t x,y;} FloodObject;
void campaign_flood_start(Campaign *c,Arena *a);
void campaign_flood_step(Campaign *c,Arena *a,const uint16_t keys[2]);
const char *campaign_flood_objective(const Campaign *c);
/* Rocks and shark fins currently on screen. */
unsigned campaign_flood_rocks(unsigned timer,FloodObject out[FLOOD_ROCK_LIMIT]);
unsigned campaign_flood_fins(unsigned timer,FloodObject out[FLOOD_FIN_LIMIT]);
#endif
