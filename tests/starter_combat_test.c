#include "arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {Arena a;Fighter enemies[6],ally;uint8_t targets[9],levels[2],hits[2];} Fixture;
static void setup(Fixture *f,unsigned level){
    memset(f,0,sizeof *f);arena_init(&f->a);f->a.running=1;f->a.p[1].ready=0;
    f->a.p[0].x=100;f->a.p[0].y=285;f->levels[0]=f->levels[1]=level;f->targets[0]=2;
    f->enemies[0]=(Fighter){.x=350,.y=285,.hp=1000,.ready=1,.facing=-1};
}
static void step(Fixture *f,unsigned keys){uint16_t inputs[9]={(uint16_t)keys};arena_group_step(&f->a,f->enemies,&f->ally,inputs,f->targets,f->levels,f->hits);}
static void rest(Fixture *f){for(int t=0;f->a.p[0].cooldown&&t<60;t++)step(f,0);assert(!f->a.p[0].cooldown);}
static void chains(unsigned level){
    Fixture f;setup(&f,level);const int light[]={POSE_ATTACK,POSE_LIGHT2,POSE_LIGHT3,POSE_FINISHER,POSE_ATTACK};
    for(unsigned i=0;i<5;i++){step(&f,ATTACK|ATTACK_EDGE);assert(f.a.p[0].pose==light[i]);assert(f.a.p[0].attack_seq==i+1);rest(&f);}
    setup(&f,level);const int heavy[]={POSE_HEAVY,POSE_HEAVY2,POSE_HEAVY3};
    for(unsigned i=0;i<3;i++){step(&f,HEAVY|HEAVY_EDGE);assert(f.a.p[0].pose==heavy[i]);assert(f.a.p[0].attack_seq==i+1);rest(&f);}
    /* Mixed light-heavy uppercut is gated; ordinary heavy chain is not. */
    setup(&f,level);step(&f,ATTACK|ATTACK_EDGE);rest(&f);step(&f,HEAVY|HEAVY_EDGE);
    assert(f.a.p[0].pose==(level>=8?POSE_UPPERCUT:POSE_HEAVY));
    setup(&f,level);f.a.p[0].z=60;f.a.p[0].float_timer=11;
    step(&f,ATTACK|ATTACK_EDGE);for(int i=0;i<4;i++)step(&f,0);
    step(&f,HEAVY|HEAVY_EDGE);assert(f.a.p[0].pose==(level>=8?POSE_UPPERCUT:POSE_AIRHEAVY));
    setup(&f,level);step(&f,JUMP|JUMP_EDGE);step(&f,0);step(&f,JUMP|JUMP_EDGE);
    for(int i=0;i<3;i++)step(&f,0);
    assert((f.a.p[0].pose==POSE_AIRSPIN)==(level>=2));
}
static void stomps(void){
    Fixture f;
    for(int side=-1;side<=1;side+=2){
        setup(&f,1);Fighter *p=&f.a.p[0],*q=&f.enemies[0];p->x=180;q->x=180+side*20;q->y=294;
        q->pose=POSE_KNOCKDOWN;q->cooldown=60;q->invulnerable=30;
        step(&f,HEAVY|HEAVY_EDGE);assert(p->pose==POSE_STOMP);
        for(int frame=1;frame<8;frame++){step(&f,0);assert(q->hp==1000);}
        step(&f,0);assert(p->animation==8&&q->hp<1000&&f.hits[0]==1);int hp=q->hp;
        for(int t=0;t<8;t++){step(&f,0);}assert(q->hp==hp);
    }
    const int cases[4][3]={{30,0,1000},{20,10,1000},{20,0,0},{50,0,1000}};
    for(unsigned i=0;i<4;i++){
        setup(&f,1);f.enemies[0].x=f.a.p[0].x+cases[i][0];f.enemies[0].y=285+cases[i][1];f.enemies[0].hp=cases[i][2];
        f.enemies[0].pose=POSE_KNOCKDOWN;f.enemies[0].cooldown=60;step(&f,HEAVY|HEAVY_EDGE);assert(f.a.p[0].pose!=POSE_STOMP);
    }
    /* Moving out before the authored contact event makes the stomp miss. */
    setup(&f,1);f.enemies[0].x=120;f.enemies[0].pose=POSE_KNOCKDOWN;f.enemies[0].cooldown=60;
    step(&f,HEAVY|HEAVY_EDGE);assert(f.a.p[0].pose==POSE_STOMP);f.enemies[0].x=200;
    for(int t=0;t<12;t++){step(&f,0);}assert(f.enemies[0].hp==1000&&!f.hits[0]);
}
static void stomp_target_selection(void){
    Fixture f;setup(&f,1);
    f.enemies[0].x=f.a.p[0].x+20; /* Selected standing enemy. */
    f.enemies[1]=f.enemies[0];f.enemies[1].x=f.a.p[0].x-25;
    f.enemies[1].pose=POSE_KNOCKDOWN;f.enemies[1].cooldown=21;f.enemies[1].invulnerable=30;
    step(&f,HEAVY);assert(f.a.p[0].pose==POSE_STOMP);
}
static void juggling(void){
    Fixture f;
    for(int airborne=0;airborne<=1;airborne++){
        setup(&f,1);Fighter *p=&f.a.p[0],*q=&f.enemies[0];q->x=145;q->z=airborne?20:0;
        p->z=35;p->vz=-2;p->pose=POSE_AIRATTACK;p->attack_kind=HIT_AIR;p->animation=1;p->cooldown=8;
        step(&f,0);assert(q->hp<1000&&p->vz==-3&&p->float_timer==7&&!q->float_timer);
        assert(q->vz==(airborne?4:9));if(airborne)assert(q->knockback==6);
        int height=p->z;for(int t=0;t<7;t++)step(&f,0);assert(p->z==height&&p->vz==-3);
    }
    setup(&f,1);f.a.p[0].z=35;f.a.p[0].vz=-2;f.a.p[0].pose=POSE_AIRATTACK;f.a.p[0].attack_kind=HIT_AIR;f.a.p[0].animation=1;f.a.p[0].cooldown=8;
    step(&f,0);assert(!f.hits[0]&&!f.a.p[0].float_timer&&f.a.p[0].vz==-3);
    /* Target enters one tick after the callback: no three-tick late catch. */
    f.enemies[0].x=145;step(&f,0);assert(f.enemies[0].hp==1000);
    setup(&f,8);Fixture copy=f;unsigned rng=123;
    for(int t=0;t<1500;t++){
        rng=rng*1664525u+1013904223u;unsigned keys=(rng>>16)&INPUT_MASK;
        step(&f,keys);step(&copy,keys);assert(!memcmp(&f,&copy,sizeof f));
    }
}
int main(void){chains(1);chains(2);chains(7);chains(8);stomps();stomp_target_selection();juggling();puts("PASS: starter light/heavy chains, contextual stomp, level gates, source aerial contact momentum and deterministic group combat");}
