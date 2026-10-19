#ifndef CAMPAIGN_BOSS_H
#define CAMPAIGN_BOSS_H
#include "arena.h"
struct Campaign;
#define BOSS_MAP 11
#define BOSS_MAX_HEALTH 450
#define BOSS_HEALTH(players) (250+100*(players))
enum {BOSS_INTRO=23,BOSS_FIGHT,BOSS_DEATH,BOSS_TREASURE,BOSS_PRINCESS,BOSS_RELEASE,BOSS_DUEL,BOSS_KISS,BOSS_COMPLETE};
enum {BOSS_IDLE,BOSS_WALK,BOSS_PUNCH,BOSS_SPIKES,BOSS_SLAM,BOSS_FIRE,BOSS_DIE};
enum {BOSS_CHEST=1,BOSS_CHEST_OPEN=2,BOSS_RESCUED=4};
#define BOSS_PRINCESS_X 459
#define BOSS_PRINCESS_Y 265
/* User-directed grounded contact position. */
#define BOSS_KISS_OFFSET_X 10
#define BOSS_GOLD_COUNT 29
#define BOSS_BURY_WIGGLES 8
/* Local state uses spare boss-only prop slots; snapshots carry progress. */
unsigned campaign_boss_buried(const struct Campaign *campaign,unsigned hero);
unsigned campaign_boss_buried_frame(const struct Campaign *campaign,unsigned hero);
typedef struct {int16_t x,y;uint8_t kind,value,age;} BossGold;
void campaign_boss_start(struct Campaign *c,Arena *a);
void campaign_boss_step(struct Campaign *c,Arena *a,const uint16_t keys[2]);
const char *campaign_boss_objective(const struct Campaign *c);
#endif
