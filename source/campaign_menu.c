#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_menu.h"
#include "menu.h"
#include "ui_skin.h"
#include "ui_icon_art.h"
#include <stdio.h>
int campaign_menu_hit(int x,int y){
    if(x<0||x>=256||y<66||y>=190)return -1;
    if(y<88)return x>=214?6:-1;
    if(y>=90&&y<170)return (y-90)/20;
    return -1;
}
void campaign_stats_button(uint16_t *p,unsigned points){
    ui_skin_tile(p,170,118,points!=0);
    menu_text(p,176,130,"STATS",UI_INK);char text[12];snprintf(text,sizeof text,"+%u",points);menu_text(p,187,149,text,UI_INK);
}
void campaign_menu_draw(uint16_t *p,const Campaign *c,unsigned h,int page,int selected,int waiting,int live){
    if(h>=2)return;
    ui_skin_box(p,0,62,256,113,UI_PAPER);
    ui_skin_panel(p,6,64,244,24);
    menu_text(p,14,65,"ATTRIBUTES",UI_INK);menu_text(p,217,65,"BACK",UI_INK);
    /* Original combo slots 1..6 unlock at these knight levels (f_SetupPlayer combo_unlocks). */
    {static const uint8_t slots[6]={2,4,8,16,32,50};unsigned level=c->profiles[h].level,x=14;
     menu_text(p,x,76,"NEW COMBO LV",UI_INK);x+=menu_text_width("NEW COMBO LV")+6;
     for(unsigned i=0;i<6;i++){char t[4];snprintf(t,sizeof t,"%u",slots[i]);
      menu_text(p,x,76,t,level>=slots[i]?UI_INK:UI_RGB(18,15,10));x+=menu_text_width(t)+7;}}
    static const char *names[]={"Strength","Defense","Magic","Agility"};
    uint8_t icon[UI_ICON_PIXELS];
    for(int i=0;i<4;i++){
        int y=90+i*20;char text[16];
        ui_skin_box(p,8,y,240,19,i==selected?UI_GOLD:UI_LIGHT);
        ui_skin_box(p,10,y+1,18,17,UI_INK);
        ui_icon_decode((unsigned)i,icon);
        for(int yy=0;yy<16;yy++)for(int x=0;x<16;x++)if(icon[yy*16+x])p[(y+1+yy)*256+11+x]=ui_icon_palette[icon[yy*16+x]];
        menu_text(p,33,y+2,names[i],UI_INK);
        snprintf(text,sizeof text,"%u / 25",1+c->stats[h][i]);menu_text(p,170,y+2,text,UI_INK);
        for(unsigned j=0;j<25;j++)ui_skin_box(p,34+j*5,y+14,4,3,j<=c->stats[h][i]?UI_INK:UI_RGB(22,20,15));
        if(campaign_points(c,h)&&c->stats[h][i]<24)menu_text(p,230,y+2,"+",UI_INK);
        if(i==2){/* Magic stat gates: air cast 10, splash 15, elemental chain 20. */
            static const uint8_t gate[3]={10,15,20};static const char *label[3]={"Air","Spl","Ele"};unsigned x=menu_text_width(names[i])+39;
            for(unsigned k=0;k<3;k++){char t[12];snprintf(t,sizeof t,"%s%u",label[k],gate[k]);
                menu_text(p,x,y+2,t,1+c->stats[h][i]>=gate[k]?UI_INK:UI_RGB(18,15,10));x+=menu_text_width(t)+4;}
        }
    }
    char text[64];
    if(waiting)snprintf(text,sizeof text,"Waiting for host...");
    else snprintf(text,sizeof text,"%u points - tap stat or A",campaign_points(c,h));
    ui_skin_box(p,0,175,256,17,UI_INK);ui_skin_text(p,8,179,240,text,UI_LIGHT);
    (void)live;(void)page;
}
