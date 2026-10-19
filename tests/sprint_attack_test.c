#include "arena.h"
#include "protocol.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void start(Arena*a){arena_lobby_init(a);a->p[0].ready=a->p[1].ready=1;arena_start_round(a);a->countdown=0;a->p[0].x=70;a->p[0].y=290;a->p[1].x=350;a->p[1].y=200;}
static void step(Arena*a,unsigned first,unsigned second,unsigned count){uint16_t k[2]={(uint16_t)first,(uint16_t)second};while(count--)arena_step(a,k);}
static void sprint(Arena*a){step(a,RIGHT,0,35);assert(arena_fighter_running(&a->p[0]));}
static void valid(Arena*a){StatePacket p={.magic=PROTOCOL_MAGIC,.version=PROTOCOL_VERSION,.arena=*a};assert(protocol_state_valid(&p));}
static void campaign_gate(unsigned level,unsigned button,int expected){
 Arena a;start(&a);Fighter e[6]={{0}},ally={0};uint8_t targets[9]={2,2},levels[2]={(uint8_t)level,1},hits[2];uint16_t keys[9]={RIGHT};
 for(unsigned i=0;i<35;i++)arena_group_step(&a,e,&ally,keys,targets,levels,hits);
 assert(arena_fighter_running(&a.p[0]));keys[0]=RIGHT|button|(button==ATTACK?ATTACK_EDGE:HEAVY_EDGE);arena_group_step(&a,e,&ally,keys,targets,levels,hits);assert(a.p[0].pose==expected);
}
int main(void){
 Arena a;start(&a);sprint(&a);int x=a.p[0].x;step(&a,RIGHT|ATTACK|ATTACK_EDGE,0,1);assert(a.p[0].pose==POSE_DASHPUNCH&&a.p[0].attack_kind==HIT_LIGHT&&!a.p[0].invulnerable);valid(&a);
 int previous=a.p[0].x;step(&a,RIGHT,0,1);assert(a.p[0].x-previous==7);assert(a.p[0].x>x);valid(&a);step(&a,RIGHT,0,12);assert(a.p[0].pose==POSE_WALK&&!arena_fighter_running(&a.p[0]));
 start(&a);sprint(&a);a.p[1].x=a.p[0].x+45;a.p[1].y=a.p[0].y;step(&a,RIGHT|ATTACK|ATTACK_EDGE,0,1);int hp=a.p[1].hp;for(int i=0;i<10;i++){step(&a,0,0,1);valid(&a);}assert(a.p[1].hp==hp-15&&a.p[1].hit_seq==1);
 start(&a);sprint(&a);a.p[1].x=a.p[0].x+45;a.p[1].y=a.p[0].y;a.p[1].facing=-1;step(&a,RIGHT|ATTACK|ATTACK_EDGE,BLOCK,1);step(&a,0,BLOCK,10);assert(a.p[1].hp==ARENA_HEALTH&&a.p[1].block_seq==1);
 start(&a);sprint(&a);a.p[1].x=a.p[0].x+45;a.p[1].y=a.p[0].y;a.p[1].facing=1;step(&a,RIGHT|ATTACK|ATTACK_EDGE,BLOCK,1);step(&a,0,BLOCK,10);assert(a.p[1].hp==ARENA_HEALTH-15);
 start(&a);sprint(&a);a.p[1].x=a.p[0].x+45;a.p[1].y=a.p[0].y-25;step(&a,RIGHT|ATTACK|ATTACK_EDGE,0,1);step(&a,0,0,10);assert(a.p[1].hp==ARENA_HEALTH);
 start(&a);sprint(&a);a.p[1].x=a.p[0].x+50;a.p[1].y=a.p[0].y;step(&a,RIGHT|HEAVY|HEAVY_EDGE,0,1);assert(a.p[0].pose==POSE_DASHSLAM&&a.p[0].z==1&&a.p[0].float_timer==10);previous=a.p[0].x;step(&a,0,0,1);assert(a.p[0].x-previous==9&&a.p[1].hp==ARENA_HEALTH-15&&a.p[1].vz==13);valid(&a);step(&a,0,0,15);assert(a.p[0].z==0&&a.p[1].hit_seq==1);
 /* Existing spell priority and recovery roll remain distinct. */
 start(&a);sprint(&a);step(&a,RIGHT|MAGIC|ATTACK|ATTACK_EDGE,0,1);assert(a.p[0].pose==POSE_CAST);
 start(&a);step(&a,RIGHT|ATTACK|ATTACK_EDGE,0,1);assert(a.p[0].pose==POSE_ATTACK);
 start(&a);sprint(&a);step(&a,DOWN,0,1);assert(arena_fighter_running(&a.p[0]));step(&a,DOWN|ATTACK|ATTACK_EDGE,0,1);assert(a.p[0].pose==POSE_DASHPUNCH);
 campaign_gate(1,ATTACK,POSE_ATTACK);campaign_gate(3,HEAVY,POSE_HEAVY);campaign_gate(4,ATTACK,POSE_DASHPUNCH);campaign_gate(4,HEAVY,POSE_DASHSLAM);
 /* Swept movement remains inside the stage at both edges. */
 for(int side=-1;side<=1;side+=2){start(&a);a.p[0].x=side>0?365:55;a.p[0].facing=side;a.p[0].pose=POSE_WALK;a.p[0].animation=35;step(&a,HEAVY|HEAVY_EDGE,0,1);for(int t=0;t<20;t++){step(&a,0,0,1);valid(&a);}}
 puts("Sprint attacks: original dash poses, momentum, level4 gate, active hit windows, directional shield, depth avoidance, launch, wall collision and wire validation PASS");return 0;
}
