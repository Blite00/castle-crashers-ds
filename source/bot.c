#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "bot.h"
#include "stage.h"
#include <stdlib.h>

const char *bot_difficulty_name(int difficulty){
    static const char *names[]={"Easy","Medium","Hard"};
    return names[difficulty>=0&&difficulty<BOT_COUNT?difficulty:BOT_MEDIUM];
}
uint16_t bot_input(const Arena *a,int difficulty){
    if(difficulty<0||difficulty>=BOT_COUNT)difficulty=BOT_MEDIUM;
    const unsigned reaction[]={12,6,3};
    unsigned cadence=reaction[difficulty];
    const Fighter *p=&a->p[1],*q=&a->p[0];uint16_t k=0;
    if(a->clash_timer)return a->tick%(difficulty==BOT_EASY?12:difficulty==BOT_MEDIUM?6:4)==0?ATTACK|ATTACK_EDGE:0;
    if(difficulty!=BOT_EASY&&a->tick%cadence==0&&p->pose==POSE_KNOCKDOWN&&p->cooldown<16)return JUMP|(p->x<q->x?LEFT:RIGHT);
    int dx=q->x-p->x,dy=q->y-p->y;
    if(dx>23)k|=RIGHT;else if(dx< -23)k|=LEFT;
    else if((dx>0&&p->facing<0)||(dx<0&&p->facing>0))k|=dx>0?RIGHT:LEFT;
    if(dy>4)k|=DOWN;else if(dy< -4)k|=UP;
    /* Basic practice steering around the original arena rocks. */
    int move_x=!!(k&RIGHT)-!!(k&LEFT),move_y=!!(k&DOWN)-!!(k&UP);
    if(move_x&&!stage_position_open(p->x+move_x*3,p->y)){
        k&=~(LEFT|RIGHT);k|=p->y<235?UP:DOWN;
    }
    if(move_y&&!stage_position_open(p->x,p->y+move_y*2)){
        k&=~(UP|DOWN);k|=p->x<196?LEFT:RIGHT;
    }
    /* Movement remains smooth; defensive and attack decisions have reaction gaps. */
    if(difficulty!=BOT_EASY&&q->cooldown&&abs(dx)<65&&abs(dy)<12&&dx*p->facing>0&&a->tick%90<(difficulty==BOT_HARD?60:20))return k|BLOCK;
    if(a->tick%cadence)return k;
    if(dx<45&&dx> -45&&abs(dy)<12){
        /* CPU sends deliberate presses too; holding no longer repeats attacks. */
        if(!p->cooldown||(difficulty!=BOT_EASY&&p->cooldown<=5))k|=difficulty!=BOT_EASY&&p->combo==1?(HEAVY|HEAVY_EDGE):(ATTACK|ATTACK_EDGE);
    }else if(difficulty!=BOT_EASY&&abs(dy)<12&&a->tick%(difficulty==BOT_HARD?45:90)==0)k|=MAGIC|ATTACK|ATTACK_EDGE;
    else if(difficulty!=BOT_EASY&&a->tick%60==30&&!p->cooldown&&abs(q->y-p->y)<12)k|=p->bow_equipped?(BOW|BOW_EDGE):(EQUIP|EQUIP_EDGE);
    if(difficulty!=BOT_EASY&&q->z&&a->tick%(difficulty==BOT_HARD?30:60)==0)k|=JUMP;
    return k;
}
