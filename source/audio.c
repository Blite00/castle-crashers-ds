#include "audio.h"
#include <string.h>
#include "audio_timeline.h"
#ifdef ARM9
#include <nds.h>
#include <filesystem.h>
#include <stdio.h>
#include "render.h"
#include "cinematic_audio.h"
#include "audio_assets.h"
#include "pickup_audio.h"
#include "menu_sfx.h"
#include "break_sfx.h"
#include "campaign_breaks.h"
#include "campaign_thieves_scenes.h"
#include "barracks_audio.h"
#include "chapter_assets.h"
#include "chapter_audio.h"
#include "resident_music.h"
#include "audio_envelopes.h"
#include "campaign_thieves.h"
#include "campaign_deer.h"
#include "deer_sfx.h"
#include "flood_sfx.h"
#include "boss_sfx.h"
#include "campaign_camp.h"
#include "campaign_boss.h"
#endif

/* Full hardware range; the output controller reserves mix headroom only when
 * effects overlap. Offline envelope compression improves speaker loudness
 * while keeping every decoded ADPCM peak below -1dBFS. */
enum { MASTER_VOLUME=127, MENU_VOLUME=127, ARENA_VOLUME=112, EFFECT_VOLUME=127,
       ARROW_VOLUME=64 }; /* Original arrow cue is a much denser transient. */

static struct {
    int enabled, mode, observed, filesystem_ready, assets_ok;
#ifdef ARM9
    AudioTimeline story;
    uint8_t *bank,*fly_data,*chapter_music;
    unsigned music_bank,hub_track,machine_origin,boss_track;
    unsigned bank_kind,tail_end,tail_active,field_wait,field_tick,hammer_tick;
    unsigned output_tick,output_master;
    struct {unsigned until,started;uint16_t left,right;const AudioEnvelope*envelope;} voice[16];
#endif
    uint16_t attack[2], hit[2], block[2], magic[2], arrow[2];
} audio;

#ifdef ARM9
/* One mono music sample feeds two hardware channels, with independent speaker
 * routing. 9/14 per speaker is +2.18dB over the former half-volume center pan.
 * Channel0 was unused; no duplicate sample buffer or software mixing is needed.
 * Keep it paired for every stop/fade so a bank load can never leave stale DMA. */
static unsigned speaker_music_volume(unsigned volume){return (volume*9u+7u)/14u;}
static unsigned hardware_scale(unsigned v){return v==127?128:v;}
/* Use decaying SFX peak envelopes and a power-based overlap budget with a
 * 1.3x crest margin. Summing every possible full-scale peak made real combat
 * quieter; this practical budget preserves measured mix loudness while
 * reducing clipping. It is not a guarantee for adversarial aligned samples.
 * One-shot expiry stays conservative when display frames are delayed. */
static unsigned root_ceil(unsigned value){
 unsigned result=0,bit=1u<<30,original=value;
 while(bit>value)bit>>=2;
 while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}
 return result*result<original?result+1:result;
}
static void output_headroom(void){
 unsigned left=0,right=0;
 for(unsigned i=0;i<16;i++)if(audio.voice[i].until){
  if(audio.voice[i].until!=~0u&&audio.voice[i].until<=audio.output_tick){audio.voice[i].until=0;continue;}
  unsigned peak=228;
  const AudioEnvelope*e=audio.voice[i].envelope;
  if(e){unsigned age=audio.output_tick-audio.voice[i].started;age=age>2?age-2:0;if(age>=e->count)age=e->count-1;peak=audio_envelope_peaks[e->offset+age];}
  unsigned l=(audio.voice[i].left*peak+255)>>8,r=(audio.voice[i].right*peak+255)>>8;
  left+=l*l;right+=r*r;
 }
 unsigned peak=(root_ceil(left>right?left:right)*13u+9u)/10u,volume=MASTER_VOLUME;
 if(peak){unsigned safe=2026171u/peak;if(safe<volume)volume=safe;}
 if(!audio.enabled)volume=0;
 if(audio.output_master!=volume){audio.output_master=volume;soundSetMasterVolume(volume);}
}
static void stop_channel(int channel){
 soundKill(channel);if(channel==8)soundKill(0);
 audio.voice[channel].until=0;output_headroom();
}
static void set_channel_volume(int channel,unsigned volume){
 if(channel==8){volume=speaker_music_volume(volume);soundSetVolume(0,(u8)volume);audio.voice[8].left=audio.voice[8].right=hardware_scale(volume)*128;}
 soundSetVolume(channel,(u8)volume);output_headroom();
}
static void play_sample(int channel,const void*data,SoundFormat format,unsigned bytes,unsigned rate,unsigned volume,unsigned pan,bool loop,unsigned point){
 if(pan>127)pan=127;
 audio.voice[channel].started=audio.output_tick;audio.voice[channel].envelope=NULL;
 if(channel!=8&&!loop)for(unsigned i=0;i<sizeof(audio_envelopes)/sizeof(audio_envelopes[0]);i++)if(audio_envelopes[i].bytes==bytes&&audio_envelopes[i].rate==rate){audio.voice[channel].envelope=&audio_envelopes[i];break;}
 audio.voice[channel].until=loop?~0u:audio.output_tick+((bytes-4u)*2u+8u)*60u/rate+3u;
 if(channel==8){
  volume=speaker_music_volume(volume);audio.voice[8].left=audio.voice[8].right=hardware_scale(volume)*128;
  output_headroom();
  soundPlaySampleChannel(8,data,format,bytes,(u16)rate,(u8)volume,0,loop,(u16)point);
  soundPlaySampleChannel(0,data,format,bytes,(u16)rate,(u8)volume,127,loop,(u16)point);
 }else{
  unsigned v=hardware_scale(volume),p=hardware_scale(pan);
  audio.voice[channel].left=v*(128-p);audio.voice[channel].right=v*p;output_headroom();
  soundPlaySampleChannel(channel,data,format,bytes,(u16)rate,(u8)volume,(u8)pan,loop,(u16)point);
 }
}
#endif

