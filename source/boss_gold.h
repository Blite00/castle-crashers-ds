#ifndef BOSS_GOLD_H
#define BOSS_GOLD_H
#include "campaign.h"
static inline int campaign_boss_gold(const Campaign*c,unsigned index,BossGold*out){
 static const uint8_t frames[BOSS_GOLD_COUNT]={65,70,75,80,85,89,95,99,101,103,108,115,119,120,124,130,136,138,145,152,155,158,164,169,172,175,178,182,187};
 uint32_t mask=c->war.hit_seq[0]|((uint32_t)c->war.hit_seq[1]<<16);
 if(index>=BOSS_GOLD_COUNT||c->phase<BOSS_TREASURE||c->phase>BOSS_COMPLETE||(mask&(1u<<index)))return 0;
 unsigned clock=c->phase==BOSS_TREASURE?c->timer:512;
 if(clock<frames[index])return 0;
 uint32_t state=0x9e3779b9u^((unsigned)c->npc_ticks*0x85ebca6bu)^((index+1)*0xc2b2ae35u);
 state^=state>>16;state*=0x7feb352du;state^=state>>15;
 out->x=260+state%330;state=state*1664525u+1013904223u;out->y=285+((state>>8)%60);
 state=state*1664525u+1013904223u;unsigned roll=state>>16;
 out->kind=(roll&1)?DROP_BAG:(roll&2)?DROP_COIN:DROP_GEM;out->value=out->kind==DROP_BAG?5:out->kind==DROP_COIN?1:6;
 unsigned age=clock-frames[index];out->age=age>255?255:age;
 if(age<19){int startx=c->war.machine.x+c->war.machine.facing*30,starty=c->war.machine.y;out->x=startx+(out->x-startx)*(int)age/19;out->y=starty+(out->y-starty)*(int)age/19;}
 return 1;
}
static inline int campaign_boss_gold_height(const BossGold*gold){
 static const uint8_t bounce[20]={6,22,34,43,48,50,50,48,46,44,40,27,15,3,0,4,8,6,4,0};
 return gold->age<20?bounce[gold->age]:0;
}
#endif
