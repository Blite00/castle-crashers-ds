#include "hud.h"
#include "menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "boss_meter_art.h"
int main(void){
    static uint16_t data[256*192+2],off[256*192];Arena a;memset(&a,0,sizeof a);
    assert(hud_item_hit(102,118)&&hud_item_hit(153,169));
    assert(!hud_item_hit(101,118)&&!hud_item_hit(154,118)&&!hud_item_hit(102,117)&&!hud_item_hit(102,170));
    assert(menu_text_width("EQUIPPED")<=72&&menu_text_width("NO ITEM")<=72);
    data[0]=0x1234;data[256*192+1]=0x5678;
    for(int on=0;on<2;on++){
        for(int i=1;i<=256*192;i++)data[i]=0x8000;
        a.p[0].bow_equipped=on;hud_render_item(data+1,&a.p[0]);
        assert(data[0]==0x1234&&data[256*192+1]==0x5678);
        for(int y=0;y<192;y++)for(int x=0;x<256;x++)if(!hud_item_hit(x,y))assert(data[1+y*256+x]==0x8000);
        if(!on)memcpy(off,data+1,sizeof off);else assert(memcmp(off,data+1,sizeof off));
        char path[80];snprintf(path,sizeof path,"build/item-hud-%d.ppm",on);FILE *f=fopen(path,"wb");assert(f);
        fprintf(f,"P6\n256 192\n255\n");for(int i=1;i<=256*192;i++){fputc((data[i]&31)*255/31,f);fputc(((data[i]>>5)&31)*255/31,f);fputc(((data[i]>>10)&31)*255/31,f);}fclose(f);
    }
    for(int i=1;i<=256*192;i++)data[i]=0x8000;
    hud_render_potions(data+1,0,0);
    for(int i=1;i<=256*192;i++)assert(data[i]==0x8000);
    for(int on=0;on<2;on++){
        for(int i=1;i<=256*192;i++)data[i]=0x8000;
        hud_render_potions(data+1,3,on);
        assert(data[0]==0x1234&&data[256*192+1]==0x5678);
        for(int y=0;y<192;y++)for(int x=0;x<256;x++)if(!hud_item_hit(x,y))assert(data[1+y*256+x]==0x8000);
        if(!on)memcpy(off,data+1,sizeof off);else assert(memcmp(off,data+1,sizeof off));
    }
    FILE *potion=fopen("build/potion-hud.ppm","wb");assert(potion);fprintf(potion,"P6\n256 192\n255\n");
    for(int i=1;i<=256*192;i++){fputc((data[i]&31)*255/31,potion);fputc(((data[i]>>5)&31)*255/31,potion);fputc(((data[i]>>10)&31)*255/31,potion);}fclose(potion);
    /* Empty lobby slots have no portrait or health bar; all choices retain HUD. */
    memset(data+1,0,sizeof off);hud_render(data+1,&a);
    for(int i=1;i<=256*192;i++)assert(data[i]==0);
    for(int knight=0;knight<4;knight++){
        memset(data+1,0,sizeof off);a.p[0].ready=1;a.p[0].knight_id=knight;
        a.p[0].hp=ARENA_HEALTH;a.p[0].mana=ARENA_MANA;hud_render(data+1,&a);
        assert(data[1+11*256+55]!=0&&data[1+24*256+55]!=0);
        for(int y=0;y<55;y++)for(int x=128;x<256;x++)assert(data[1+y*256+x]==0);
    }
    assert(hud_progress_percent(1,0)==1);
    assert(hud_progress_percent(1,95)==50);
    assert(hud_progress_percent(1,190)==100);
    assert(hud_progress_percent(2,191)==1);
    assert(hud_progress_percent(2,295)==50);
    assert(hud_progress_percent(2,400)==100);
    for(unsigned hero=0;hero<2;hero++){
        for(int i=1;i<=256*192;i++)data[i]=0x8000;
        hud_render_progress(data+1,hero,1,95);
        unsigned x=5+hero*128+74;
        for(unsigned y=0;y<3;y++)for(unsigned k=0;k<40;k++)
            assert(data[1+(40+y)*256+x+k]==(k<20?(y?0xfe5c:0xff3f):0x8c42));
        assert(data[0]==0x1234&&data[256*192+1]==0x5678);
    }
    /* Damage shrinks the same bar for solo/co-op and zero clears old fill.
       Boss drawing must not touch either player's health/inventory rows. */
    for(unsigned maximum=350;maximum<=450;maximum+=100){
        for(int i=1;i<=256*192;i++)data[i]=0x8000;
        /* Original meter: the lit red bar on its centre row shrinks with health; nothing is written outside it. */
        unsigned bar=BOSS_METER_FILL_HI-BOSS_METER_FILL_LO+1;
        #define LIT(row) ({unsigned n_=0;for(int x_=BOSS_METER_LEFT+BOSS_METER_FILL_LO;x_<=BOSS_METER_LEFT+BOSS_METER_FILL_HI;x_++)for(unsigned i_=1;i_<16;i_++)if((BOSS_METER_FILL_MASK>>i_&1)&&data[1+(row)*256+x_]==boss_meter_palette[i_]){n_++;break;}n_;})
        hud_render_boss(data+1,maximum,maximum);assert(LIT(BOSS_METER_TOP+BOSS_METER_H/2)==bar);
        hud_render_boss(data+1,maximum/2,maximum);assert(LIT(BOSS_METER_TOP+BOSS_METER_H/2)==bar/2);
        hud_render_boss(data+1,0,maximum);assert(LIT(BOSS_METER_TOP+BOSS_METER_H/2)==0);
        hud_render_boss(data+1,maximum,maximum);
        for(unsigned y=0;y<192;y++)if(y<BOSS_METER_TOP||y>=BOSS_METER_TOP+BOSS_METER_H)for(unsigned x=0;x<256;x++)assert(data[1+y*256+x]==0x8000);
        for(unsigned y=BOSS_METER_TOP;y<BOSS_METER_TOP+BOSS_METER_H;y++)for(unsigned x=0;x<256;x++)if(x<BOSS_METER_LEFT||x>=BOSS_METER_LEFT+BOSS_METER_W)assert(data[1+y*256+x]==0x8000);
        assert(data[0]==0x1234&&data[256*192+1]==0x5678);
    }
    for(unsigned notice=0;notice<2;notice++){
        for(int i=1;i<=256*192;i++)data[i]=0x8000;
        hud_render_level_notice(data+1,99,"Run + A / X",notice?60:140);
        for(unsigned y=0;y<192;y++)if(y<60||y>=100)for(unsigned x=0;x<256;x++)assert(data[1+y*256+x]==0x8000);
        assert(data[0]==0x1234&&data[256*192+1]==0x5678);
    }
    puts("PASS: bow item touch boundaries, level notices preserve health/items, solo/co-op boss bar damage/zero and clipped writes");return 0;
}
