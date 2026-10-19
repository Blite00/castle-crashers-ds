#ifndef WAR_SEQUENCE_H
#define WAR_SEQUENCE_H
#include <stdint.h>
/* Original LEVEL11/12 BSP/AS. World=(source-origin)/2.
   LEVEL12 is bridge continuation, NOT giant Barbarian Boss (LEVEL7).
   See docs/next-level-source-audit.md. Source anchors may need nearest legal
   foot placement where offscreen entrances lie outside the BSP corridor. */
#define WAR_MAP 6
#define BRIDGE_MAP 7
#define WAR_WIDTH 4608
#define WAR_HEIGHT 448
#define WAR_ORIGIN_X (-2112)
#define WAR_ORIGIN_Y (-700)
#define BRIDGE_WIDTH 1024
#define BRIDGE_HEIGHT 448
#define BRIDGE_ORIGIN_X (-1024)
#define BRIDGE_ORIGIN_Y (-800)
typedef struct {int16_t x,y;} WarPoint;
typedef struct {int16_t x,y;uint8_t kind;} WarSpawn;
typedef struct {uint8_t waypoint,first,count;int16_t left,right;} WarWave;
enum {WAR_BARBARIAN,WAR_THIEF,WAR_SLEEPING_BARBARIAN};
static const WarPoint war_waypoints[23]={{204,266},{306,259},{482,274},{536,272},{755,270},{788,270},{1187,335},{1234,256},{1253,286},{1439,284},{1706,284},{2042,276},{2415,285},{2674,290},{3054,294},{3276,278},{3598,278},{3742,269},{3892,282},{4100,221},{4284,224},{4400,228},{4601,226}};
static const WarPoint bridge_waypoints[6]={{168,293},{337,299},{586,290},{652,278},{687,270},{938,216}};
#define WAR_WAVE_COUNT 9
#define WAR_FIXED_ENEMY_COUNT 39
static const WarWave war_waves[WAR_WAVE_COUNT]={{0,0,5,204,482},{3,5,5,306,755},{5,10,6,755,1228},{8,16,2,755,1439},{9,18,7,1234,1682},{13,25,4,2042,3029},{14,29,4,2415,3251},{15,33,4,2674,4075},{19,37,2,3892,4400}};
static const WarSpawn war_spawns[WAR_FIXED_ENEMY_COUNT]={{356,284,0},{331,259,0},{381,244,0},{406,274,0},{431,259,0},{818,270,0},{805,246,0},{842,296,0},{868,270,0},{880,308,0},{1187,335,0},{1237,335,0},{1234,256,0},{1284,256,0},{536,262,0},{536,297,0},{1462,274,1},{1622,306,1},{1732,284,0},{1782,308,0},{1742,308,0},{1832,284,0},{1134,270,0},{1134,296,0},{1084,286,0},{3054,319,0},{3104,304,0},{3114,284,0},{3079,269,0},{3276,288,0},{3326,284,0},{2649,290,0},{2669,314,0},{3606,276,2},{3685,240,2},{3762,291,2},{3813,246,2},{4400,228,0},{4438,266,0}};
static const WarPoint war_start[2]={{185,264},{163,279}};
static const WarPoint war_allies[2]={{256,259},{281,284}};
static const WarPoint bridge_start[2]={{228,300},{206,316}};
static const WarPoint bridge_spawns[3]={{636,290},{686,316},{711,276}};
/* LEVEL11 machine movie364: spawn45, walls75..78, playerrelease126,
   cinematic branch267 ends. EWARMACH die106 releases bossgate at188. */
/* LEVEL11 case12: f_SetLeash(WP13x+160, ..., WP11x, ...) bounds heroes and camera for the
   machine fight. World=source/2, so the right edge is WP13+80, not the machine spawn. */
#define WAR_MACHINE_LEASH_LEFT 2042
#define WAR_MACHINE_LEASH_RIGHT 2754
#define WAR_CAMERA_WIDTH 424
#define WAR_MACHINE_TRIGGER_WP 12
#define WAR_MACHINE_SPAWN_X 3279
#define WAR_MACHINE_SPAWN_Y 290
#define WAR_MACHINE_INTRO_SPAWN_FRAME 45
#define WAR_MACHINE_INTRO_RELEASE_FRAME 126
#define WAR_MACHINE_INTRO_END_FRAME 267
#define WAR_MACHINE_DEATH_TICKS 188
#define WAR_MACHINE_HP(players) (250+100*(players))
#define WAR_GATE_HP(players) (150*(players))
#define WAR_GATE_X 4443
#define WAR_GATE_Y 196
#define WAR_HORSE_CINEMA_WP 10
#define WAR_HORSE_CINEMA_TICKS 245
#define BRIDGE_CINEMA_TRIGGER_WP 2
#define BRIDGE_CINEMA_TICKS 46
/* Source machine rifle barrel14 fires at9; jack59 summons42 and75. */
#define WAR_MACHINE_SHOT_FRAME 9
#define WAR_MACHINE_SUMMON_FRAME1 42
#define WAR_MACHINE_SUMMON_FRAME2 75
#endif