static void stop_all(void) {
#ifdef ARM9
    for (int i = 6; i <= 15; i++) stop_channel(i);
    audio.tail_active=audio.field_wait=0;
#endif
}

#ifdef ARM9
#ifndef AUDIO_BANK_PREFIX
#define AUDIO_BANK_PREFIX "nitro:/audio/"
#endif
typedef struct {unsigned bank,track,bytes;const char *path;} ChapterMusicFile;
#define ANY 255u
/* First match wins: each chapter music file, with the bank and track that select it. */
static const ChapterMusicFile *chapter_music_file(unsigned bank,unsigned track){
 static const ChapterMusicFile files[]={
  {CHAPTER_DEER,CHAPTER_AUDIO_DEER_VICTORY,CHAPTER_AUDIO_DEER_VICTORY_BYTES,AUDIO_BANK_PREFIX "chapter-deer-victory.bin"},
  {ANY,CHAPTER_AUDIO_BOSS_VICTORY,CHAPTER_AUDIO_BOSS_VICTORY_BYTES,AUDIO_BANK_PREFIX "chapter-boss-victory.bin"},
  {CHAPTER_DEER,ANY,CHAPTER_AUDIO_DEER_BYTES,AUDIO_BANK_PREFIX "chapter-deer.bin"},
  {CHAPTER_CAMP,CHAPTER_AUDIO_CAMP_BOSS,CHAPTER_AUDIO_CAMP_BOSS_BYTES,AUDIO_BANK_PREFIX "chapter-camp-boss.bin"},
  {CHAPTER_CAMP,CHAPTER_AUDIO_CAMP,CHAPTER_AUDIO_CAMP_BYTES,AUDIO_BANK_PREFIX "chapter-camp.bin"},
  {CHAPTER_FLOOD,CHAPTER_AUDIO_FLOOD_BOSS,CHAPTER_AUDIO_FLOOD_BOSS_BYTES,AUDIO_BANK_PREFIX "chapter-flood-boss.bin"},
  {CHAPTER_FLOOD,CHAPTER_AUDIO_FLOOD,CHAPTER_AUDIO_FLOOD_BYTES,AUDIO_BANK_PREFIX "chapter-flood.bin"},
  {CHAPTER_THIEVES,CHAPTER_AUDIO_THIEVES_BOSS,CHAPTER_AUDIO_THIEVES_BOSS_BYTES,AUDIO_BANK_PREFIX "chapter-thieves-boss.bin"},
  {CHAPTER_THIEVES,ANY,CHAPTER_AUDIO_THIEVES_BYTES,AUDIO_BANK_PREFIX "chapter-thieves.bin"},
  {CHAPTER_FOREST,ANY,CHAPTER_AUDIO_FOREST_BYTES,AUDIO_BANK_PREFIX "chapter-forest.bin"},
  {ANY,CHAPTER_AUDIO_BOSS_DUEL,CHAPTER_AUDIO_BOSS_DUEL_BYTES,AUDIO_BANK_PREFIX "chapter-boss-duel.bin"},
  {ANY,CHAPTER_AUDIO_BOSS_VICTORY,CHAPTER_AUDIO_BOSS_VICTORY_BYTES,AUDIO_BANK_PREFIX "chapter-boss-victory.bin"},
  {CHAPTER_BOSS,ANY,CHAPTER_AUDIO_BOSS_BYTES,AUDIO_BANK_PREFIX "chapter-boss.bin"},
  {ANY,CHAPTER_AUDIO_WORLDMAP,CHAPTER_AUDIO_WORLDMAP_BYTES,AUDIO_BANK_PREFIX "chapter-worldmap.bin"},
  {CHAPTER_WAR,ANY,CHAPTER_AUDIO_WAR_BYTES,AUDIO_BANK_PREFIX "chapter-war.bin"},
  {ANY,CHAPTER_AUDIO_WEAPONS,CHAPTER_AUDIO_WEAPONS_BYTES,AUDIO_BANK_PREFIX "chapter-weapons.bin"},
  {ANY,ANY,CHAPTER_AUDIO_BLACKSMITH_BYTES,AUDIO_BANK_PREFIX "chapter-blacksmith.bin"}};
 for(unsigned i=0;i<sizeof files/sizeof files[0];i++)if((files[i].bank==ANY||files[i].bank==bank)&&(files[i].track==ANY||files[i].track==track))return &files[i];
 return &files[sizeof files/sizeof files[0]-1];
}
static int load_chapter_music(unsigned bank,unsigned track){
 stop_channel(8); /* Stop DMA before a different cue overwrites the borrowed tail. */
 audio.chapter_music=NULL;audio.music_bank=bank;
 if(!audio.filesystem_ready||!chapter_assets_ready(bank))return 0;
 const ChapterMusicFile *file=chapter_music_file(bank,track);unsigned bytes=file->bytes;
 uint8_t*out=chapter_assets_audio_workspace(bytes);if(!out){audio.assets_ok=0;return 0;}
 const char*path=file->path;
 FILE*f=fopen(path,"rb");if(!f){audio.assets_ok=0;return 0;}
 unsigned got=(unsigned)fread(out,1,bytes,f);int extra=fgetc(f);fclose(f);
 if(got!=bytes||extra!=EOF){audio.assets_ok=0;return 0;}
 DC_FlushRange(out,bytes);audio.chapter_music=out;audio.assets_ok=1;return 1;
}
static const uint8_t *load_resident_music(const char*path,unsigned bytes){
 stop_channel(8);audio.chapter_music=NULL;
 if(!audio.filesystem_ready)return NULL;
 uint8_t*out=chapter_assets_music_workspace(bytes);if(!out){return NULL;}
 FILE*f=fopen(path,"rb");if(!f){return NULL;}
 unsigned got=(unsigned)fread(out,1,bytes,f);int extra=fgetc(f);fclose(f);
 if(got!=bytes||extra!=EOF){return NULL;}
 return out;
}
static void chapter_music_play(unsigned track){
 stop_channel(8);
 if(!audio.chapter_music||!chapter_assets_ready(audio.music_bank))return;
 const ChapterAudioClip*c=&chapter_audio_clips[track];
 play_sample(8,audio.chapter_music+c->offset,SoundFormat_ADPCM,c->length,c->rate,ARENA_VOLUME,64,c->loop,c->loop?1:0);
}
#endif

