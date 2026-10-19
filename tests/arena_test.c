#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"
static void start(Arena *a){arena_init(a);arena_start_round(a);a->countdown=0;a->p[0].x=154;a->p[1].x=270;a->p[0].y=a->p[1].y=285;}
static void step(Arena *a,int first,int second,int ticks){uint16_t k[2]={(uint16_t)first,(uint16_t)second};while(ticks--)arena_step(a,k);}
static void close_fighters(Arena *a){a->p[1].x=a->p[0].x+20;}
int main(void){
    Arena a,b;arena_init(&a);b=a;step(&a,LEFT,0,10);assert(!memcmp(&a,&b,sizeof a));
    arena_start_round(&a);step(&a,ATTACK|RIGHT,0,90);assert(a.countdown==0&&a.p[0].x==80&&a.p[0].attack_seq==0);step(&a,RIGHT,0,1);assert(a.p[0].x>80);
    start(&a);step(&a,LEFT,0,1000);assert(a.p[0].x==ARENA_MIN_X);
    start(&a);step(&a,JUMP,0,1);assert(a.p[0].z>0);step(&a,ATTACK,0,1);assert(a.p[0].attack_kind==HIT_AIR);step(&a,0,0,40);assert(a.p[0].z==0);
    start(&a);close_fighters(&a);step(&a,ATTACK,0,1);assert(a.p[1].hp==ARENA_HEALTH);step(&a,0,0,1);assert(a.p[1].hp==ARENA_HEALTH);step(&a,0,0,1);assert(a.p[1].hp==ARENA_HEALTH-15);step(&a,0,0,3);assert(a.p[1].hp==ARENA_HEALTH-15);
    start(&a);close_fighters(&a);step(&a,ATTACK,BLOCK,4);assert(a.p[1].hp==ARENA_HEALTH&&a.p[1].block_seq==1);
    start(&a);close_fighters(&a);a.p[1].facing=1;step(&a,ATTACK,BLOCK,4);assert(a.p[1].hp==ARENA_HEALTH-15);
    start(&a);for(int i=0;i<4;i++){step(&a,ATTACK,0,1);if(i<3)step(&a,0,0,i==2?13:12);}assert(a.p[0].combo==4&&a.p[0].attack_kind==HIT_FINISH);
    start(&a);step(&a,ATTACK,0,1);step(&a,0,0,6);step(&a,HEAVY,0,7);assert(a.p[0].attack_kind==HIT_LAUNCH);close_fighters(&a);step(&a,0,0,3);assert(a.p[1].z>0&&a.p[1].hp==ARENA_HEALTH-24);
    start(&a);step(&a,MAGIC|ATTACK,0,1);assert(a.p[0].mana==ARENA_MANA-16);step(&a,0,0,30);assert(a.p[0].mana>ARENA_MANA-16&&a.p[1].freeze>0&&a.p[1].hp==ARENA_HEALTH-18);
    start(&a);step(&a,0,MAGIC|ATTACK,1);assert(a.p[1].mana==ARENA_MANA-16);step(&a,0,0,30);assert(a.p[0].burn>0&&a.p[0].hp<=ARENA_HEALTH-18);
    start(&a);close_fighters(&a);step(&a,MAGIC|HEAVY,BLOCK,1);assert(a.p[0].mana==ARENA_MANA-25);step(&a,0,BLOCK,6);assert(a.p[1].hp==ARENA_HEALTH-24&&a.p[1].freeze==30);
    start(&a);a.p[0].mana=0;step(&a,MAGIC|ATTACK,0,1);assert(a.p[0].magic_seq==0);
    start(&a);close_fighters(&a);a.p[0].hp=a.p[1].hp=15;step(&a,ATTACK,ATTACK,3);assert(a.winner==3&&a.p[0].hp==0&&a.p[1].hp==0);
    start(&a);close_fighters(&a);a.p[1].hp=15;step(&a,ATTACK,0,4);assert(a.winner==1&&a.wins[0]==1);b=a;step(&a,ATTACK,0,10);assert(a.winner==b.winner&&a.wins[0]==b.wins[0]&&a.p[0].x==b.p[0].x&&a.p[1].hp==0);arena_start_round(&a);assert(a.wins[0]==1&&a.round==2&&!a.winner&&a.countdown==90&&a.p[1].hp==ARENA_HEALTH);
    start(&a);start(&b);unsigned rng=12345;
    for(int i=0;i<60000;i++){
        rng=rng*1664525u+1013904223u;uint16_t k[2]={(rng>>12)&511,(rng>>23)&511};
        arena_step(&a,k);arena_step(&b,k);assert(!memcmp(&a,&b,sizeof a));
        for(int p=0;p<2;p++){assert(a.p[p].x>=ARENA_MIN_X&&a.p[p].x<=ARENA_MAX_X);assert(a.p[p].y>=ARENA_MIN_Y&&a.p[p].y<=ARENA_MAX_Y);assert(a.p[p].hp>=0&&a.p[p].hp<=ARENA_HEALTH);assert(a.p[p].z>=0&&a.p[p].z<=80);assert(a.p[p].pose>=0&&a.p[p].pose<POSE_COUNT);}
        if(a.winner){arena_start_round(&a);arena_start_round(&b);}
    }
    /* Lobby starts with no selected/visible fighter and requires both confirmations. */
    arena_lobby_init(&a);assert(!arena_players_ready(&a)&&!a.p[0].ready&&!a.p[1].ready);
    arena_start_round(&a);assert(!a.running&&!a.round);
    assert(arena_select_knight(&a,0,KNIGHT_RED,0)&&!a.p[0].ready&&a.p[0].knight_id==KNIGHT_BLUE);
    assert(arena_select_knight(&a,0,KNIGHT_RED,1)&&a.p[0].knight_id==KNIGHT_RED);
    arena_start_round(&a);assert(!a.running);
    assert(!arena_select_knight(&a,2,KNIGHT_GREEN,1));
    assert(!arena_select_knight(&a,1,KNIGHT_COUNT,1));
    assert(!arena_select_knight(&a,1,KNIGHT_GREEN,2));
    assert(arena_select_knight(&a,1,KNIGHT_GREEN,1)&&arena_players_ready(&a));
    arena_start_round(&a);assert(a.running&&a.p[0].knight_id==KNIGHT_RED&&a.p[1].knight_id==KNIGHT_GREEN);
    assert(!arena_select_knight(&a,0,KNIGHT_BLUE,1)); /* No mid-round identity changes. */
    a.p[0].bow_equipped=1;arena_start_round(&a);
    assert(a.p[0].knight_id==KNIGHT_RED&&a.p[1].knight_id==KNIGHT_GREEN&&!a.p[0].bow_equipped);
    arena_lobby_init(&a);assert(!a.p[0].ready&&!a.p[1].ready);
    /* Original elemental status belongs to the chosen knight, never player slot. */
    for(int slot=0;slot<2;slot++)for(int knight=0;knight<KNIGHT_COUNT;knight++){
        start(&a);close_fighters(&a);a.p[slot].knight_id=knight;
        step(&a,slot?0:MAGIC|ATTACK,slot?MAGIC|ATTACK:0,6);
        const Fighter *target=&a.p[1-slot];
        assert(target->hp==ARENA_HEALTH-18);
        assert((target->freeze>0)==(knight==KNIGHT_BLUE));
        assert((target->burn>0)==(knight==KNIGHT_ORANGE));
    }
    puts("PASS: countdown, movement, air attacks, active hit frame, directional zero-damage block, light chain, launcher, magic costs/regen/statuses, rematch, 60000 deterministic ticks");
}
