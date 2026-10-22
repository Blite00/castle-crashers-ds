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
#define FLOOD_ANGRY_X 543          /* sprite 333 in sprite 548 */
#define FLOOD_ANGRY_Y 248
typedef struct {int16_t x,y;} FloodObject;
/* Campaign.species of the three enemy kinds, and their AI modes (Campaign.flood). Every flood enemy has knight_id 2, which keeps the shared enemy AI off. */
enum {FLOOD_FISH=0,FLOOD_BAT,FLOOD_CAT};
enum {FISH_WALK=0,FISH_JUMP,FISH_CHARGE,FISH_REST};
enum {CAT_WAIT=1,CAT_PICK,CAT_CHASE,CAT_PUNCH,CAT_SPIT,CAT_LANE,CAT_CHARGE};
/* Hero pose while a bat sits on the head: POSE_DIG, animation 0..4 selects the bathead frame. */
#define FLOOD_POSE_BAT POSE_DIG
void campaign_flood_start(Campaign *c,Arena *a);
void campaign_flood_step(Campaign *c,Arena *a,const uint16_t keys[2]);
/* World x of the king boat. */
int campaign_flood_boat_x(const Arena *a);
/* Health of the living Catfish, 0 when there is none. */
int campaign_flood_boss_hp(const Campaign *c);
const char *campaign_flood_objective(const Campaign *c);
/* Rocks and shark fins currently on screen. */
unsigned campaign_flood_rocks(unsigned timer,FloodObject out[FLOOD_ROCK_LIMIT]);
/* Fins run on the timeline clock (Campaign.break_tick), which stops at the checkpoints; rocks run on Campaign.timer. */
unsigned campaign_flood_fins(unsigned clock,FloodObject out[FLOOD_FIN_LIMIT]);
#endif
