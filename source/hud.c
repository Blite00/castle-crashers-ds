#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "hud.h"
#include "ui_skin.h"
#include "hud_art.h"
#include "item_art.h"
#include "loot_art.h"
#include "weapon_art.h"
#include "campaign_weapons.h"
#include "menu.h"
#include "campaign_sequence.h"
#include "progression_art.h"
#include "boss_meter_art.h"
#include <stdio.h>
#define RGB(r,g,b) (0x8000u|(r)|((g)<<5)|((b)<<10))
static void image(uint16_t *pixels,int id,int x,int y,int width){
    hud_draw_image(pixels,(unsigned)id,x,y,width);
}
void hud_render(uint16_t *pixels,const Arena *arena){
    for(int player=0;player<2;player++){
        const Fighter *p=&arena->p[player];int x=5+player*128;
        if(!p->ready)continue;
        /* Original portrait frame and meter positions, reduced to arena essentials. */
        for(int y=5;y<32;y++)for(int j=x+47;j<x+117;j++)pixels[y*256+j]=RGB(2,2,3);
        image(pixels,0,x,5,118);
        static const int portraits[4]={1,2,5,6};
        image(pixels,portraits[p->knight_id<4?p->knight_id:0],x+2,6,43);
        image(pixels,3,x+50,11,p->hp*64/ARENA_HEALTH);
        image(pixels,4,x+50,24,p->mana*64/ARENA_MANA);
        char score[16];snprintf(score,sizeof(score),"WINS %u",(unsigned)arena->wins[player]);
        if(arena->map_id<3)menu_text(pixels,x+49,34,score,RGB(31,29,20));
    }
}

int hud_item_hit(int x,int y){return x>=102&&x<154&&y>=118&&y<170;}
static void item_square(uint16_t *pixels,int equipped){
    ui_skin_tile(pixels,102,118,equipped);
}
static void item_image(uint16_t *pixels,const uint8_t *data,const uint16_t *palette,int w,int h){
    int left=128-w/2,top=140-h/2;
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){
        unsigned c=data[y*w+x];int dx=left+x,dy=top+y;
        if(c&&dx>=104&&dx<152&&dy>=120&&dy<157)pixels[dy*256+dx]=palette[c];
    }
}
void hud_render_item(uint16_t *pixels,const Fighter *player){
    item_square(pixels,player->bow_equipped);
    const ItemFrame *f=&item_frames[0];item_image(pixels,item_pixels+f->offset,item_palette,f->w,f->h);
    menu_text(pixels,117,153,"BOW",UI_INK);
}
void hud_render_potions(uint16_t *pixels,unsigned count,int equipped){
    if(!count)return;
    item_square(pixels,equipped);
    const ArtFrame *f=&loot_frames[LOOT_FRAME_POTION];item_image(pixels,loot_pixels+f->offset,loot_palette,f->w,f->h);
    char label[8];snprintf(label,sizeof(label),"x%u",count>5?5:count);
    menu_text(pixels,121,153,label,UI_INK);
}

void hud_render_weapon(uint16_t *pixels,unsigned slot){
    if(slot>=WEAPON_COUNT)slot=0;
    const WeaponIcon *icon=weapon_icon(slot);if(!icon)return;
    for(int y=118;y<170;y++)for(int x=34;x<86;x++)
        pixels[y*256+x]=(x<36||x>=84||y<120||y>=168)?RGB(31,26,8):RGB(6,8,11);
    for(int y=0;y<32;y++)for(int x=0;x<32;x++){
        unsigned c=icon->pixels[y*32+x];
        if(c)pixels[(122+y)*256+44+x]=icon->palette[c];
    }
    menu_text(pixels,42,157,"WEAPON",RGB(31,29,20));
}

/* MAIN HUD sprite277 names sprite196 "xp" at(74,35). Its shape195 is
 *40x3: RGB(255,204,255) top row, RGB(231,148,252) below. f_Exp selects
 * a1..100 frame from progress within this level, not cumulative XP. */
