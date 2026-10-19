#ifndef CAMPAIGN_THIEVES_SCENES_H
#define CAMPAIGN_THIEVES_SCENES_H
#include "campaign.h"
#include "../generated/thieves_sfx.h"
/* Thieves Forest cutscene clips of LEVEL4 (sprites 300 intro, 235 owl, 251 deer, 138 + 313 bear and animals). The host plays one
   at a time and publishes it through the wired phase fields: while a clip runs in WAR_TRAVEL, spawned = clip + 1 and timer = clip
   frame, so a guest draws, shakes and sounds from the same numbers. Clip frames are source frames (30 per second). */
#define THIEVES_SCENE_COUNT 4
enum {THIEVES_SCENE_INTRO,THIEVES_SCENE_OWL,THIEVES_SCENE_DEER,THIEVES_SCENE_BEAR};
typedef struct {uint16_t frame;uint8_t sfx,shake;} ThievesCue; /* shake is the amplitude in world units, zero for sound only */
typedef struct {int8_t wp;int8_t art;uint16_t freeze_from,freeze_to,release;ThievesCue cues[12];} ThievesScene;
/* art: index into thieves_scene_clips, or -1 for no drawing. Explosions rumble the screen (f_ScreenShake 0.2/8, 0.3/10, 0.4/12). */
static const ThievesScene thieves_scenes[THIEVES_SCENE_COUNT]={
    {-1,0,0,100,154,{{10,THIEVES_SFX_SCREAM3,0},{56,THIEVES_SFX_HELP1,0}}},
    {2,1,0,0,431,{{35,THIEVES_SFX_FART1,0},{101,THIEVES_SFX_EXPLOSION3,2},{131,THIEVES_SFX_EXPLOSION4,3},{145,THIEVES_SFX_FART2,0},{252,THIEVES_SFX_EXPLOSION3,3},
                  {277,THIEVES_SFX_FART2,0},{295,THIEVES_SFX_EXPLOSION4,4},{377,THIEVES_SFX_MEATY4,2}}},
    {7,2,0,0,202,{{34,THIEVES_SFX_EXPLOSION3,2},{66,THIEVES_SFX_EXPLOSION4,3},{99,THIEVES_SFX_FART2,0}}},
    {17,-1,10,237,288,{{5,THIEVES_SFX_EXPLOSION3,2},{35,THIEVES_SFX_EXPLOSION4,3},{50,THIEVES_SFX_EXPLOSION3,0},{75,THIEVES_SFX_EXPLOSION3,2},{86,THIEVES_SFX_EXPLOSION4,0},
                      {105,THIEVES_SFX_EXPLOSION4,3},{127,THIEVES_SFX_EXPLOSION3,0},{151,THIEVES_SFX_EXPLOSION3,2},{164,THIEVES_SFX_FART2,0},{181,THIEVES_SFX_EXPLOSION4,3},
                      {252,THIEVES_SFX_EXPLOSION3,2},{282,THIEVES_SFX_EXPLOSION4,3}}}};
/* The clip being played, or -1. */
static inline int thieves_scene_playing(const Campaign *c){return c->phase==WAR_TRAVEL&&c->spawned>=1&&c->spawned<=THIEVES_SCENE_COUNT?(int)c->spawned-1:-1;}
static inline unsigned thieves_scene_shake(const Campaign *c){
    int s=thieves_scene_playing(c);if(s<0)return 0;
    for(unsigned i=0;i<12;i++){const ThievesCue *q=&thieves_scenes[s].cues[i];if(q->shake&&c->timer>=q->frame&&c->timer<q->frame+q->shake*3u)return q->shake;}
    return 0;
}
static inline int thieves_scene_frozen(const Campaign *c){
    int s=thieves_scene_playing(c);return s>=0&&thieves_scenes[s].freeze_to&&c->timer>=thieves_scenes[s].freeze_from&&c->timer<thieves_scenes[s].freeze_to;
}
#endif
