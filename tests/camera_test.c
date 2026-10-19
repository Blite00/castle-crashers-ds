#include <assert.h>
#include <stdio.h>
#include "camera.h"
#include "stage.h"
int main(void){
    Arena a;arena_init(&a);ArenaCamera c={0};
    unsigned rng=9;
    for(int i=0;i<60000;i++){
        for(int p=0;p<2;p++){
            rng=rng*1664525u+1013904223u;
            a.p[p].x=ARENA_MIN_X+rng%(ARENA_MAX_X-ARENA_MIN_X+1);
            rng=rng*1664525u+1013904223u;
            a.p[p].y=ARENA_MIN_Y+rng%(ARENA_MAX_Y-ARENA_MIN_Y+1);
            a.p[p].z=(rng>>16)%81;
        }
        camera_update(&c,&a);
        assert(c.width>=288&&c.width<=WORLD_WIDTH&&c.height*4==c.width*3);
        assert(c.x>=0&&c.y>=0&&c.x+c.width<=WORLD_WIDTH&&c.y+c.height<=WORLD_HEIGHT);
        for(int p=0;p<2;p++){
            assert(a.p[p].x-32>=c.x&&a.p[p].x+32<=c.x+c.width);
            assert(a.p[p].y-a.p[p].z-56>=c.y&&a.p[p].y+8<=c.y+c.height);
        }
    }
    a.p[0].x=ARENA_MIN_X;a.p[1].x=ARENA_MAX_X;
    a.p[0].y=a.p[1].y=239;a.p[0].z=a.p[1].z=0;
    camera_update(&c,&a);assert(c.width==WORLD_WIDTH);
    a.p[0].x=200;a.p[1].x=220;
    camera_update(&c,&a);assert(c.width==WORLD_WIDTH-4);
    for(int i=0;i<100;i++)camera_update(&c,&a);
    assert(c.width==288);
    a.p[0].x=a.p[1].x=ARENA_MIN_X;
    for(int i=0;i<100;i++)camera_update(&c,&a);
    assert(c.x==0);
    a.p[0].x=a.p[1].x=ARENA_MAX_X;
    for(int i=0;i<100;i++)camera_update(&c,&a);
    assert(c.x+c.width==WORLD_WIDTH);
    a.map_id=3;c.ready=0;
    for(int x=40;x<CAMPAIGN_WORLD_WIDTH-40;x+=7){
        a.p[0].x=x;a.p[1].x=x+20;a.p[0].y=a.p[1].y=350;
        camera_update(&c,&a);
        assert(c.x>=0&&c.x+c.width<=CAMPAIGN_WORLD_WIDTH);
        assert(c.y>=0&&c.y+c.height<=CAMPAIGN_WORLD_HEIGHT);
        assert(a.p[0].x>=c.x&&a.p[1].x<c.x+c.width);
    }
    assert(c.x>WORLD_WIDTH);
    a.p[0].x=1800;a.p[0].hp=100;a.p[1].x=100;a.p[1].hp=0;
    camera_update(&c,&a);
    assert(a.p[0].x>=c.x&&a.p[0].x<c.x+c.width);
    a.p[0].hp=0;a.p[1].hp=100;a.p[1].x=2200;
    camera_update(&c,&a);
    assert(a.p[1].x>=c.x&&a.p[1].x<c.x+c.width);
    /* The first War encounter ends at x482. Following must stop before the
       hero reaches that boundary, leaving the whole visible arena usable. */
    a.map_id=6;a.round=6;a.p[1].ready=0;a.p[0].ready=1;a.p[0].hp=100;
    a.p[0].y=280;a.p[0].z=0;c=(ArenaCamera){0};
    for(int x=250;x<=482;x++){
        a.p[0].x=x;camera_update(&c,&a);camera_limit_horizontal(&c,&a,0,482+32);
        assert(c.x+c.width<=514);assert(x+32<=c.x+c.width);
    }
    assert(c.x+c.width==514);assert((a.p[0].x-c.x)*256/c.width>=220);
    /* The left and right limits also survive co-op zoom and scripted pans. */
    c=(ArenaCamera){.x=4000,.y=-40,.width=424,.height=318};
    camera_limit_horizontal(&c,&a,2010,3311);assert(c.x==2887&&c.y==0);
    c.x=-100;c.y=900;a.map_id=10;camera_clamp_world(&c,&a);
    assert(c.x==136&&c.y==448-318);
    stage_set_map(10);a.p[0].x=203;a.p[0].y=269;a.p[0].pose=POSE_IDLE;
    a.p[0].cooldown=0;a.running=1;a.countdown=0;a.winner=0;
    for(int t=0;t<90;t++){uint16_t keys[2]={LEFT,0};arena_step(&a,keys);}
    assert(a.p[0].x>=168&&a.p[0].x-136>=32);
    assert(a.p[0].x-152<24); /* Authored return portal remains reachable. */
    /* A jumping knight must not bob the picture vertically. */
    a.map_id=3;c.ready=0;a.p[0].x=a.p[1].x=1000;a.p[0].y=a.p[1].y=350;a.p[0].z=a.p[1].z=0;
    camera_update(&c,&a);{int y0=c.y,w0=c.width;
    for(int z=0;z<=70;z+=5){a.p[0].z=z;camera_update(&c,&a);assert(c.y==y0&&c.width==w0);}}
    a.p[0].z=a.p[1].z=0;
    c.x=-240;c.y=-120;a.map_id=3;camera_clamp_world(&c,&a);
    assert(c.x==0&&c.y==0);
    puts("PASS: camera framing, jumps, scrolling, encounter edges, scripted bounds and 60000 pairs");
}
