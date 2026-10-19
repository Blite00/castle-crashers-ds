#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "audio_timeline.h"
static int crossed(unsigned old,unsigned frame,unsigned cue){return old<cue&&frame>=cue;}
static uint32_t fresh(unsigned old,unsigned frame,unsigned cue,uint32_t event){
    return crossed(old,frame,cue)&&frame-cue<=3?event:0;
}
uint32_t audio_timeline_step(AudioTimeline *s,unsigned story,unsigned frame){
    uint32_t events=0;unsigned old=s->source_frame;
    int changed=!s->seen||story!=s->story||frame<old;
    if(changed){
        if(s->seen&&s->story!=AUDIO_STORY_NONE){
            events|=AUDIO_CUE_EFFECTS_STOP;
            if(s->story==AUDIO_STORY_CRYSTAL)events|=AUDIO_CUE_INTRO_STOP|AUDIO_CUE_FORCE_STOP|AUDIO_CUE_FIELD_START;
        }
        old=frame?frame-1:0;
        if(story==AUDIO_STORY_CRYSTAL){
            events|=AUDIO_CUE_INTRO_STOP; /* Suppress field music throughout. */
            if(frame<=5)events|=AUDIO_CUE_INTRO_START;
            if(frame>=172&&frame<359)events|=AUDIO_CUE_FORCE_START;
        }
    }
    if(story==AUDIO_STORY_CRYSTAL){
        if(crossed(old,frame,172)&&frame<359)events|=AUDIO_CUE_FORCE_START;
        events|=fresh(old,frame,347,AUDIO_CUE_FLY);
        if(crossed(old,frame,347))events|=AUDIO_CUE_MUSIC_FADE;
        if(crossed(old,frame,359))events|=AUDIO_CUE_INTRO_STOP|AUDIO_CUE_FORCE_STOP;
    }else if(story==AUDIO_STORY_MACHINE){
        events|=fresh(old,frame,59,AUDIO_CUE_MACHINE_INTRO);
        if((changed&&frame>=210)||crossed(old,frame,210))events|=AUDIO_CUE_MACHINE_LOOP;
    }else if(story==AUDIO_STORY_PRINCESSES){
        events|=fresh(old,frame,22,AUDIO_CUE_SCREAM)|fresh(old,frame,90,AUDIO_CUE_HELP);
    }else if(story==AUDIO_STORY_BARRACKS){
        events|=fresh(old,frame,161,AUDIO_CUE_DOOR)|fresh(old,frame,179,AUDIO_CUE_BLOCK1);
        events|=fresh(old,frame,196,AUDIO_CUE_BLOCK3)|fresh(old,frame,209,AUDIO_CUE_BLOCK3);
        events|=fresh(old,frame,219,AUDIO_CUE_PUNCH)|fresh(old,frame,231,AUDIO_CUE_GROUND2)|fresh(old,frame,293,AUDIO_CUE_GROUND5);
        if(crossed(old,frame,161))events|=AUDIO_CUE_PARTY_STOP;
    }
    s->story=(uint8_t)story;s->source_frame=(uint16_t)frame;s->seen=1;return events;
}
