/* Native Windows CPU diagnostic; never a DS FPS measurement. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "campaign.h"
#include "campaign_war.h"
#include "campaign_boss.h"
#include "chapter_assets.h"
#include "render.h"
#include "stage.h"
extern unsigned war_diag_boss_tiles,war_diag_ram_tiles;
static unsigned short pixels[49152],palette[256];static unsigned char scene[512*512];
static double elapsed(LARGE_INTEGER t,LARGE_INTEGER f){LARGE_INTEGER n;QueryPerformanceCounter(&n);return (n.QuadPart-t.QuadPart)*1000.0/f.QuadPart;}
static int cmp(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
int main(void){LARGE_INTEGER freq;QueryPerformanceFrequency(&freq);const char*names[]={"war_ambush","ram_intro","ram_fight","boss_intro","boss_burial","frog_walk"};
for(int kind=0;kind<6;kind++){
 Arena a;Campaign c={0};arena_init(&a);campaign_start(&c,&a,1);a.p[0].ready=a.p[1].ready=1;
 if(kind<3){campaign_war_start(&c,&a,6);memset(c.enemies,0,sizeof c.enemies);memset(&c.ally,0,sizeof c.ally);c.group=5;c.phase=kind==0?WAR_HORSE_CINEMA:WAR_MACHINE_INTRO;a.p[0].x=kind?2970:1706;a.p[1].x=a.p[0].x-30;a.p[0].y=a.p[1].y=280;if(kind==2){uint16_t k[2]={0};for(int t=0;t<126;t++)campaign_step(&c,&a,k);}}
 else if(kind<5){campaign_boss_start(&c,&a);if(kind==4){c.phase=BOSS_FIGHT;c.war.machine.ready=1;c.war.machine.x=500;c.war.machine.y=290;c.war.mode=BOSS_SLAM;c.war.mode_timer=30;a.p[0].x=370;a.p[1].x=390;a.p[0].y=a.p[1].y=290;}}
 else campaign_hubs_start(&c,&a,HUB_FROG,0);
 unsigned bank=kind<3?CHAPTER_WAR:kind<5?CHAPTER_BOSS:CHAPTER_HUBS;chapter_assets_invalidate();assert(chapter_assets_load(bank));arena_render_invalidate_assets();arena_upload_background(scene,palette,a.map_id);
 double times[360],sims[360];unsigned maxrows=0,maxsprite=0,maxfg=0,maxtile=0;double totaltile=0;double totalrows=0,totalsprite=0,totalfg=0;int frames=kind==0?150:kind==1?126:kind==3?299:360;
 for(int t=0;t<frames;t++){
  uint16_t keys[2]={0};if(kind==5){keys[0]=t<170?RIGHT:LEFT;keys[1]=keys[0];}a.p[0].hp=a.p[1].hp=100;a.p[0].invulnerable=a.p[1].invulnerable=3;
  LARGE_INTEGER b;QueryPerformanceCounter(&b);campaign_step(&c,&a,keys);sims[t]=elapsed(b,freq);
  unsigned bt=war_diag_boss_tiles+war_diag_ram_tiles;unsigned bs=arena_render_sprite_decodes(),bf=arena_render_foreground_decodes();arena_render_campaign(&c);QueryPerformanceCounter(&b);arena_render(&a,pixels);arena_present_background();times[t]=elapsed(b,freq);
  unsigned rs=arena_render_sprite_decodes()-bs,rf=arena_render_foreground_decodes()-bf,rows=arena_render_terrain_rows();if(rs>maxsprite)maxsprite=rs;if(rf>maxfg)maxfg=rf;if(rows>maxrows)maxrows=rows;unsigned tiles=war_diag_boss_tiles+war_diag_ram_tiles-bt;if(tiles>maxtile)maxtile=tiles;totaltile+=tiles;totalrows+=rows;totalsprite+=rs;totalfg+=rf;
 }
 double cold=times[0];qsort(times,frames,sizeof(double),cmp);qsort(sims,frames,sizeof(double),cmp);
 printf("%s frames%d render_ms p50%.4f p95%.4f max%.4f cold%.4f sim_ms p50%.4f p95%.4f max%.4f rows_avg%.1f max%u sprites_avg%.2f max%u fg_avg%.2f max%u bossram_tiles_avg%.2f max%u\n",names[kind],frames,times[frames/2],times[frames*95/100],times[frames-1],cold,sims[frames/2],sims[frames*95/100],sims[frames-1],totalrows/frames,maxrows,totalsprite/frames,maxsprite,totalfg/frames,maxfg,totaltile/frames,maxtile);
}return 0;}
