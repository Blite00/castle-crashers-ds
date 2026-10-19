/* Desktop CPU comparison only; this does not measure physical DS frame rate. */
#include <stdio.h>
#include <time.h>
#include "arena.h"
#include "render.h"
#include "campaign.h"
int main(void){static uint16_t pixels[256*192];for(int map=3;map<=5;map+=2){Arena a;Campaign c={0};arena_init(&a);a.map_id=map;a.running=1;c.phase=map==5?KEEP_BARRACKS_TRAVEL:KEEP_TRAVEL;arena_render_campaign(&c);clock_t start=clock();unsigned hash=0;for(int n=0;n<2400;n++){a.tick=n;a.p[0].x=map==5?300+n%90:100+n%2000;a.p[1].x=a.p[0].x+150;a.p[0].y=270;a.p[1].y=310;a.p[0].pose=a.p[1].pose=POSE_WALK;a.p[0].animation=a.p[1].animation=n;arena_render(&a,pixels);for(int i=0;i<49152;i+=97)hash=hash*33+pixels[i];}printf("map%d frames2400 CPUms%.2f hash%u\n",map,1000.0*(clock()-start)/CLOCKS_PER_SEC,hash);}return 0;}