static void start_music(void) {
#ifdef ARM9
    const unsigned char *data;
    unsigned size,rate;
    if (audio.mode == AUDIO_SILENT || audio.mode == AUDIO_BARRACKS_QUIET) return;
    if(audio.mode==AUDIO_WORLDMAP){
        if(load_chapter_music(CHAPTER_HUBS,CHAPTER_AUDIO_WORLDMAP))chapter_music_play(CHAPTER_AUDIO_WORLDMAP);
        return;
    }
    if(audio.mode==AUDIO_DEER)return; /* The Mill Run's music starts with frame 285 of its entrance clip (audio_update_campaign). */
    if(audio.mode==AUDIO_CAMP){
        if(load_chapter_music(CHAPTER_CAMP,CHAPTER_AUDIO_CAMP))chapter_music_play(CHAPTER_AUDIO_CAMP);
        return;
    }
    if(audio.mode==AUDIO_FLOOD){
        if(load_chapter_music(CHAPTER_FLOOD,CHAPTER_AUDIO_FLOOD))chapter_music_play(CHAPTER_AUDIO_FLOOD);
        return;
    }
    if(audio.mode==AUDIO_THIEVES){
        if(load_chapter_music(CHAPTER_THIEVES,CHAPTER_AUDIO_THIEVES))chapter_music_play(CHAPTER_AUDIO_THIEVES);
        return;
    }
    if(audio.mode==AUDIO_FOREST){
        if(load_chapter_music(CHAPTER_FOREST,CHAPTER_AUDIO_FOREST))chapter_music_play(CHAPTER_AUDIO_FOREST);
        return;
    }
    if(audio.mode>=AUDIO_BLACKSMITH&&audio.mode<=AUDIO_ARK){
        unsigned track=audio.mode==AUDIO_WEAPONS?CHAPTER_AUDIO_WEAPONS:audio.mode==AUDIO_ARK&&audio.hub_track==CHAPTER_AUDIO_WEAPONS?CHAPTER_AUDIO_WEAPONS:CHAPTER_AUDIO_BLACKSMITH;
        audio.hub_track=track;if(load_chapter_music(CHAPTER_HUBS,track))chapter_music_play(track);return;
    }
    if(audio.mode==AUDIO_BARBARIAN_BOSS&&!load_chapter_music(CHAPTER_BOSS,CHAPTER_AUDIO_BARBARIAN_BOSS))return;
    if(audio.mode==AUDIO_WAR&&!load_chapter_music(CHAPTER_WAR,CHAPTER_AUDIO_WAR_INTRO))return;
    if(audio.mode==AUDIO_BARRACKS){data=castle_audio_barracks;size=sizeof castle_audio_barracks;rate=BARRACKS_AUDIO_RATE;}
    else if((audio.mode==AUDIO_CAMPAIGN||audio.mode==AUDIO_WAR||audio.mode==AUDIO_BARBARIAN_BOSS)){data=castle_audio_campaign;size=sizeof castle_audio_campaign;rate=CASTLE_AUDIO_CAMPAIGN_RATE;}
    else if (audio.mode == AUDIO_ARENA) {
        data = NULL; size = RESIDENT_MUSIC_ARENA_BYTES;rate=CASTLE_AUDIO_ARENA_RATE;
    } else {
        data = NULL; size = RESIDENT_MUSIC_MENU_BYTES;rate=CASTLE_AUDIO_MENU_RATE;
    }
    if(audio.mode==AUDIO_ARENA||audio.mode==AUDIO_MENU){
        data=load_resident_music(audio.mode==AUDIO_ARENA?AUDIO_BANK_PREFIX "arena.bin":AUDIO_BANK_PREFIX "menu.bin",size);
        if(!data)return;
    }
    DC_FlushRange(data, size);
    /* Word 0 is the ADPCM header; loops must start at word 1. */
    play_sample(8, data, SoundFormat_ADPCM, size,
                           rate, audio.mode==AUDIO_MENU?MENU_VOLUME:ARENA_VOLUME, 64, audio.mode!=AUDIO_BARRACKS, audio.mode==AUDIO_BARRACKS?0:1);
#endif
}

