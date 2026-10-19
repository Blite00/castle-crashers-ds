#include "bot.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    Arena a;arena_init(&a);stage_set_map(0);
    a.p[1].x=200;a.p[0].x=230;a.p[0].y=a.p[1].y=180;
    a.p[1].facing=1;a.p[1].cooldown=a.p[0].cooldown=0;
    int attacks[3]={0},blocks[3]={0};
    for(int d=0;d<BOT_COUNT;d++)for(int t=0;t<360;t++){
        a.tick=t;attacks[d]+=!!(bot_input(&a,d)&ATTACK_EDGE);
        a.p[0].cooldown=10;blocks[d]+=!!(bot_input(&a,d)&BLOCK);a.p[0].cooldown=0;
    }
    assert(attacks[0]<attacks[1]&&attacks[1]<attacks[2]);
    assert(blocks[0]==0&&blocks[1]>0&&blocks[2]>blocks[1]);
    a.tick=0;a.p[1].combo=1;
    assert(bot_input(&a,BOT_EASY)&ATTACK_EDGE);
    assert(bot_input(&a,BOT_MEDIUM)&HEAVY_EDGE);
    assert(bot_input(&a,BOT_HARD)&HEAVY_EDGE);
    assert(bot_input(&a,-1)==bot_input(&a,BOT_MEDIUM));
    for(int d=0;d<BOT_COUNT;d++){
        arena_init(&a);arena_start_round(&a);
        for(int t=0;t<10000;t++){
            uint16_t keys[2]={t%12==0?ATTACK|ATTACK_EDGE:RIGHT,bot_input(&a,d)};
            arena_step(&a,keys);
            assert(a.p[1].hp>=0&&a.p[1].hp<=ARENA_HEALTH);
            if(a.winner)arena_start_round(&a);
        }
    }
    puts("PASS: difficulty reaction/defense tiers, combo limits and 30,000 practice ticks");
}

