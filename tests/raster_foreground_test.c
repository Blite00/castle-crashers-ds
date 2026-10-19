#include "raster.h"
#include <assert.h>
#include <stdio.h>
static int hidden(int x,int y){return x>=101&&y>=100;}
int main(void){
    static uint16_t buffer[256*192+2];uint8_t sprite[4]={1,1,1,1};
    uint16_t front[2]={0,0x801f},back[2]={0,0xfc00};
    RasterView v;ArenaCamera c={.width=256,.height=192};buffer[0]=0x1234;buffer[256*192+1]=0x5678;
    raster_init(&v,buffer+1,&c);
    raster_sprite(&v,sprite,front,100,100,2,2,0,16,2,100);
    v.foreground_reject=hidden;
    raster_sprite(&v,sprite,back,100,100,2,2,0,16,2,99);
    assert(buffer[1+100*256+100]==0xfc00);
    assert(buffer[1+100*256+101]==0x801f); /* existing front actor survives */
    assert(buffer[1+101*256+101]==0x801f);
    assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    raster_init(&v,buffer+1,&c);
    v.dirty_left[100]=101;v.dirty_right[100]=102;
    raster_foreground(&v,sprite,back,100,100,2,2);
    assert(buffer[1+100*256+101]==0xfc00);
    assert(buffer[1+101*256+101]==0x801f);
    for(int row=0;row<192;row++){
        assert(v.dirty_left[row]==(row==100?101:256));
        assert(v.dirty_right[row]==(row==100?102:0));
    }
    assert(buffer[0]==0x1234&&buffer[256*192+1]==0x5678);
    puts("PASS: foreground rejection preserves already drawn front actors");
}
