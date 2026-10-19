#ifndef CAMPAIGN_BREAKS_H
#define CAMPAIGN_BREAKS_H
#include "campaign.h"
#include "stage.h"
#include "campaign_weapons.h"
#include "../generated/break_points.h"
#include <string.h>
#include <stdlib.h>
/* Destructible scenery of Forest Entrance (grass tufts) and Thieves' Forest (grass tufts and treasure chests): the original
   f_RegisterGrass and f_SetChest objects, whose pieces are foreground layers of the baked level. A cut tuft or an opened chest is
   hidden by painting the ground over it; its index in the table of generated/break_points.h is the bit that is published:
   bits 0..10 ride in Campaign.broken and bits 11..18 in Campaign.npc_ticks, both already carried by the wire in these levels. */
static inline const BreakPoint *campaign_break_table(unsigned map,unsigned *count){
    if(map==13){*count=BREAK_THIEVES_COUNT;return break_thieves;}
    if(map==12){*count=BREAK_FOREST_COUNT;return break_forest;}
    *count=0;return 0;
}
static inline int campaign_break_find(unsigned map,unsigned source,int depth,int piece_x){
    unsigned n;const BreakPoint *t=campaign_break_table(map,&n);
    for(unsigned i=0;i<n;i++)if(t[i].depth==depth&&t[i].source_low==(source&255u)&&piece_x>=t[i].x-t[i].w/2-3&&piece_x<=t[i].x+t[i].w/2+3)return (int)i;
    return -1;
}
static inline int campaign_break_cut(const Campaign *c,unsigned i){return i<11?(c->broken>>i)&1:i<19?(c->npc_ticks>>(i-11))&1:0;}
static inline void campaign_break_set(Campaign *c,unsigned i){if(i<11)c->broken|=(uint16_t)(1u<<i);else if(i<19)c->npc_ticks|=(uint8_t)(1u<<(i-11));}
static inline unsigned campaign_break_count_cut(const Campaign *c){unsigned n=0;for(unsigned i=0;i<19;i++)n+=(unsigned)campaign_break_cut(c,i);return n;}

/* Host-only hit bookkeeping. */
static inline uint8_t *break_seen(unsigned i,unsigned h){static uint8_t seen[19][2];return &seen[i%19][h&1];}
static inline uint8_t *break_taken(unsigned i){static uint8_t taken[19];return &taken[i%19];}
static inline void campaign_break_reset(Campaign *c){for(unsigned i=0;i<19;i++){*break_taken(i)=0;*break_seen(i,0)=*break_seen(i,1)=0;}c->broken=0;c->npc_ticks=0;}
static inline void campaign_break_snap(const Arena *a,int *x,int *y){
    for(int r=0;r<60;r+=4)for(int dx=-r;dx<=r;dx+=4)for(int dy=-r;dy<=r;dy+=4){
        if(abs(dx)!=r&&abs(dy)!=r)continue;
        if(stage_position_open_map(a->map_id,*x+dx,*y+dy)){*x+=dx;*y+=dy;return;}
    }
}
static inline unsigned campaign_break_random(Campaign *c,unsigned n){c->rng=c->rng*1664525u+1013904223u;return (c->rng>>8)%n;}
/* Chests in x order. Thieves' Forest: weapon 37, weapon 42, then a chicken (LEVEL4 sprite188 item_type and weapon_type). */
static inline void campaign_break_open(Campaign *c,Arena *a,const BreakPoint *b,unsigned chest){
    int x=b->x,y=b->y;campaign_break_snap(a,&x,&y);
    if(chest==0)campaign_weapon_place(c,WEAPON_37,x,y);
    else if(chest==1)campaign_weapon_place(c,WEAPON_42,x,y);
    else campaign_queue_drop(c,x,y,DROP_CHICKEN,50);
}
static inline void campaign_break_step(Campaign *c,Arena *a){
    unsigned n;const BreakPoint *t=campaign_break_table(a->map_id,&n);unsigned chest=0;
    for(unsigned i=0;i<n;i++){
        unsigned this_chest=t[i].kind?chest++:0;
        if(campaign_break_cut(c,i))continue;
        Fighter object={.x=t[i].x,.y=t[i].y-6,.hp=1,.ready=1};
        for(int h=0;h<2;h++){
            Fighter *p=&a->p[h];
            if(!p->ready||p->hp<=0||!campaign_attack_contact(p)||p->attack_kind==HIT_NONE||*break_seen(i,(unsigned)h)==(uint8_t)p->attack_seq||!arena_melee_overlap(p,&object))continue;
            *break_seen(i,(unsigned)h)=(uint8_t)p->attack_seq;
            if(t[i].kind){
                if(++*break_taken(i)<3)continue;
                campaign_break_set(c,i);campaign_break_open(c,a,&t[i],this_chest);
            }else{
                campaign_break_set(c,i);
                if(campaign_break_random(c,3)==1){
                    int x=t[i].x,y=t[i].y-4;campaign_break_snap(a,&x,&y);
                    campaign_queue_drop(c,x,y,DROP_FOOD,10|(campaign_break_random(c,5)<<5)); /* Source: one tuft in three drops random food 2..6. */
                }
            }
            break;
        }
    }
}
#endif
