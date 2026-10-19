#ifndef ARENA_H
#define ARENA_H
#include <stdint.h>
#include "knights.h"
enum { LEFT=1, RIGHT=2, UP=4, DOWN=8, ATTACK=16, JUMP=32, BLOCK=64, HEAVY=128, MAGIC=256, ATTACK_EDGE=512, HEAVY_EDGE=1024, JUMP_EDGE=2048, BOW=4096, BOW_EDGE=8192, EQUIP=16384, EQUIP_EDGE=32768, INPUT_MASK=511|BOW|EQUIP };
/* Half-resolution world: background and knights now share the same scale. */
enum { WORLD_WIDTH=424, WORLD_HEIGHT=318 };
/* MAIN true-speed threshold30; DS hold-direction trigger is an adaptation. */
enum { ARENA_RUN_TICKS=30 };
enum { ARENA_MIN_X=33, ARENA_MAX_X=391, ARENA_MIN_Y=175, ARENA_MAX_Y=307 };
enum { ARENA_HEALTH=409, ARENA_MANA=85, ARENA_SHOTS=6, ARENA_BURN_TICKS=59, ARENA_FREEZE_TICKS=30 };
enum { POSE_IDLE, POSE_WALK, POSE_ATTACK, POSE_JUMP, POSE_BLOCK, POSE_HURT,
       POSE_LIGHT2, POSE_LIGHT3, POSE_FINISHER, POSE_HEAVY, POSE_UPPERCUT,
       POSE_CAST, POSE_SPLASH, POSE_AIRATTACK, POSE_KNOCKDOWN, POSE_LAND, POSE_DEAD, POSE_AIRHURT, POSE_THROW, POSE_ROLL, POSE_AIRSPIN, POSE_CLASH, POSE_HEAVY2, POSE_BOW, POSE_AIRATTACK2, POSE_AIRHEAVY, POSE_CASTDOWN, POSE_HEAVY3, POSE_STOMP, POSE_DIG, POSE_DASHPUNCH, POSE_DASHSLAM, POSE_COUNT };
enum { HIT_NONE, HIT_LIGHT, HIT_HEAVY, HIT_LAUNCH, HIT_FINISH, HIT_AIR, HIT_CAST, HIT_SPLASH, HIT_THROW, HIT_SPIN, HIT_HEAVY2, HIT_BOW, HIT_AIRHEAVY, HIT_HEAVY3, HIT_STOMP };
enum { SHOT_ICE, SHOT_FIRE, SHOT_ARROW, SHOT_LIGHTNING, SHOT_POISON };
typedef struct {
    int16_t x,y,z,vz,hp,facing,cooldown,flash,pose,animation;
    int16_t mana,combo,combo_timer,attack_kind,hitstun,invulnerable,knockback,guard,freeze,burn;
    uint16_t previous,queued,hit_done,attack_seq,hit_seq,block_seq,magic_seq,arrow_seq;
    uint8_t queue_timer,air_spin_used,heavy_chain,float_timer,bow_equipped,equip_timer,stun_chain,stun_timer,knight_id,ready;
} Fighter;
typedef struct { int16_t x,y,z,vx,life,owner,kind,vz,fraction; } Projectile;
/* Ram cannonball: muzzle height, blast reach. Its vz field holds the landing x. */
#define ARENA_CANNON_Z0 90
#define ARENA_CANNON_RADIUS_X 40
#define ARENA_CANNON_RADIUS_Y 24
typedef struct {
    uint32_t tick; Fighter p[2]; Projectile shots[ARENA_SHOTS];
    uint16_t running,winner,countdown,round,wins[2];
    uint16_t clash_timer,clash_cooldown,clash_score[2],clash_contacts;
    uint16_t map_id;
} Arena;
int arena_fighter_running(const Fighter *p);
int arena_campaign_strike(const Fighter *attacker,Fighter *target,int damage,int launch,int direction,int magic,int unblockable);
int arena_melee_overlap_body(const Fighter *attacker,const Fighter *target,int halfwidth_twice,int height);
int arena_melee_overlap(const Fighter *attacker,const Fighter *target);
void arena_init(Arena *a);
void arena_lobby_init(Arena *a);
int arena_players_ready(const Arena *a);
int arena_select_knight(Arena *a,unsigned slot,unsigned knight,int confirmed);
void arena_start_round(Arena *a);
void arena_step(Arena *a, const uint16_t input[2]);
typedef struct {uint8_t strength,defense,magic,agility,critical_divisor;} ArenaCampaignAttributes;
void arena_campaign_duel_step(Arena *a,const uint16_t input[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2]);
void arena_group_step_attributes(Arena *a,Fighter enemies[6],Fighter *ally,const uint16_t input[9],const uint8_t targets[9],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2],uint8_t landed_hits[2]);
void arena_group_step(Arena *a,Fighter enemies[6],Fighter *ally,const uint16_t input[9],const uint8_t targets[9],const uint8_t levels[2],uint8_t landed_hits[2]);
#endif
