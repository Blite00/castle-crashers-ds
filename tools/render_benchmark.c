#include <stdio.h>
#include <time.h>
#include "arena.h"
#include "render.h"
/* Desktop cost comparison only: no claim of emulated or physical DS FPS. */
int main(void){
    static uint16_t pixels[256*192];
    Arena a;arena_init(&a);a.running=1;a.tick=120;
    unsigned long spans=0;unsigned checksum=0;
    clock_t start=clock();
    for(int frame=0;frame<6000;frame++){
        a.map_id=(frame/2000)%3;a.tick=120+frame;
        for(int i=0;i<2;i++){
            a.p[i].x=i?290-(frame%100):100+(frame%100);
            a.p[i].y=260+frame%35;a.p[i].z=i?0:frame%50;
            a.p[i].previous=MAGIC;a.p[i].pose=POSE_AIRSPIN;a.p[i].animation=frame%13;
        }
        for(int i=0;i<ARENA_SHOTS;i++)a.shots[i]=(Projectile){.x=60+i*50,.y=270,.z=14,.life=20,.vx=i%2?5:-5,.owner=i%2};
        arena_render(&a,pixels);
        const uint16_t *left=arena_dirty_left(),*right=arena_dirty_right();
        for(int y=0;y<192;y++)if(right[y]>left[y])spans+=(right[y]-left[y])*2;
        for(int i=0;i<256*192;i+=97)checksum=checksum*33u+pixels[i];
    }
    printf("{\"frames\":6000,\"cpu_ms\":%.2f,\"average_dirty_bytes\":%lu,\"checksum\":%u}\n",1000.0*(clock()-start)/CLOCKS_PER_SEC,spans/6000,checksum);
    return 0;
}