int audio_prepare_assets(void){
#ifdef ARM9
    if(audio.filesystem_ready)return audio.assets_ok; /* Already mounted: leave the music that is playing alone. */
    audio.filesystem_ready=nitroFSInit(NULL)?1:0;
    audio.assets_ok=audio.filesystem_ready;
    /* Menu and arena music come from NitroFS, which mounts after the first menu frame. */
    if(audio.filesystem_ready&&(audio.mode==AUDIO_MENU||audio.mode==AUDIO_ARENA))start_music();
#endif
    return audio.assets_ok;
}
int audio_assets_available(void){return audio.assets_ok;}
#ifdef ARM9
#ifndef AUDIO_BANK_PREFIX
#define AUDIO_BANK_PREFIX "nitro:/audio/"
#endif
static void load_story_bank(unsigned story){
    unsigned barracks=story==AUDIO_STORY_BARRACKS;
    unsigned size=barracks?CINEMATIC_AUDIO_BARRACKS_BYTES:CINEMATIC_AUDIO_MAIN_BYTES;
    audio.bank=NULL;audio.fly_data=NULL;audio.bank_kind=barracks;
    if(!audio.filesystem_ready)return;
    uint8_t *out=barracks?arena_barracks_audio_workspace(size):arena_cinematic_audio_workspace(size);
    if(!out){audio.assets_ok=0;return;}
    FILE *file=fopen(barracks?AUDIO_BANK_PREFIX "barracks.bin":AUDIO_BANK_PREFIX "cinematic.bin","rb");
    if(!file){audio.assets_ok=0;return;}
    unsigned got=(unsigned)fread(out,1,size,file);int extra=fgetc(file);fclose(file);
    if(got!=size||extra!=EOF){audio.assets_ok=0;return;}
    DC_FlushRange(out,size);audio.bank=out;audio.assets_ok=1;
    if(story==AUDIO_STORY_CRYSTAL){
        /* The fly cue outlives the camera. Enemy caches are unused during the
           intro; this separate copy survives world-terrain reuse on exit. */
        const CinematicAudioClip *fly=&cinematic_audio_clips[CIN_AUDIO_FLY];
        audio.fly_data=arena_barracks_audio_workspace(fly->length);
        if(audio.fly_data){memcpy(audio.fly_data,out+fly->offset,fly->length);DC_FlushRange(audio.fly_data,fly->length);}
    }
}
static void story_sample(unsigned id,unsigned channel,unsigned loop,unsigned volume){
    const CinematicAudioClip *clip=&cinematic_audio_clips[id];
    const uint8_t *data=id==CIN_AUDIO_FLY&&audio.fly_data?audio.fly_data:audio.bank?audio.bank+clip->offset:NULL;
    if(!data||(id>=CIN_AUDIO_DOOR)!=!!audio.bank_kind)return;
    play_sample((int)channel,data,SoundFormat_ADPCM,clip->length,clip->rate,(u8)volume,64,!!loop,loop?1:0);
}
static void update_story(const Campaign *c,const Arena *a){
    unsigned story=c->phase==KEEP_BARRACKS_INTRO?AUDIO_STORY_BARRACKS:c->phase==KEEP_INTRO?AUDIO_STORY_CRYSTAL:c->phase==KEEP_ABDUCTION?AUDIO_STORY_PRINCESSES:AUDIO_STORY_NONE;
    unsigned frame=story==AUDIO_STORY_CRYSTAL?c->timer+2:story==AUDIO_STORY_PRINCESSES?c->timer+4:story==AUDIO_STORY_BARRACKS?c->timer:0;
    if(audio.mode==AUDIO_WAR){
        if(c->phase==WAR_MACHINE_INTRO){audio.machine_origin=a->tick-c->timer+1;story=AUDIO_STORY_MACHINE;frame=c->timer;}
        else if(c->phase==WAR_MACHINE_FIGHT||c->phase==WAR_MACHINE_DEATH){story=AUDIO_STORY_MACHINE;frame=audio.machine_origin?a->tick-(audio.machine_origin-1):210;if(frame>210)frame=210;}
    }
    if(audio.chapter_music&&!chapter_assets_ready(audio.music_bank)){stop_channel(8);audio.chapter_music=NULL;}
    unsigned old_story=audio.story.story,old_frame=audio.story.source_frame;
    int entered=!audio.story.seen||story!=old_story||frame<old_frame;
    uint32_t events=audio_timeline_step(&audio.story,story,frame);
    int natural_exit=old_story==AUDIO_STORY_CRYSTAL&&story==AUDIO_STORY_NONE&&old_frame>=365;
    if(events&AUDIO_CUE_EFFECTS_STOP){
        stop_channel(13);stop_channel(15);
        if(!natural_exit||!audio.fly_data){stop_channel(14);audio.tail_active=0;}
    }
    if(events&AUDIO_CUE_FORCE_STOP)stop_channel(13);
    if(events&(AUDIO_CUE_INTRO_STOP|AUDIO_CUE_PARTY_STOP))stop_channel(8);
    if(entered&&story!=AUDIO_STORY_NONE&&story!=AUDIO_STORY_MACHINE){
        audio.tail_active=audio.field_wait=0;load_story_bank(story);
    }
    if(events&AUDIO_CUE_MACHINE_INTRO)chapter_music_play(CHAPTER_AUDIO_WAR_INTRO);
    if(events&AUDIO_CUE_MACHINE_LOOP)chapter_music_play(CHAPTER_AUDIO_WAR_BOSS);
    if(events&AUDIO_CUE_INTRO_START)story_sample(CIN_AUDIO_INTRO,8,0,ARENA_VOLUME);
    /* Muting changes hardware master volume, so music/loops retain position.
       One-shots are discarded while muted, never queued for later playback. */
    if(events&AUDIO_CUE_FORCE_START)story_sample(CIN_AUDIO_FORCE,13,1,EFFECT_VOLUME);
    if(audio.enabled){
        if(events&AUDIO_CUE_FLY){
            story_sample(CIN_AUDIO_FLY,14,0,EFFECT_VOLUME);
            const CinematicAudioClip *fly=&cinematic_audio_clips[CIN_AUDIO_FLY];
            audio.tail_end=a->tick+((fly->length-4)*60+fly->rate-1)/fly->rate;audio.tail_active=audio.fly_data!=NULL;
        }
        if(events&AUDIO_CUE_SCREAM)story_sample(CIN_AUDIO_SCREAM,14,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_HELP)story_sample(CIN_AUDIO_HELP,14,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_DOOR)story_sample(CIN_AUDIO_DOOR,15,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_BLOCK3)story_sample(CIN_AUDIO_BLOCK3,15,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_GROUND2)story_sample(CIN_AUDIO_GROUND2,15,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_GROUND5)story_sample(CIN_AUDIO_GROUND5,15,0,EFFECT_VOLUME);
        if(events&AUDIO_CUE_BLOCK1)play_sample(15,castle_audio_block,SoundFormat_ADPCM,sizeof castle_audio_block,CASTLE_AUDIO_RATE,EFFECT_VOLUME,64,false,0);
        if(events&AUDIO_CUE_PUNCH)play_sample(15,castle_audio_hit,SoundFormat_ADPCM,sizeof castle_audio_hit,CASTLE_AUDIO_RATE,EFFECT_VOLUME,64,false,0);
    }
    if(story==AUDIO_STORY_CRYSTAL&&frame>=347&&frame<359)
        set_channel_volume(8,(u8)(ARENA_VOLUME*(377-frame)/30));
    if(old_story==AUDIO_STORY_MACHINE&&story==AUDIO_STORY_NONE&&audio.mode==AUDIO_WAR){
        /* EWARMACH death frame188 restores the field loop, without rereading
         * the machine cues or restarting on subsequent travel updates. */
        stop_channel(8);
        DC_FlushRange(castle_audio_campaign,sizeof castle_audio_campaign);
        play_sample(8,castle_audio_campaign,SoundFormat_ADPCM,sizeof castle_audio_campaign,
                              CASTLE_AUDIO_CAMPAIGN_RATE,ARENA_VOLUME,64,true,1);
    }
    if(events&AUDIO_CUE_FIELD_START){
        if(story!=AUDIO_STORY_CRYSTAL&&audio.mode==AUDIO_CAMPAIGN){
            if(natural_exit){audio.field_wait=1;audio.field_tick=a->tick;}
            else start_music();
        }
    }
    if(audio.field_wait&&a->tick-audio.field_tick>=8){audio.field_wait=0;start_music();}
    if(audio.tail_active&&story!=AUDIO_STORY_CRYSTAL){
        int unsafe=a->map_id!=3||a->tick>=audio.tail_end;
        for(unsigned i=0;i<CAMPAIGN_ENEMIES;i++)if(c->enemies[i].ready)unsafe=1;
        if(unsafe){stop_channel(14);audio.tail_active=0;audio.fly_data=NULL;}
    }
    if(entered&&story==AUDIO_STORY_NONE)audio.bank=NULL;
}
#endif

