#include "audio.h"
#include <nds.h>
#include "cinematic_audio.h"
#include "chapter_audio.h"
#include "audio_envelopes.h"
#include "chapter_assets.h"
#include <assert.h>
#include <stdio.h>
unsigned campaign_deer_cue(unsigned after,unsigned now){(void)after;(void)now;return 0;} /* The Mill Run cue table lives in campaign_deer.c, which this backend test does not link. */
#include <string.h>
static unsigned char scenery[177000],enemy[27048],music[450000];
uint8_t chapter_asset_buffer[CHAPTER_ASSET_CAPACITY]; /* The thieves cutscene sounds live in a chapter bank. */
static unsigned bank=CHAPTER_WAR,music_loads;
int chapter_assets_ready(unsigned wanted){return bank==wanted;}
void *chapter_assets_audio_workspace(unsigned n){if(n>sizeof music)return NULL;music_loads++;return music;}
void *chapter_assets_music_workspace(unsigned n){if(n>sizeof music)return NULL;return music;}
static struct {const void *data;unsigned bytes,plays,volume,pan,rate;int active;} channels[16];
static unsigned loads,master;static int fs_ready=1;
void soundEnable(void){memset(channels,0,sizeof channels);}
void soundKill(int ch){channels[ch].active=0;}
void soundSetMasterVolume(int v){master=(unsigned)v;}
void soundSetVolume(int ch,int v){assert(v>=0&&v<=127);channels[ch].volume=v;}
void DC_FlushRange(const void *p,unsigned n){assert(p&&n);}
int soundPlaySampleChannel(int ch,const void *p,SoundFormat fmt,unsigned n,unsigned rate,int vol,int pan,_Bool loop,unsigned start){
 (void)fmt;(void)rate;(void)vol;(void)pan;assert(start==(loop?1u:0u));
 channels[ch].data=p;channels[ch].bytes=n;channels[ch].active=1;channels[ch].plays++;channels[ch].volume=vol;channels[ch].pan=pan;channels[ch].rate=rate;return ch;
}
_Bool nitroFSInit(const char *p){(void)p;return fs_ready;}
void *arena_cinematic_audio_workspace(unsigned n){assert(n<=sizeof scenery);loads++;return scenery;}
void *arena_barracks_audio_workspace(unsigned n){assert(n<=sizeof enemy);return enemy;}
static void assert_mix_budget(void){
 double left=0,right=0;
 for(unsigned i=0;i<16;i++)if(channels[i].active){unsigned v=channels[i].volume==127?128:channels[i].volume,p=channels[i].pan==127?128:channels[i].pan;unsigned peak=228;
 if(i!=0&&i!=8)for(unsigned e=0;e<sizeof(audio_envelopes)/sizeof(audio_envelopes[0]);e++)if(audio_envelopes[e].bytes==channels[i].bytes&&audio_envelopes[e].rate==channels[i].rate){peak=audio_envelope_peaks[audio_envelopes[e].offset];break;}
 double l=v*(128-p)*peak,r=v*p*peak;left+=l*l;right+=r*r;}
 double m=(master==127?128:master)/128.0;
 assert(left*m*m*1.3*1.3 < 0.97*0.97*16384.0*16384.0*255.0*255.0);
 assert(right*m*m*1.3*1.3 < 0.97*0.97*16384.0*16384.0*255.0*255.0);
}
static void step(Campaign *c,Arena *a,unsigned phase,unsigned timer){c->phase=phase;c->timer=timer;a->tick++;audio_update_campaign(c,a);}
int main(void){
 Campaign c={0};Arena a={0};a.running=1;a.map_id=5;
 audio_init();assert(audio_prepare_assets());audio_set_mode(AUDIO_BARRACKS);
 step(&c,&a,KEEP_BARRACKS_INTRO,0);assert(channels[8].active);
 for(unsigned t=1;t<=161;t++)step(&c,&a,KEEP_BARRACKS_INTRO,t);
 assert(!channels[8].active&&channels[15].active&&channels[15].data==enemy+cinematic_audio_clips[CIN_AUDIO_DOOR].offset);
 unsigned p=channels[15].plays;step(&c,&a,KEEP_BARRACKS_INTRO,161);assert(channels[15].plays==p);
 step(&c,&a,KEEP_BARRACKS_TRAVEL,0);assert(!channels[15].active);
 a.map_id=3;audio_set_mode(AUDIO_CAMPAIGN);step(&c,&a,KEEP_INTRO,0);assert(loads==1&&channels[8].data==scenery);
 for(unsigned t=1;t<=345;t++)step(&c,&a,KEEP_INTRO,t);
 assert(channels[13].active&&channels[14].active&&channels[14].data==enemy);
 for(unsigned t=346;t<=363;t++)step(&c,&a,KEEP_INTRO,t);
 assert(!channels[8].active&&!channels[13].active);
 step(&c,&a,KEEP_TRAVEL,0);assert(channels[14].active);
 c.enemies[0].ready=1;step(&c,&a,KEEP_TRAVEL,0);assert(!channels[14].active);c.enemies[0].ready=0;
 for(unsigned t=0;t<8;t++){step(&c,&a,KEEP_TRAVEL,0);}assert(channels[8].active);
 step(&c,&a,KEEP_ABDUCTION,0);unsigned scream=channels[14].plays;
 audio_toggle();assert(master==0);step(&c,&a,KEEP_ABDUCTION,18);assert(channels[14].plays==scream);
 audio_toggle();assert(master==127);step(&c,&a,KEEP_ABDUCTION,18);assert(channels[14].plays==scream);
 step(&c,&a,KEEP_ABDUCTION,86);assert(channels[14].plays==scream+1);
 step(&c,&a,KEEP_PAN_OUT,0);assert(!channels[14].active);
 step(&c,&a,KEEP_INTRO,0);step(&c,&a,KEEP_INTRO,200);assert(channels[13].active);
 step(&c,&a,KEEP_TRAVEL,0);assert(!channels[13].active&&channels[8].active);
 audio_set_mode(AUDIO_MENU);assert(channels[8].active);audio_update_campaign(&c,&a);assert(channels[8].active);
 audio_set_mode(AUDIO_SILENT);bank=CHAPTER_WAR;a.map_id=6;a.tick=1000;audio_set_mode(AUDIO_WAR);assert(music_loads==1&&channels[8].active);
 step(&c,&a,WAR_MACHINE_INTRO,0);
 for(unsigned t=1;t<=59;t++)step(&c,&a,WAR_MACHINE_INTRO,t);
 assert(channels[8].data==music+chapter_audio_clips[CHAPTER_AUDIO_WAR_INTRO].offset);
 for(unsigned t=60;t<=126;t++)step(&c,&a,WAR_MACHINE_INTRO,t);
 for(unsigned t=127;t<=210;t++)step(&c,&a,WAR_MACHINE_FIGHT,0);
 assert(music_loads==1&&channels[8].data==music+chapter_audio_clips[CHAPTER_AUDIO_WAR_BOSS].offset);
 unsigned plays=channels[8].plays;step(&c,&a,WAR_MACHINE_FIGHT,0);assert(channels[8].plays==plays);
 step(&c,&a,WAR_MACHINE_DEATH,187);step(&c,&a,WAR_TRAVEL,0);
 assert(channels[8].active&&channels[8].data!=music&&music_loads==1);
 plays=channels[8].plays;step(&c,&a,WAR_TRAVEL,0);assert(channels[8].plays==plays);
 step(&c,&a,WAR_MACHINE_FIGHT,0);
 bank=99;step(&c,&a,WAR_MACHINE_FIGHT,0);assert(!channels[8].active);
 audio_set_mode(AUDIO_SILENT);assert(!channels[8].active);bank=CHAPTER_HUBS;audio_set_mode(AUDIO_BLACKSMITH);assert(channels[8].active&&music_loads==2);
 a.tick=12;unsigned hammer=channels[15].plays;audio_update_campaign(&c,&a);
 assert(channels[15].plays==hammer+1&&channels[15].data==music+chapter_audio_clips[CHAPTER_AUDIO_SMITH_HAMMER].offset);
 audio_update_campaign(&c,&a);assert(channels[15].plays==hammer+1);
 plays=channels[8].plays;audio_set_mode(AUDIO_ARK);assert(channels[8].plays==plays&&music_loads==2);audio_set_mode(AUDIO_WEAPONS);assert(music_loads==3&&channels[8].bytes==CHAPTER_AUDIO_WEAPONS_BYTES);
 audio_set_mode(AUDIO_WORLDMAP);assert(music_loads==4&&channels[8].bytes==CHAPTER_AUDIO_WORLDMAP_BYTES);
 plays=channels[8].plays;for(unsigned i=0;i<90;i++)audio_set_mode(AUDIO_WORLDMAP);
 assert(channels[8].plays==plays&&music_loads==4);
 audio_set_mode(AUDIO_SILENT);bank=CHAPTER_BOSS;a.map_id=11;audio_set_mode(AUDIO_BARBARIAN_BOSS);assert(music_loads==5);
 step(&c,&a,BOSS_INTRO,126);assert(channels[8].data!=music);
 step(&c,&a,BOSS_INTRO,129);assert(channels[8].data==music+chapter_audio_clips[CHAPTER_AUDIO_BARBARIAN_BOSS].offset);
 plays=channels[8].plays;step(&c,&a,BOSS_FIGHT,0);assert(channels[8].plays==plays);
 step(&c,&a,BOSS_TREASURE,9);assert(channels[8].plays==plays);
 step(&c,&a,BOSS_TREASURE,12);assert(channels[8].data==music+chapter_audio_clips[CHAPTER_AUDIO_GOLDGRAB].offset&&music_loads==5);
 plays=channels[8].plays;step(&c,&a,BOSS_PRINCESS,20);assert(channels[8].plays==plays);
 a.p[1].ready=1;step(&c,&a,BOSS_DUEL,0);assert(music_loads==6&&channels[8].bytes==CHAPTER_AUDIO_BOSS_DUEL_BYTES);
 plays=channels[8].plays;step(&c,&a,BOSS_DUEL,35);step(&c,&a,BOSS_KISS,20);assert(music_loads==6&&channels[8].plays==plays);
 step(&c,&a,BOSS_KISS,64);assert(music_loads==6&&channels[8].plays==plays);
 step(&c,&a,BOSS_KISS,67);assert(music_loads==7&&channels[8].bytes==CHAPTER_AUDIO_BOSS_VICTORY_BYTES);
 plays=channels[8].plays;step(&c,&a,BOSS_COMPLETE,0);assert(music_loads==7&&channels[8].plays==plays);
 plays=channels[8].plays;step(&c,&a,BOSS_COMPLETE,90);assert(music_loads==7&&channels[8].plays==plays);
 audio_set_mode(AUDIO_SILENT);bank=99;audio_set_mode(AUDIO_WAR);assert(!channels[8].active&&music_loads==7);
 fs_ready=0;audio_init();assert(!audio_prepare_assets());audio_set_mode(AUDIO_CAMPAIGN);
 unsigned before=loads;step(&c,&a,KEEP_INTRO,0);assert(loads==before&&!channels[8].active);
 step(&c,&a,KEEP_INTRO,170);assert(!channels[13].active);
 step(&c,&a,KEEP_TRAVEL,0);assert(channels[8].active);
 /* Full music plus simultaneous two-player attacks/hits/magic: gain falls
  * before the new voices start, and the power budget preserves its crest margin. Muting and expiry restore the correct system gain. */
 fs_ready=1;audio_init();audio_prepare_assets();audio_set_mode(AUDIO_ARENA);memset(&a,0,sizeof a);a.running=1;
 a.p[0].ready=a.p[1].ready=1;audio_update(&a);
 assert(channels[0].active&&channels[8].active&&channels[0].data==channels[8].data);
 assert(channels[8].pan==0&&channels[0].pan==127&&channels[8].volume==72&&channels[0].volume==72);
 for(unsigned i=0;i<2;i++){a.p[i].attack_seq++;a.p[i].hit_seq++;a.p[i].magic_seq++;a.p[i].x=212;}
 audio_update(&a);assert(master<127);assert_mix_budget();
 audio_toggle();assert(master==0);audio_toggle();assert(master<127);assert_mix_budget();
 for(unsigned t=0;t<130;t++){audio_update(&a);}assert(master==127);
 audio_set_mode(AUDIO_SILENT);assert(!channels[0].active&&!channels[8].active);
 puts("audio backend: bank loading, cue timing, duplicate/snapshot, mute, skip and borrowed-buffer lifetimes passed");return 0;
}


