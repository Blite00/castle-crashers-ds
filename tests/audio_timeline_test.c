#include "audio_timeline.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    AudioTimeline s={0};unsigned starts=0,force=0,fly=0,stops=0;
    for(unsigned f=2;f<366;f++){
        unsigned e=audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,f);
        starts+=!!(e&AUDIO_CUE_INTRO_START);force+=!!(e&AUDIO_CUE_FORCE_START);fly+=!!(e&AUDIO_CUE_FLY);
        if(f==347)assert(e&AUDIO_CUE_MUSIC_FADE);
        if(f==359){assert((e&(AUDIO_CUE_INTRO_STOP|AUDIO_CUE_FORCE_STOP))==(AUDIO_CUE_INTRO_STOP|AUDIO_CUE_FORCE_STOP));stops++;}
        assert(!audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,f));
    }
    assert(starts==1&&force==1&&fly==1&&stops==1);
    unsigned e=audio_timeline_step(&s,AUDIO_STORY_NONE,0);assert(e&AUDIO_CUE_FIELD_START);
    s=(AudioTimeline){0};audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,2);
    e=audio_timeline_step(&s,AUDIO_STORY_NONE,0);assert(e&AUDIO_CUE_FORCE_STOP);assert(e&AUDIO_CUE_EFFECTS_STOP);
    s=(AudioTimeline){0};e=audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,240);
    assert((e&AUDIO_CUE_FORCE_START)&&!(e&AUDIO_CUE_INTRO_START)&&!(e&AUDIO_CUE_FLY));
    s=(AudioTimeline){0};audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,4);
    e=audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,23);assert(e&AUDIO_CUE_SCREAM);
    e=audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,89);assert(!e);
    e=audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,92);assert(e&AUDIO_CUE_HELP);
    s=(AudioTimeline){0};audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,4);
    e=audio_timeline_step(&s,AUDIO_STORY_PRINCESSES,130);assert(!(e&(AUDIO_CUE_SCREAM|AUDIO_CUE_HELP)));
    s=(AudioTimeline){0};unsigned count[6]={0};
    for(unsigned f=0;f<=486;f++){
        e=audio_timeline_step(&s,AUDIO_STORY_BARRACKS,f);
        unsigned bits[]={AUDIO_CUE_DOOR,AUDIO_CUE_BLOCK1,AUDIO_CUE_BLOCK3,AUDIO_CUE_PUNCH,AUDIO_CUE_GROUND2,AUDIO_CUE_GROUND5};
        for(unsigned j=0;j<6;j++)count[j]+=!!(e&bits[j]);
        if(f==161)assert((e&(AUDIO_CUE_PARTY_STOP|AUDIO_CUE_DOOR))==(AUDIO_CUE_PARTY_STOP|AUDIO_CUE_DOOR));
    }
    for(unsigned j=0;j<6;j++)assert(count[j]==(j==2?2u:1u));
    e=audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,2);assert(e&AUDIO_CUE_INTRO_START);
    audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,200);e=audio_timeline_step(&s,AUDIO_STORY_CRYSTAL,2);
    assert((e&AUDIO_CUE_INTRO_START)&&(e&AUDIO_CUE_FORCE_STOP));
    s=(AudioTimeline){0};unsigned intro=0,boss=0;
    for(unsigned f=0;f<=220;f++){e=audio_timeline_step(&s,AUDIO_STORY_MACHINE,f);intro+=!!(e&AUDIO_CUE_MACHINE_INTRO);boss+=!!(e&AUDIO_CUE_MACHINE_LOOP);assert(!audio_timeline_step(&s,AUDIO_STORY_MACHINE,f));}assert(intro==1&&boss==1);
    s=(AudioTimeline){0};e=audio_timeline_step(&s,AUDIO_STORY_MACHINE,215);assert(e&AUDIO_CUE_MACHINE_LOOP);assert(!(e&AUDIO_CUE_MACHINE_INTRO));
    puts("Audio timeline: exact source cues, interruption, skips, duplicates, late snapshots and restart pass");
}