void audio_init(void) {
    memset(&audio, 0, sizeof audio);
    audio.enabled = 1;
#ifdef ARM9
    soundEnable();
    audio.output_master=MASTER_VOLUME;soundSetMasterVolume(MASTER_VOLUME);
    DC_FlushRange(castle_audio_swing, sizeof castle_audio_swing);
    DC_FlushRange(castle_audio_hit, sizeof castle_audio_hit);
    DC_FlushRange(castle_audio_block, sizeof castle_audio_block);
    DC_FlushRange(castle_audio_magic_blue, sizeof castle_audio_magic_blue);
    DC_FlushRange(castle_audio_magic_orange, sizeof castle_audio_magic_orange);
    DC_FlushRange(castle_audio_magic_red, sizeof castle_audio_magic_red);
    DC_FlushRange(castle_audio_magic_green, sizeof castle_audio_magic_green);
    DC_FlushRange(castle_audio_arrow, sizeof castle_audio_arrow);
#endif
}

void audio_set_mode(enum AudioMode mode) {
    if (audio.mode == (int)mode) return;
#ifdef ARM9
    if(mode==AUDIO_ARK&&(audio.mode==AUDIO_BLACKSMITH||audio.mode==AUDIO_WEAPONS)&&audio.chapter_music&&chapter_assets_ready(CHAPTER_HUBS)){audio.mode=mode;return;}
#endif
    stop_all();
    audio.mode = mode;
#ifdef ARM9
    audio.chapter_music=NULL;audio.machine_origin=0;audio.boss_track=0;audio.hammer_tick=~0u;
    if(mode<AUDIO_CAMPAIGN||mode>=AUDIO_WAR){audio.story=(AudioTimeline){0};audio.bank=audio.fly_data=NULL;}
#endif
    audio.observed = 0;
    start_music();
}

void audio_toggle(void) {
    audio.enabled = !audio.enabled;
#ifdef ARM9
    output_headroom();
#endif
}

int audio_enabled(void) { return audio.enabled; }

