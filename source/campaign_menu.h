#ifndef CAMPAIGN_MENU_H
#define CAMPAIGN_MENU_H
#include "campaign.h"
void campaign_menu_draw(uint16_t *pixels,const Campaign *c,unsigned hero,int page,int selected,int waiting,int live);
int campaign_menu_hit(int x,int y); /* 0..3 stats,6 close,-1 none */
void campaign_stats_button(uint16_t *pixels,unsigned points);
#endif
