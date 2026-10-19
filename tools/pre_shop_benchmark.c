/* Compare native CPU/work counters, not physical DS FPS. */
#include "render.h"
#include "stage.h"
#include <time.h>
#include <assert.h>
#include <stdio.h>
int main(void){
 static uint16_t pixels[256*192];Arena a;Campaign c={0};arena_init(&a);a.map_id=3;a.running=1;a.p[1].ready=0;c.phase=KEEP_TRAVEL;c.group=4;c.broken=1;arena_render_campaign(&c);
 unsigned before=arena_render_foreground_decodes();clock_t start=clock();
 for(unsigned n=0;n<1200;n++){
  a.tick=600+n;a.p[0].x=2180+n%200;a.p[0].y=235;a.p[0].pose=POSE_WALK;a.p[0].animation=n;
  arena_render(&a,pixels);
 }
 assert(arena_render_foreground_decodes()-before<1800);
 printf("pre-shop1200frames fg%u CPUms%.1f\n",arena_render_foreground_decodes()-before,1000.0*(clock()-start)/CLOCKS_PER_SEC);
 return 0;
}