void audio_update(const Arena *arena) {
#ifdef ARM9
    audio.output_tick++;output_headroom();
#endif
    if (!arena) return;
    for (int i = 0; i < 2; i++) {
        const Fighter *p = &arena->p[i];
        if (audio.observed && audio.enabled && audio.mode>=AUDIO_ARENA) {
#ifdef ARM9
            int pan = arena->map_id>=3?64+(p->x-arena->p[0].x)*127/424:p->x * 127 / (WORLD_WIDTH-1);
            if (pan < 0) pan = 0;
            if (pan > 127) pan = 127;
            if (p->attack_seq != audio.attack[i])
                play_sample(9+i, castle_audio_swing, SoundFormat_ADPCM,
                    sizeof castle_audio_swing, CASTLE_AUDIO_RATE, EFFECT_VOLUME, (u8)pan, false, 0);
            if (p->arrow_seq != audio.arrow[i])
                play_sample(9+i, castle_audio_arrow, SoundFormat_ADPCM,
                    sizeof castle_audio_arrow, CASTLE_AUDIO_RATE, ARROW_VOLUME, (u8)pan, false, 0);
            if (p->hit_seq != audio.hit[i])
                play_sample(11+i, castle_audio_hit, SoundFormat_ADPCM,
                    sizeof castle_audio_hit, CASTLE_AUDIO_RATE, EFFECT_VOLUME, (u8)pan, false, 0);
            if (p->block_seq != audio.block[i])
                play_sample(11+i, castle_audio_block, SoundFormat_ADPCM,
                    sizeof castle_audio_block, CASTLE_AUDIO_RATE, EFFECT_VOLUME, (u8)pan, false, 0);
            if (p->magic_seq != audio.magic[i]) {
                const unsigned char *data = castle_audio_magic_blue;
                unsigned size = sizeof castle_audio_magic_blue;
                if(p->knight_id==1){data=castle_audio_magic_orange;size=sizeof castle_audio_magic_orange;}
                else if(p->knight_id==2){data=castle_audio_magic_red;size=sizeof castle_audio_magic_red;}
                else if(p->knight_id==3){data=castle_audio_magic_green;size=sizeof castle_audio_magic_green;}
                play_sample(13+i, data, SoundFormat_ADPCM,
                    size, CASTLE_AUDIO_RATE, EFFECT_VOLUME, (u8)pan, false, 0);
            }
#endif
        }
        audio.attack[i] = p->attack_seq;
        audio.hit[i] = p->hit_seq;
        audio.block[i] = p->block_seq;
        audio.magic[i] = p->magic_seq;
        audio.arrow[i] = p->arrow_seq;
    }
    audio.observed = 1;
}

void audio_update_enemy(const Fighter *enemy){
    static uint16_t last_hit;static int seen;
    if(!enemy||!enemy->ready){seen=0;return;}
#ifdef ARM9
    if(seen&&enemy->hit_seq!=last_hit&&audio.enabled&&audio.mode>=AUDIO_ARENA)
        play_sample(15,castle_audio_hit,SoundFormat_ADPCM,sizeof castle_audio_hit,CASTLE_AUDIO_RATE,EFFECT_VOLUME,64,false,0);
#endif
    last_hit=enemy->hit_seq;seen=1;
    (void)last_hit;(void)seen;
}

