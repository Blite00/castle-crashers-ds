#ifndef CAMPAIGN_DEER_H
#define CAMPAIGN_DEER_H
#include "campaign.h"
#include "../generated/deer_layout.h"
/* LEVEL22 (DS map 14): the deer ride through the mill, two players. Everything that moves on its own is a pure function of
   Campaign.timer (one tick per source frame), so the guest renders it from the 276-byte snapshot without extra wire fields:
   the camera guide, the Mill Monster, the logs and the scene script. Only the heroes (position, pose, hp) are simulated.
   World units are DS pixels (source/2); speeds are in 1/16 of a pixel per tick.
   Timeline: INTRO (timer 0..315, the entrance clip), CHASE (waves of logs, the guide loops over one tile of the flume),
   FINALE (the guide runs on to the dock), DOCK (the dock jump, then the level is complete). c->group holds the stage. */
enum {DEER_STAGE_INTRO=0,DEER_STAGE_CHASE,DEER_STAGE_FINALE,DEER_STAGE_DOCK};
#define DEER_INTRO_TICKS 316
#define DEER_GUIDE_START 330        /* guide x when the chase begins */
#define DEER_HERO_OFFSET 114        /* heroes start this far ahead of the guide */
#define DEER_CAMERA_AHEAD 64       /* the camera centres this far ahead of the guide */
#define DEER_MASH_LINE (-37)        /* source: a hero more than 75 units behind the guide is tossed by the monster */
#define DEER_LOG_SPAWN_AHEAD 772    /* source: logs reappear at x 1600 beyond the camera */
#define DEER_LOG_LIMIT 6
#define DEER_STORY_POOP 16          /* story_flags bit: the second player's deer is the one that poops (source: random(3)+2 of deer 2..4) */
#define DEER_WAVE_COUNT 21
#define DEER_DOCK_X 3458            /* the dock jump point (source sprite 210) */
typedef struct {int16_t x,y;uint8_t kind,debris;} DeerLog;
void campaign_deer_start(Campaign *c,Arena *a);
void campaign_deer_step(Campaign *c,Arena *a,const uint16_t keys[2]);
/* World x of the camera guide and the stage at timer t. */
int campaign_deer_guide(const Campaign *c);
unsigned campaign_deer_stage(const Campaign *c);
/* The Mill Monster: group frame and x (its origin), or -1 when hidden. */
int campaign_deer_monster_key(const Campaign *c,int *x);
/* Logs currently on screen, in world coordinates; returns the count. */
unsigned campaign_deer_logs(const Campaign *c,DeerLog out[DEER_LOG_LIMIT]);
/* Source screen shake (world units) of the current tick. */
unsigned campaign_deer_shake(const Campaign *c);
/* Gap rows of a wall log (kinds 10..12): the lane the hero must be in. */
int campaign_deer_gap(unsigned kind,int *top,int *bottom);
/* Camera lock: the screen centre. */
#define DEER_OVERLAY_X 158          /* entrance wall, sprite 208 at source (-1133,-927) */
#define DEER_OVERLAY_Y 261
#define DEER_ARM_BEHIND 66          /* sprite 194 sits 133 source units (66 pixels) behind the monster origin */
#define DEER_ARM_Y 182
/* The cinematic camera of the entrance clip (sprite 234 guide): on the party, then left to the monster and its wall, then back for the jump. */
int campaign_deer_camera_x(const Campaign *c);
const char *campaign_deer_objective(const Campaign *c);
/* Scene sounds: the first cue after tick `after` up to tick `now`, as (tick<<8)|id with id 1..6 (see tools/convert_deer_sfx.py), or 0. */
unsigned campaign_deer_cue(unsigned after,unsigned now);
/* Hero pose keys on the wire: see campaign_deer.c. */
#define DEER_POSE_RUN POSE_WALK
#define DEER_POSE_JUMP POSE_JUMP
#define DEER_POSE_SLAM POSE_FINISHER
#define DEER_POSE_TOSS POSE_HEAVY
#define DEER_POSE_MOUNT POSE_UPPERCUT
#define DEER_POSE_DOCK POSE_LIGHT3
#define DEER_POSE_SCARED POSE_CAST
#endif
