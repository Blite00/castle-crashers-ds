#ifndef CAMPAIGN_WAR_H
#define CAMPAIGN_WAR_H
#include "arena.h"
#include "war_sequence.h"
struct Campaign;
enum {WAR_TRAVEL=13,WAR_FIGHT,WAR_HORSE_CINEMA,WAR_MACHINE_INTRO,
      WAR_MACHINE_FIGHT,WAR_MACHINE_DEATH,WAR_BRIDGE_CINEMA,WAR_COMPLETE,WAR_FOREST_CINEMA};
enum {WAR_MACHINE_LEFT=1,WAR_MACHINE_RIGHT,WAR_MACHINE_TRACK,WAR_MACHINE_WINDUP,WAR_MACHINE_SUMMON,WAR_MACHINE_DYING};
enum {WAR_EVENT_RAM=1,WAR_EVENT_CANNON=2,WAR_EVENT_BOMB=4};
/* Source movie runs245 ticks; DS releases after the blast/landing at168. */
#define WAR_HORSE_RELEASE_TICKS 168
enum {WAR_FLAG_HORSE_DONE=1,WAR_FLAG_MACHINE_DONE=2,WAR_FLAG_GATE_OPEN=4,WAR_FLAG_BRIDGE_STORY_DONE=8,WAR_FLAG_SLEEP_RELEASED=16};
typedef struct {
    Fighter machine;
    uint16_t mode_timer,summon_timer,shot_timer,gate_hp;
    uint16_t hit_seq[2],gate_hit_seq[2];
    uint8_t mode,flags,target,events;
    uint8_t runner_mask,runner_spawned,wrestle_pending;
    int8_t speed4,frac;uint8_t barrel,barrel_timer[3],barrel_target[3]; /* host-only machine AI state */
} WarState;
/* Starts a new map visit, preserving hero profiles/stats/inventory/health. */
void campaign_war_start(struct Campaign *campaign,Arena *arena,unsigned map);
/* Parent calls after shared drop/feedback clocks and game-over checks.
   Uses shared combat once per live tick. Cinematic ticks increment arena.tick.
   After this function, root physics consumes war.events for ram/cannon damage
   and applies hero damage to war.machine.hp and war.gate_hp. */
void campaign_war_step(struct Campaign *campaign,Arena *arena,const uint16_t keys[2]);
/* Source f_HitDoor: door frame 2..5 below 90/75/50/25 percent health, frame 6 when destroyed. */
static inline unsigned campaign_war_gate_frame(unsigned gate_hp,unsigned players){
    if(!gate_hp)return 6;
    unsigned max=WAR_GATE_HP(players?players:1),n=gate_hp-1; /* DS stores source health plus one. */
    if(n*100<max*25)return 5;
    if(n*100<max*50)return 4;
    if(n*100<max*75)return 3;
    if(n*100<max*90)return 2;
    return 1;
}
/* Source doorsmack frames 5..8 shove the door right, then settle (DS pixels). */
static inline int campaign_war_gate_shake(unsigned age){static const int8_t shove[4]={3,3,1,0};return age<4?shove[age]:0;}
/* Source f_HitDoor throws 1/2/2/3/5 door_shrapnel chips per melee hit by door frame. */
static inline unsigned campaign_war_gate_chips(unsigned frame){static const uint8_t chips[7]={0,0,1,2,2,3,5};return frame<7?chips[frame]:0;}
const char *campaign_war_objective(const struct Campaign *campaign,const Arena *arena);
#endif