void audio_menu_sound(unsigned kind){
#ifdef ARM9
    if(!audio.enabled)return;
    const unsigned char *data=kind==1?menu_sfx_select:menu_sfx_move;unsigned size=kind==1?sizeof menu_sfx_select:sizeof menu_sfx_move;
    DC_FlushRange(data,size);play_sample(14,data,SoundFormat_ADPCM,size,MENU_SFX_RATE,EFFECT_VOLUME,64,false,0);
#else
    (void)kind;
#endif
}
void audio_update_campaign(const Campaign *c,const Arena *a){
    static unsigned seen,last_tick;static uint8_t seq[2];
    if(!c||!a||!a->running||audio.mode<AUDIO_CAMPAIGN){seen=0;return;}
#ifdef ARM9
    update_story(c,a);
    /* The wooden gate has no fighter hit sequence; its synchronized health drops on every melee swing (7). */
    {
        static uint16_t gate_hp;static int gate_seen;
        if(a->map_id==WAR_MAP&&c->war.gate_hp){
            if(gate_seen&&c->war.gate_hp+2<=gate_hp&&audio.enabled&&audio.mode>=AUDIO_ARENA)
                play_sample(15,castle_audio_hit,SoundFormat_ADPCM,sizeof castle_audio_hit,CASTLE_AUDIO_RATE,EFFECT_VOLUME,64,false,0);
            gate_hp=c->war.gate_hp;gate_seen=1;
        }else gate_seen=0;
    }
    if(audio.mode==AUDIO_BLACKSMITH&&audio.enabled&&a->tick%27==12&&a->tick!=audio.hammer_tick&&audio.chapter_music&&chapter_assets_ready(CHAPTER_HUBS)){
        audio.hammer_tick=a->tick;
        const ChapterAudioClip*clip=&chapter_audio_clips[CHAPTER_AUDIO_SMITH_HAMMER];
        play_sample(15,audio.chapter_music+clip->offset,SoundFormat_ADPCM,clip->length,clip->rate,EFFECT_VOLUME,64,false,0);
    }
    /* LEVEL7 source: intro sprite202 frame127 starts NG64061, treasure
     * sprite226 frame10 starts goldgrab. Snapshot jumps cross each cue once. */
    if(audio.mode==AUDIO_DEER&&c->timer>=285&&!audio.boss_track){
        audio.boss_track=1;
        if(load_chapter_music(CHAPTER_DEER,CHAPTER_AUDIO_DEER))chapter_music_play(CHAPTER_AUDIO_DEER);
    }
    if(c->phase==WAR_COMPLETE&&a->map_id>=FOREST_MAP&&a->map_id<=CAMP_MAP&&(audio.mode==AUDIO_CAMP||audio.mode==AUDIO_FLOOD||audio.mode==AUDIO_FOREST||audio.mode==AUDIO_THIEVES||audio.mode==AUDIO_DEER)){
        /* MAIN f_ScoreTally: StopMusic, then PlayMusic(4) Waterflame_RaceVictory while the score signs count up. */
        if(audio.boss_track!=2){
            audio.boss_track=2;
            unsigned bank=a->map_id==CAMP_MAP?CHAPTER_CAMP:a->map_id==FLOOD_MAP?CHAPTER_FLOOD:a->map_id==DEER_MAP?CHAPTER_DEER:a->map_id==THIEVES_MAP?CHAPTER_THIEVES:CHAPTER_FOREST,track=a->map_id==DEER_MAP?CHAPTER_AUDIO_DEER_VICTORY:CHAPTER_AUDIO_BOSS_VICTORY;
            if(load_chapter_music(bank,track))chapter_music_play(track);
        }
    }else if(audio.mode==AUDIO_CAMP){
        unsigned wanted=campaign_camp_boss_music(c);
        if(wanted!=audio.boss_track){
            audio.boss_track=wanted;
            unsigned track=wanted?CHAPTER_AUDIO_CAMP_BOSS:CHAPTER_AUDIO_CAMP;
            if(load_chapter_music(CHAPTER_CAMP,track))chapter_music_play(track);
        }
    }else if(audio.mode==AUDIO_FLOOD){
        unsigned wanted=c->group==1;
        if(wanted!=audio.boss_track){
            audio.boss_track=wanted;
            unsigned track=wanted?CHAPTER_AUDIO_FLOOD_BOSS:CHAPTER_AUDIO_FLOOD;
            if(load_chapter_music(CHAPTER_FLOOD,track))chapter_music_play(track);
        }
    }else if(audio.mode==AUDIO_THIEVES){
        unsigned wanted=c->group>=THIEVES_EVENT_COUNT-1&&c->group<THIEVES_EVENT_COUNT&&c->phase==WAR_FIGHT;
        if(wanted!=audio.boss_track){
            audio.boss_track=wanted;
            unsigned track=wanted?CHAPTER_AUDIO_THIEVES_BOSS:CHAPTER_AUDIO_THIEVES;
            if(load_chapter_music(CHAPTER_THIEVES,track))chapter_music_play(track);
        }
    }
    if(audio.mode==AUDIO_BARBARIAN_BOSS){
        unsigned wanted=(c->phase==BOSS_COMPLETE||(c->phase==BOSS_KISS&&c->timer>=65))?4:c->phase==BOSS_DUEL?3:c->phase==BOSS_KISS?(a->p[1].ready?3u:2u):(c->phase>BOSS_TREASURE||(c->phase==BOSS_TREASURE&&c->timer>=10))?2:
                        (c->phase>BOSS_INTRO||(c->phase==BOSS_INTRO&&c->timer>=160))?1:0;
        if(wanted!=audio.boss_track){
            audio.boss_track=wanted;
            unsigned track=wanted==4?CHAPTER_AUDIO_BOSS_VICTORY:wanted==3?CHAPTER_AUDIO_BOSS_DUEL:wanted==2?CHAPTER_AUDIO_GOLDGRAB:CHAPTER_AUDIO_BARBARIAN_BOSS;
            if(wanted<3||load_chapter_music(CHAPTER_BOSS,track))chapter_music_play(track);
        }
    }
#endif
    if(a->tick<last_tick)seen=0;
    for(int i=0;i<2;i++){
#ifdef ARM9
        if(seen&&seq[i]!=c->pickup_seq[i]&&c->pickup_timer[i]&&audio.enabled&&(c->pickup_kind[i]!=DROP_WEAPON||c->phase==KEEP_STORE)){
            const unsigned char *data=pickup_audio_eat;unsigned size=sizeof pickup_audio_eat;
            if((c->pickup_kind[i]<=DROP_BAG||c->pickup_kind[i]==DROP_GEM||c->pickup_kind[i]==DROP_POTION||(c->pickup_kind[i]==DROP_WEAPON&&c->phase==KEEP_STORE))){
                if(c->pickup_seq[i]&1){data=pickup_audio_coin1;size=sizeof pickup_audio_coin1;}
                else{data=pickup_audio_coin2;size=sizeof pickup_audio_coin2;}
            }
            DC_FlushRange(data,size);
            play_sample(6+i,data,SoundFormat_ADPCM,size,PICKUP_AUDIO_RATE,EFFECT_VOLUME,64,false,0);
        }
#endif
        seq[i]=c->pickup_seq[i];
    }
#ifdef ARM9
    {   /* Scenery: a cut tuft or an opened chest is heard when its published bit turns on, on host and guest alike. */
        static unsigned last_bits;static int last_map=-1;
        unsigned bits=(c->broken&2047u)|((unsigned)c->npc_ticks<<11);
        if(a->map_id==13||a->map_id==12){
            if(last_map==(int)a->map_id&&audio.enabled)for(unsigned i=0;i<19;i++)if((bits&~last_bits)&(1u<<i)){
                unsigned n;const BreakPoint *t=campaign_break_table(a->map_id,&n);
                int chest=i<n&&t[i].kind;const unsigned char *d=chest?break_sfx_chest:break_sfx_grass;unsigned size=chest?sizeof break_sfx_chest:sizeof break_sfx_grass;
                DC_FlushRange(d,size);play_sample(12,d,SoundFormat_ADPCM,size,BREAK_SFX_RATE,EFFECT_VOLUME,64,false,0);break;
            }
            last_bits=bits;last_map=(int)a->map_id;
        }else last_map=-1;
    }
#endif
#ifdef ARM9
    {   /* Mill Run: the entrance clip and finale sounds, fired when the published timer crosses a cue, plus the monster's punch. */
        static unsigned last_timer;static uint8_t toss[2];static int last_deer=-1;
        if(a->map_id==DEER_MAP&&audio.enabled){
            if(last_deer!=DEER_MAP||c->timer<last_timer)last_timer=c->timer?c->timer-1:0;
            if(c->timer>last_timer&&c->timer-last_timer<60){
                unsigned cue,after=last_timer;
                while((cue=campaign_deer_cue(after,c->timer))!=0){
                    unsigned id=cue&255;after=cue>>8;
                    const unsigned char *data=deer_sfx_data+deer_sfx_table[id-1][0];unsigned size=deer_sfx_table[id-1][1];
                    DC_FlushRange(data,size);play_sample(13,data,SoundFormat_ADPCM,size,DEER_SFX_RATE,EFFECT_VOLUME,64,false,0);
                }
            }
            for(unsigned h=0;h<2;h++){
                int now=a->p[h].ready&&a->p[h].pose==DEER_POSE_TOSS&&a->p[h].animation==0;
                if(now&&!toss[h]){const unsigned char *data=deer_sfx_data+deer_sfx_table[DEER_SFX_MEATY4][0];unsigned size=deer_sfx_table[DEER_SFX_MEATY4][1];
                    DC_FlushRange(data,size);play_sample(12,data,SoundFormat_ADPCM,size,DEER_SFX_RATE,EFFECT_VOLUME,64,false,0);}
                toss[h]=(uint8_t)now;
            }
            last_timer=c->timer;
        }
        last_deer=a->map_id;
    }
#endif
#ifdef ARM9
    {   /* Barbarian Boss gate scene: the cues of sprite 202 (ground thumps, the roar, the falling door) when the published timer crosses them. */
        static unsigned last_timer;static int last_boss=-1;
        if(a->map_id==BOSS_MAP&&c->phase==BOSS_INTRO&&audio.enabled){
            if(last_boss!=BOSS_MAP||c->timer<last_timer)last_timer=c->timer?c->timer-1:0;
            if(c->timer>last_timer&&c->timer-last_timer<60){
                unsigned cue,after=last_timer;
                while((cue=campaign_boss_intro_cue(after,c->timer))!=0){
                    unsigned id=cue&255;after=cue>>8;
                    const unsigned char *data=boss_sfx_data+boss_sfx_table[id-1][0];unsigned size=boss_sfx_table[id-1][1];
                    DC_FlushRange(data,size);play_sample(13,data,SoundFormat_ADPCM,size,BOSS_SFX_RATE,EFFECT_VOLUME,64,false,0);
                }
            }
            last_timer=c->timer;
        }
        last_boss=a->map_id;
    }
#endif
#ifdef ARM9
    {   /* Flooded Castle: the angry fish clip (sprite 333 frames 10, 35 and 65), the cannon that stuns the Catfish, its hairball and the bat's bite.
           Everything is read from the published snapshot, so host and guest hear the same things. */
        static int last_flood=-1;static unsigned last_clock;static uint8_t last_stun,last_spit;static int16_t last_hp[2];
        if(a->map_id==FLOOD_MAP&&audio.enabled){
            if(last_flood!=FLOOD_MAP||c->break_tick<last_clock)last_clock=c->break_tick?c->break_tick-1u:0;
            static const struct {uint16_t at;uint8_t id;} cues[]={{1581,FLOOD_SFX_TOADBLAH},{1606,FLOOD_SFX_EXPLOSION4},{1636,FLOOD_SFX_SLIMEHIT3}};
            int id=-1;
            for(unsigned i=0;i<sizeof cues/sizeof *cues;i++)if(last_clock<cues[i].at&&c->break_tick>=cues[i].at&&c->break_tick-last_clock<60)id=cues[i].id;
            last_clock=c->break_tick;
            int stun=0;for(int i=0;i<CAMPAIGN_ENEMIES;i++)if(c->species[i]==2&&c->enemies[i].ready&&c->enemies[i].hp>0&&c->enemies[i].pose==POSE_HURT)stun=1;
            int spit=c->war.machine.ready&&c->war.machine.attack_kind==0;
            if(stun&&!last_stun)id=FLOOD_SFX_EXPLOSION3;
            if(spit&&!last_spit)id=FLOOD_SFX_CATBURP;
            for(int h=0;h<2;h++){if(a->p[h].ready&&a->p[h].pose==POSE_DIG&&a->p[h].hp<last_hp[h])id=FLOOD_SFX_CHOMP;last_hp[h]=a->p[h].hp;}
            last_stun=(uint8_t)stun;last_spit=(uint8_t)spit;
            if(id>=0){const unsigned char *data=flood_sfx_data+flood_sfx_table[id][0];unsigned size=flood_sfx_table[id][1];
                DC_FlushRange(data,size);play_sample(13,data,SoundFormat_ADPCM,size,FLOOD_SFX_RATE,EFFECT_VOLUME,64,false,0);}
        }
        last_flood=a->map_id;
    }
#endif
#ifdef ARM9
    {   /* Thieves Forest cutscene sounds: fired when the published clip frame crosses a cue, so host and guest hear the same clip. */
        static int last_scene=-1;static unsigned last_frame;
        int scene=a->map_id==13?thieves_scene_playing(c):-1;
        if(scene>=0&&audio.enabled){
            if(scene!=last_scene)last_frame=c->timer?c->timer-1:0;
            if(c->timer>last_frame&&c->timer-last_frame<30)
                for(unsigned i=0;i<12;i++){
                    const ThievesCue *q=&thieves_scenes[scene].cues[i];
                    if(q->frame&&q->frame>last_frame&&q->frame<=c->timer){
                        const unsigned char *data=thieves_sfx_data+thieves_sfx_table[q->sfx][0];unsigned size=thieves_sfx_table[q->sfx][1];
                        DC_FlushRange(data,size);play_sample(13,data,SoundFormat_ADPCM,size,THIEVES_SFX_RATE,EFFECT_VOLUME,64,false,0);
                    }
                }
        }
        last_scene=scene;last_frame=scene>=0?c->timer:0;
    }
#endif
    (void)seq;(void)seen;seen=1;last_tick=a->tick;
}

