#include "campaign_combat.h"
#include "stage.h"
#include "campaign_weapons.h"
#include <stdlib.h>
#include <string.h>

/* Source enemies have separate punch/wait clocks. These deterministic timing
   values and one-attacker budget are DS campaign balance adaptations, not a
   reconstruction of the original partially decompiled AI scheduler. */
enum { WINDUP_TICKS=10, RECOVERY_TICKS=30, PRESSURE_TICKS=30, MERCY_TICKS=24 };
void campaign_combat_reset(CampaignCombatState *state){memset(state,0,sizeof(*state));}
static int recovering(const Fighter *p){
    return p->hitstun||p->freeze||p->pose==POSE_KNOCKDOWN||p->pose==POSE_ROLL||p->pose==POSE_AIRHURT;
}
static uint16_t approach(const Fighter *p,const Fighter *q,int distance){
    /* Barbarian source speed is 2..4.9 original units. Three of four player
       movement ticks approximates its slower half-scale pace without new physics. */
    int dx=q->x-p->x,dy=q->y-p->y;uint16_t k=0;
    if(dx>distance)k|=RIGHT;else if(dx< -distance)k|=LEFT;
    if(dy>5)k|=DOWN;else if(dy< -5)k|=UP;
    int mx=!!(k&RIGHT)-!!(k&LEFT),my=!!(k&DOWN)-!!(k&UP);
    Fighter probe=*p;probe.x+=mx*3;probe.y+=my*2;
    stage_move(&probe,p->x,p->y);
    if(mx&&probe.x==p->x)k&=~(LEFT|RIGHT);
    if(my&&probe.y==p->y)k&=~(UP|DOWN);
    return k;
}
static uint16_t reposition(const Fighter *p,const Fighter *q,const Fighter *hunter,int priority,int slot,unsigned tick){
    /* MAIN f_SetTargets assigns90/150/250 source-unit staging distances.
       f_EnemyWalkInit priority2 flanks opposite the primary hunter and offsets
       depth by100-|dx|/2 near the prey. Half-scale roles retain that geometry;
       alternate depth sides and deterministic timing are DS adaptations. */
    Fighter goal=*q;int lane=((slot+tick/90)&1)?1:-1;
    int distance=priority==1?45:priority==2?75:125;
    int side=p->x<q->x?-1:1;
    if(priority==2&&hunter)side=hunter->x<q->x?1:-1;
    int dx=abs(p->x-q->x),depth=priority>1&&dx<100?50-dx/2:25;
    goal.x=q->x+side*distance;goal.y=q->y+lane*depth;
    uint16_t k=approach(p,&goal,3);
    if(!(k&(UP|DOWN))&&abs(p->y-goal.y)>5){
        goal.y=q->y-lane*depth;uint16_t alternate=approach(p,&goal,3);
        if(alternate&(UP|DOWN))k=alternate;
    }
    return k;
}
static int target_slot(uint8_t target){return target==8?2:target<2?target:0;}
static void move_enemy(uint16_t *input,uint8_t *walking,uint16_t direction,unsigned tick){
    *walking=!!direction;*input=tick%4==3?0:direction;
}
/* PLAYER DefineSprite394 removes the body at authored frame44. */
static void advance_corpses(CampaignCombatState *state,Fighter enemies[6],Fighter *ally){
    for(int i=0;i<7;i++){
        Fighter *p=i<6?&enemies[i]:ally;
        if(!p->ready||p->hp){state->death_age[i]=0;continue;}
        if(p->z||p->pose!=POSE_DEAD)continue;
        if(state->death_age[i]<44)state->death_age[i]++;
        p->animation=state->death_age[i];
        if(state->death_age[i]>=44)p->ready=0;
    }
}
static void combat_step(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2],uint8_t landed_hits[2]){
    stage_set_map(heroes->map_id);
    uint16_t input[9]={keys[0],keys[1],0,0,0,0,0,0,0};uint8_t selected[9]={2,2,0,0,0,0,0,0,2};
    uint16_t hero_hits[3]={heroes->p[0].hit_seq,heroes->p[1].hit_seq,ally->hit_seq},enemy_hits[6];
    unsigned occupied[3]={0,0,0};int primary[3]={-1,-1,-1};uint8_t priority[6]={0};
    uint8_t walking[6]={0};int16_t old_pose[6],old_animation[6];
    Fighter *friends[3]={&heroes->p[0],&heroes->p[1],ally};
    for(int h=0;h<3;h++){
        if(state->mercy[h])state->mercy[h]--;
        if(state->pressure[h])state->pressure[h]--;
        int nearest=100000;
        for(int e=0;e<6;e++)if(enemies[e].knight_id!=3&&enemies[e].ready&&enemies[e].hp){
            int distance=abs(enemies[e].x-friends[h]->x)+2*abs(enemies[e].y-friends[h]->y);
            if(distance<nearest){nearest=distance;selected[h<2?h:8]=e+2;}
        }
    }
    /* Source prey/priority assignments persist between task changes. A bounded
       reevaluation interval and hysteresis avoid frame-by-frame co-op jitter;
       the30/24 values are adaptations, not extracted source constants. */
    for(int e=0;e<6;e++){
        Fighter *p=&enemies[e];int target=target_slot(targets[e]);
        enemy_hits[e]=p->hit_seq;old_pose[e]=p->pose;old_animation[e]=p->animation;
        if(state->recovery[e])state->recovery[e]--;
        if(state->shot_wait[e])state->shot_wait[e]--;
        if(state->target_clock[e])state->target_clock[e]--;
        if(p->knight_id==3)continue;
        int valid=friends[target]->ready&&friends[target]->hp;
        if(!valid&&state->windup[e]){state->windup[e]=0;state->recovery[e]=15;if(p->attack_kind==HIT_NONE)p->cooldown=0;}
        if(!p->cooldown&&!p->hitstun&&!state->windup[e]&&(!state->target_clock[e]||!valid)){
            int best=valid?abs(p->x-friends[target]->x)+2*abs(p->y-friends[target]->y):100000;
            for(int h=0;h<3;h++)if(friends[h]->ready&&friends[h]->hp){
                int distance=abs(p->x-friends[h]->x)+2*abs(p->y-friends[h]->y);
                if(distance+(valid?24:0)<best){best=distance;target=h;valid=1;}
            }
            state->target_clock[e]=30;
        }
        targets[e]=target<2?target:8;
        if(!p->ready||!p->hp)continue;
        if(state->windup[e]||(p->cooldown&&p->attack_kind!=HIT_NONE)){occupied[target]++;primary[target]=e;}
    }
    for(int h=0;h<3;h++){
        if(primary[h]<0){int nearest=100000;
            for(int e=0;e<6;e++)if(enemies[e].knight_id!=3&&enemies[e].ready&&enemies[e].hp&&target_slot(targets[e])==h){
                int distance=abs(enemies[e].x-friends[h]->x)+2*abs(enemies[e].y-friends[h]->y);
                if(distance<nearest){nearest=distance;primary[h]=e;}
            }
        }
        int rank=2;
        for(int e=0;e<6;e++)if(enemies[e].knight_id!=3&&enemies[e].ready&&enemies[e].hp&&target_slot(targets[e])==h)priority[e]=e==primary[h]?1:rank++;
    }
    /* Rotating first refusal keeps one slot from monopolizing the attack turn. */
    for(int n=0;n<6;n++){
        int e=(n+heroes->tick/60)%6;Fighter *p=&enemies[e];
        if(p->pose==POSE_CLASH){state->windup[e]=state->recovery[e]=state->guard[e]=state->ranged[e]=0;continue;}
        if(!p->ready||!p->hp){state->windup[e]=state->recovery[e]=state->guard[e]=state->ranged[e]=0;continue;}
        if(p->knight_id==2)continue;
        if(p->knight_id==3){
            int nearest=100000,chosen=-1;
            for(int j=0;j<6;j++)if(enemies[j].knight_id!=3&&enemies[j].ready&&enemies[j].hp){
                int distance=abs(enemies[j].x-p->x)+2*abs(enemies[j].y-p->y);
                if(distance<nearest){nearest=distance;chosen=j;}
            }
            if(chosen>=0&&!recovering(p)){
                Fighter *q=&enemies[chosen];selected[e+2]=chosen+2;
                if(state->windup[e]){
                    if(!--state->windup[e]){input[e+2]=ATTACK|ATTACK_EDGE;p->combo=p->combo_timer=p->heavy_chain=0;state->recovery[e]=24;}
                }else if(!p->cooldown&&!state->recovery[e]){
                    move_enemy(&input[e+2],&walking[e],approach(p,q,30),heroes->tick+e);
                    if(abs(q->x-p->x)<=42&&abs(q->y-p->y)<=10&&!q->invulnerable){p->facing=q->x<p->x?-1:1;state->windup[e]=6;input[e+2]=0;walking[e]=0;}
                }
            }else state->windup[e]=0;
            continue;
        }
        int target=target_slot(targets[e]);
        targets[e]=target<2?target:8;selected[e+2]=targets[e];Fighter *q=friends[target];
        if(!q->ready||!q->hp||recovering(p)){
            if(state->windup[e]){state->windup[e]=0;state->recovery[e]=15;}
            continue;
        }
        /* Barbarian configuration enables blocking (odds12) and30-tick holds.
           Observe an already visible frontal swing once, never input buttons;
           deterministic selection replaces the source random block roll. */
        if(!p->cooldown&&!state->windup[e]&&q->cooldown&&q->animation>=2&&q->attack_kind!=HIT_NONE&&q->attack_kind!=HIT_BOW&&q->attack_seq!=state->observed_attack[e]){
            state->observed_attack[e]=q->attack_seq;
            unsigned roll=q->attack_seq*13u+(unsigned)e*7u;
            if(roll%12==0&&abs(q->x-p->x)<=80&&abs(q->y-p->y)<=10&&(p->x-q->x)*q->facing>0&&!q->z){
                state->guard[e]=30;p->facing=q->x<p->x?-1:1;
            }
        }
        if(state->guard[e]){state->guard[e]--;input[e+2]=BLOCK;continue;}
        if(recovering(q)||state->mercy[target]){
            if(state->windup[e]){state->windup[e]=0;state->recovery[e]=15;}
            if(!p->cooldown)move_enemy(&input[e+2],&walking[e],reposition(p,q,primary[target]>=0?&enemies[primary[target]]:NULL,priority[e],e,heroes->tick),heroes->tick+e);
            continue;
        }
        if(state->windup[e]){
            /* The held first attack frame is a readable anticipation. Direction
               locks here: stepping away makes this swing miss. */
            if(!--state->windup[e]){
                if(state->ranged[e]){
                    p->bow_equipped=1;input[e+2]=BOW|BOW_EDGE;state->shot_wait[e]=120;
                    p->combo=p->combo_timer=0;
                }else{
                    input[e+2]=ATTACK|ATTACK_EDGE;
                    /* f_PunchSet100 has three ground slashes. Existing standard
                       light poses approximate its separate punch100 artwork. */
                    p->combo=(p->attack_seq+e)%3;p->combo_timer=20;
                }
                p->heavy_chain=0;
                state->recovery[e]=RECOVERY_TICKS;state->pressure[target]=PRESSURE_TICKS;
            }
            continue;
        }
        if(p->cooldown)continue;
        p->bow_equipped=0;
        int busy=occupied[target]||state->pressure[target]||state->recovery[e];
        move_enemy(&input[e+2],&walking[e],busy?reposition(p,q,primary[target]>=0?&enemies[primary[target]]:NULL,priority[e],e,heroes->tick):approach(p,q,34),heroes->tick+e);
        int dx=q->x-p->x,dy=q->y-p->y;
        int free_shot=0,enemy_shot=0;
        for(int s=0;s<2;s++){free_shot+=!heroes->shots[s].life;enemy_shot+=heroes->shots[s].life&&heroes->shots[s].owner>=2;}
        int ranged=priority[e]==2&&!state->shot_wait[e]&&free_shot&&!enemy_shot&&abs(dx)>=70&&abs(dx)<=180&&abs(dy)<=10;
        if(!busy&&(abs(dx)<=42||ranged)&&abs(dy)<=10&&!q->invulnerable&&!q->z){
            state->ranged[e]=ranged;
            p->facing=dx<0?-1:1;state->windup[e]=WINDUP_TICKS;occupied[target]++;
            input[e+2]=0;walking[e]=0;
        }
    }
    if(state->ally_recovery)state->ally_recovery--;
    if(ally->ready&&ally->hp&&!recovering(ally)){
        Fighter *q=&enemies[selected[8]-2];
        if(q->knight_id!=3&&q->ready&&q->hp){
            if(state->ally_windup){
                if(!--state->ally_windup){input[8]=ATTACK|ATTACK_EDGE;ally->combo=ally->combo_timer=ally->heavy_chain=0;state->ally_recovery=24;}
            }else if(!ally->cooldown&&!state->ally_recovery){
                input[8]=approach(ally,q,30);
                if(abs(q->x-ally->x)<=42&&abs(q->y-ally->y)<=10&&!q->invulnerable){
                    ally->facing=q->x<ally->x?-1:1;state->ally_windup=6;input[8]=0;
                }
            }
        }else state->ally_windup=0;
    }else state->ally_windup=0;
    arena_group_step_attributes(heroes,enemies,ally,input,selected,levels,attributes,landed_hits);
    advance_corpses(state,enemies,ally);
    for(int h=0;h<3;h++)if(hero_hits[h]!=friends[h]->hit_seq){
        state->mercy[h]=MERCY_TICKS;
        /* Co-op overlap can expose a hero to another enemy's committed swing. */
        if(friends[h]->hp&&friends[h]->invulnerable<15)friends[h]->invulnerable=15;
    }
    for(int e=0;e<6;e++){
        Fighter *p=&enemies[e];
        if(p->pose==POSE_CLASH){state->windup[e]=state->guard[e]=state->ranged[e]=0;continue;}
        if(enemy_hits[e]!=p->hit_seq||recovering(p)||!p->hp){state->windup[e]=state->guard[e]=state->ranged[e]=0;if(state->recovery[e]<15)state->recovery[e]=15;}
        if(walking[e]&&!p->cooldown&&!recovering(p)&&p->hp){
            p->pose=POSE_WALK;p->animation=old_pose[e]==POSE_WALK?(old_animation[e]+1)%30000:0;
        }
        if(state->windup[e]){
            p->pose=state->ranged[e]?POSE_BOW:POSE_ATTACK;p->animation=0;p->cooldown=1;p->attack_kind=HIT_NONE;
        }
    }
    if(hero_hits[2]!=ally->hit_seq||recovering(ally)||!ally->hp){state->ally_windup=0;state->ally_recovery=15;}
    if(state->ally_windup){ally->pose=POSE_ATTACK;ally->animation=0;ally->cooldown=1;ally->attack_kind=HIT_NONE;}

}

void campaign_combat_step(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],uint8_t landed_hits[2]){
 combat_step(state,heroes,enemies,ally,targets,keys,levels,0,landed_hits);
}
void campaign_combat_step_weapons(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],const uint8_t weapons[2],uint8_t landed_hits[2]){
 ArenaCampaignAttributes attributes[2];
 for(int h=0;h<2;h++){
  const CampaignWeapon *w=campaign_weapon_info(weapons[h]);if(!w)w=campaign_weapon_info(0);
  attributes[h]=(ArenaCampaignAttributes){campaign_weapon_stat(1,w->strength),campaign_weapon_stat(1,w->defense),campaign_weapon_stat(1,w->magic),campaign_weapon_stat(1,w->agility),w->critical_divisor};
 }
 combat_step(state,heroes,enemies,ally,targets,keys,levels,attributes,landed_hits);
}

void campaign_combat_step_attributes(CampaignCombatState *state,Arena *heroes,Fighter enemies[6],Fighter *ally,uint8_t targets[6],const uint16_t keys[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2],uint8_t landed_hits[2]){
 combat_step(state,heroes,enemies,ally,targets,keys,levels,attributes,landed_hits);
}