unsigned hud_progress_percent(unsigned level,unsigned xp){
    if(level<1)level=1;
    if(level>99)level=99;
    unsigned previous=keep_xp_threshold(level-1),span=190+20*(level-1);
    unsigned progress=xp>previous?xp-previous:0;
    unsigned percent=progress>=span?100:progress*100/span;
    return percent?percent:1;
}
void hud_render_progress(uint16_t *pixels,unsigned player,unsigned level,unsigned xp){
    if(player>=2)return;
    int left=5+(int)player*128;
    char label[8];snprintf(label,sizeof label,"%u",level>99?99:level<1?1:level);
    menu_text(pixels,left+50,34,label,RGB(31,27,10));
    unsigned width=hud_progress_percent(level,xp)*40/100;
    for(unsigned y=0;y<3;y++)for(unsigned x=0;x<40;x++)
        pixels[(40+y)*256+left+74+x]=x<width?(y?RGB(28,18,31):RGB(31,25,31)):RGB(2,2,3);
}


void hud_render_shovel(uint16_t *pixels,int equipped){
    ui_skin_tile(pixels,34,118,equipped);
    menu_text(pixels,42,138,"SHOVEL",UI_INK);
    if(equipped)menu_text(pixels,46,155,"Y DIG",UI_INK);
}

/* Original MAIN boss meter (ornate frame, angry medallion and red bar) at 60% scale.
   The red pixels right of the current health are dimmed, so the bar empties like the source. */
void hud_render_boss(uint16_t *pixels,unsigned hp,unsigned max_hp){
    if(!max_hp)return;
    if(hp>max_hp)hp=max_hp;
    const int left=BOSS_METER_LEFT,top=BOSS_METER_TOP,bar=BOSS_METER_FILL_HI-BOSS_METER_FILL_LO+1;
    int full=(int)(hp*(unsigned)bar/max_hp);if(hp&&!full)full=1;
    unsigned pos=0;
    for(unsigned i=0;i<sizeof boss_meter_runs;i++){
        unsigned idx=boss_meter_runs[i]&15,n=(boss_meter_runs[i]>>4)+1;
        for(;n;n--,pos++){
            if(!idx)continue;
            int x=(int)(pos%BOSS_METER_W),y=(int)(pos/BOSS_METER_W);
            uint16_t color=boss_meter_palette[idx];
            if((BOSS_METER_FILL_MASK>>idx&1)&&x>=BOSS_METER_FILL_LO&&x-BOSS_METER_FILL_LO>=full)color=RGB(5,2,2);
            pixels[(top+y)*256+left+x]=color;
        }
    }
}
void hud_render_level_notice(uint16_t *pixels,unsigned level,const char *combo_keys,unsigned timer){
    if(!timer)return;
    unsigned id=combo_keys&&timer<=180?PROGRESSION_NEW_COMBO:PROGRESSION_LEVEL_LABEL;
    uint8_t decoded[PROGRESSION_ART_MAX_PIXELS];
    const ProgressionArtFrame *f=&progression_frames[id];
    if(!progression_decode(id,decoded))return;
    for(int y=60;y<100;y++)for(int x=8;x<248;x++)pixels[y*256+x]=RGB(8,6,3);
    for(int y=62;y<98;y++)for(int x=10;x<246;x++)pixels[y*256+x]=RGB(29,27,21);
    unsigned left=(256-f->w)/2;
    for(unsigned y=0;y<f->h;y++)for(unsigned x=0;x<f->w;x++){
        unsigned color=decoded[y*f->w+x];if(color)pixels[(64+y)*256+left+x]=progression_palette[color];
    }
    char text[48];
    if(id==PROGRESSION_NEW_COMBO)snprintf(text,sizeof text,"%s",combo_keys);
    else snprintf(text,sizeof text,"Level %u - tap STATS for points",level);
    menu_text(pixels,(256-menu_text_width(text))/2,87,text,RGB(5,4,2));
}
