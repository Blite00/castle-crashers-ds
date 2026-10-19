#ifndef AUDIO_TIMELINE_H
#define AUDIO_TIMELINE_H
#include <stdint.h>
enum AudioStory { AUDIO_STORY_NONE, AUDIO_STORY_BARRACKS, AUDIO_STORY_CRYSTAL, AUDIO_STORY_PRINCESSES, AUDIO_STORY_MACHINE };
enum AudioStoryEvent {
    AUDIO_CUE_INTRO_START=1u<<0, AUDIO_CUE_INTRO_STOP=1u<<1,
    AUDIO_CUE_FORCE_START=1u<<2, AUDIO_CUE_FORCE_STOP=1u<<3,
    AUDIO_CUE_FLY=1u<<4, AUDIO_CUE_SCREAM=1u<<5, AUDIO_CUE_HELP=1u<<6,
    AUDIO_CUE_EFFECTS_STOP=1u<<7, AUDIO_CUE_FIELD_START=1u<<8,
    AUDIO_CUE_MUSIC_FADE=1u<<9, AUDIO_CUE_DOOR=1u<<10,
    AUDIO_CUE_BLOCK1=1u<<11, AUDIO_CUE_BLOCK3=1u<<12,
    AUDIO_CUE_PUNCH=1u<<13, AUDIO_CUE_GROUND2=1u<<14,
    AUDIO_CUE_GROUND5=1u<<15, AUDIO_CUE_PARTY_STOP=1u<<16, AUDIO_CUE_MACHINE_INTRO=1u<<17, AUDIO_CUE_MACHINE_LOOP=1u<<18
};
typedef struct { uint16_t source_frame;uint8_t story,seen; } AudioTimeline;
/* Source frames, not scaled movie image keys. Snapshot gaps emit fresh cues
 * once; old one-shots are never replayed by a late snapshot or a duplicate. */
uint32_t audio_timeline_step(AudioTimeline *state,unsigned story,unsigned source_frame);
#endif
