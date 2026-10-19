#include <stdio.h>
#include "arena.h"
#include "render.h"
/* Scripted host-rendered preview, not an emulator or hardware recording. */
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *out=fopen(argv[1],"wb");if(!out)return 1;
    Arena a;arena_init(&a);arena_start_round(&a);a.countdown=0;a.tick=120;
    a.p[0].x=100;a.p[0].y=285;a.p[1].x=320;a.p[1].y=285;
    uint16_t frame[256*192];
    for(int t=0;t<150;t++){
        uint16_t keys[2]={0,0};
        if(t<28)keys[0]=RIGHT;
        if(t==28)keys[0]=JUMP;
        if(t>=60&&t<80)keys[0]=LEFT;
        if(t>=90&&t<132)keys[0]=ATTACK;
        arena_step(&a,keys);arena_render(&a,frame);
        for(int i=0;i<256*192;i++){
            fputc((frame[i]&31)*255/31,out);
            fputc(((frame[i]>>5)&31)*255/31,out);
            fputc(((frame[i]>>10)&31)*255/31,out);
        }
    }
    fclose(out);return 0;
}
