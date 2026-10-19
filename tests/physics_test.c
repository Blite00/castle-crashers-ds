#include "arena.h"
#include "stage.h"
#include "protocol.h"
#include "campaign.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void start(Arena *a){arena_init(a);arena_start_round(a);a->countdown=0;a->p[0].x=154;a->p[1].x=270;a->p[0].y=a->p[1].y=285;}
static void step(Arena *a,int first,int second){uint16_t k[2]={(uint16_t)first,(uint16_t)second};arena_step(a,k);}
/* Recenter between samples so small arena boundaries cannot mask acceleration. */
static int walk_sample(Arena *a,int key){
    a->p[0].x=154;a->p[0].y=285;a->p[1].x=350;a->p[1].y=305;
    step(a,key,0);return a->p[0].x-154;
}
static void running_tests(void){
    _Static_assert(sizeof(InputPacket)==64,"running must not grow input packets");
    _Static_assert(sizeof(CampaignPacket)==276,"running must not grow campaign snapshots");
    Arena a;start(&a);
    for(int i=0;i<ARENA_RUN_TICKS;i++)assert(walk_sample(&a,RIGHT)<=3);
    assert(walk_sample(&a,RIGHT)>=3);
    assert(arena_fighter_running(&a.p[0]));
    int distance=0;for(int i=0;i<360;i++)distance+=walk_sample(&a,RIGHT);
    assert(distance==1210); /* 2.75 DS units times source dash-to-run ratio22/18. */
    assert(walk_sample(&a,LEFT)<=-3&&arena_fighter_running(&a.p[0]));
    walk_sample(&a,LEFT|UP);assert(arena_fighter_running(&a.p[0]));
    walk_sample(&a,UP);assert(arena_fighter_running(&a.p[0]));
    walk_sample(&a,DOWN);assert(arena_fighter_running(&a.p[0]));
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    walk_sample(&a,0);assert(walk_sample(&a,RIGHT)<=3);
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    assert(walk_sample(&a,RIGHT|BLOCK)==1);assert(walk_sample(&a,RIGHT)<=3);
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    walk_sample(&a,RIGHT|ATTACK);assert(a.p[0].pose==POSE_DASHPUNCH);
    while(a.p[0].cooldown)walk_sample(&a,RIGHT);
    assert(walk_sample(&a,RIGHT)<=3);
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    walk_sample(&a,RIGHT|JUMP);assert(a.p[0].z&&a.p[0].pose==POSE_JUMP);
    while(a.p[0].z||a.p[0].cooldown)walk_sample(&a,RIGHT);
    assert(walk_sample(&a,RIGHT)<=3);
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    a.p[0].x=ARENA_MAX_X;step(&a,RIGHT,0);assert(a.p[0].animation==0);
    assert(walk_sample(&a,RIGHT)<=3);
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    a.p[0].x=ARENA_MAX_X;a.p[0].y=285;step(&a,RIGHT|UP,0);
    assert(a.p[0].y<285&&arena_fighter_running(&a.p[0]));
    int old_animation=a.p[0].animation;
    for(int i=0;i<8;i++){walk_sample(&a,RIGHT|MAGIC);assert(arena_fighter_running(&a.p[0])&&a.p[0].animation>old_animation);old_animation=a.p[0].animation;}
    for(int i=0;i<40;i++)walk_sample(&a,RIGHT);
    a.p[0].hitstun=3;walk_sample(&a,RIGHT);assert(!arena_fighter_running(&a.p[0]));
    while(a.p[0].hitstun)walk_sample(&a,RIGHT);
    assert(walk_sample(&a,RIGHT)<=3);
    /* Both cooperative heroes accelerate independently, enemy/ally pacing does not. */
    start(&a);Fighter enemies[6]={{0}},ally={0};
    uint16_t inputs[9]={RIGHT,RIGHT,RIGHT,0,0,0,0,0,RIGHT};
    uint8_t targets[9]={2,2,0,0,0,0,0,0,2},levels[2]={1,1},hits[2];
    ArenaCampaignAttributes attributes[2]={{1,1,1,1,0},{1,1,1,25,0}};
    enemies[0]=a.p[0];ally=a.p[0];
    for(int i=0;i<80;i++){
        a.p[0].x=50;a.p[0].y=285;a.p[1].x=130;a.p[1].y=305;
        enemies[0].x=240;enemies[0].y=285;ally.x=320;ally.y=305;
        arena_group_step_attributes(&a,enemies,&ally,inputs,targets,levels,attributes,hits);
        assert(enemies[0].x-240<=3&&ally.x-320<=3);
        if(i<30)assert(a.p[0].x-50<=3&&a.p[1].x-130<=4);
        else assert(a.p[0].x-50>=2&&a.p[1].x-130>=4&&arena_fighter_running(&a.p[0])&&arena_fighter_running(&a.p[1]));
    }
}
int main(void){
    running_tests();
    Arena a;start(&a);
    int apex=0,land_tick=0;
    for(int tick=1;tick<=30;tick++){
        step(&a,tick==1?JUMP:0,0);
        if(a.p[0].z>apex)apex=a.p[0].z;
        if(!a.p[0].z&&!land_tick){land_tick=tick;assert(a.p[0].pose==POSE_LAND);}
    }
    assert(apex==55&&land_tick==21&&a.p[0].pose==POSE_IDLE);
    start(&a);step(&a,JUMP,0);
    for(int i=0;i<50;i++)step(&a,ATTACK,0);
    assert(a.p[0].z==0); /* Whiffed air attacks cannot sustain flight. */
    start(&a);a.p[0].cooldown=1;a.p[0].queued=ATTACK;
    step(&a,BLOCK,0);assert(a.p[0].guard&&a.p[0].attack_seq==0);
    start(&a);a.p[0].x=200;a.p[1].x=216;
    for(int i=0;i<30;i++)step(&a,RIGHT,LEFT);
    assert(a.p[1].x-a.p[0].x>=16);
    start(&a);a.p[0].x=a.p[1].x=ARENA_MIN_X;
    step(&a,LEFT,LEFT);assert((a.p[0].x==ARENA_MIN_X||a.p[1].x==ARENA_MIN_X)&&abs(a.p[1].x-a.p[0].x)>=16);
    start(&a);a.p[0].x=a.p[1].x=ARENA_MAX_X;
    step(&a,RIGHT,RIGHT);assert((a.p[0].x==ARENA_MAX_X||a.p[1].x==ARENA_MAX_X)&&abs(a.p[1].x-a.p[0].x)>=16);
    start(&a);a.p[0].x=200;a.p[1].x=204;a.p[1].y+=20;
    step(&a,RIGHT,LEFT);assert(a.p[0].x>a.p[1].x);
    start(&a);a.p[0].x=200;a.p[1].x=204;a.p[0].z=40;
    step(&a,RIGHT,LEFT);assert(a.p[0].x>a.p[1].x);
    for(int facing=-1;facing<=1;facing+=2)for(int distance=0;distance<=21;distance++){
        start(&a);a.p[0].x=200;a.p[0].facing=facing;a.p[1].x=200+facing*distance;
        for(int tick=0;tick<7;tick++)step(&a,tick==0?MAGIC|ATTACK:0,0);
        if(a.p[1].hp!=ARENA_HEALTH-18)printf("magic facing%d distance%d hp%d positions%d/%d\n",facing,distance,a.p[1].hp,a.p[0].x,a.p[1].x);
        assert(a.p[1].hp==ARENA_HEALTH-18);
    }
    for(int height=0;height<2;height++){
        start(&a);a.p[1].x=a.p[0].x+20;
        if(height)a.p[1].z=65;else a.p[1].y+=20;
        for(int i=0;i<3;i++)step(&a,i==0?ATTACK:0,0);
        assert(a.p[1].hp==ARENA_HEALTH);
    }
    start(&a);a.p[0].z=a.p[1].z=40;a.p[1].x=a.p[0].x+20;
    step(&a,ATTACK,0);step(&a,0,0);a.p[1].invulnerable=4;
    step(&a,0,0);assert(a.p[1].hp==ARENA_HEALTH&&a.p[0].vz<0);
    start(&a);a.p[0].hp=1;a.p[0].burn=30;
    step(&a,ATTACK,0);assert(!a.p[0].hp&&!a.p[0].attack_seq&&a.p[0].pose==POSE_DEAD);
    for(int i=0;i<20;i++)step(&a,RIGHT|ATTACK,0);
    assert(a.p[0].pose==POSE_DEAD&&a.p[0].animation==1&&a.wins[1]==1);
    start(&a);a.p[0].z=a.p[1].z=40;a.p[1].x=a.p[0].x+20;
    step(&a,ATTACK,ATTACK);step(&a,0,0);step(&a,0,0);
    assert(a.p[0].hp==a.p[1].hp&&a.p[0].hp<ARENA_HEALTH);
    assert(a.p[0].vz==a.p[1].vz&&a.p[0].z==a.p[1].z);
    start(&a);a.p[1].pose=POSE_AIRHURT;a.p[1].z=40;a.p[1].freeze=2;
    step(&a,0,0);assert(a.p[1].pose==POSE_AIRHURT);
    step(&a,0,0);step(&a,0,ATTACK);assert(!a.p[1].attack_seq);
    for(int i=0;i<12&&a.p[1].z;i++)step(&a,0,0);
    assert(!a.p[1].z&&a.p[1].pose==POSE_KNOCKDOWN&&a.p[1].cooldown==21);
    for(int i=0;i<21;i++)step(&a,0,0);
    assert(a.p[1].pose==POSE_IDLE);
    start(&a);a.winner=1;a.p[1].hp=0;a.p[0].z=40;
    a.p[0].pose=POSE_AIRATTACK;a.p[0].cooldown=9;
    for(int i=0;i<30;i++)step(&a,0,0);
    assert(a.p[0].pose==POSE_IDLE&&!a.p[0].z);
    /* A visible swing can connect on its second active tick, once only. */
    start(&a);a.p[1].x=a.p[0].x+72;
    step(&a,ATTACK,0);step(&a,0,0);step(&a,0,0);
    assert(a.p[1].hp==ARENA_HEALTH&&!a.p[0].hit_done);
    step(&a,0,LEFT);assert(a.p[1].hp==ARENA_HEALTH-15&&a.p[0].hit_done);
    step(&a,0,0);step(&a,0,0);assert(a.p[1].hit_seq==1);
    /* Entering range after all three active ticks is a real miss. */
    start(&a);a.p[1].x=a.p[0].x+80;
    step(&a,ATTACK,0);for(int i=0;i<4;i++)step(&a,0,0);
    a.p[1].x=a.p[0].x+20;step(&a,0,0);
    assert(a.p[1].hp==ARENA_HEALTH&&a.p[0].hit_done);
    start(&a);a.p[1].x=a.p[0].x+20;
    step(&a,ATTACK,BLOCK);for(int i=0;i<5;i++)step(&a,0,BLOCK);
    assert(a.p[1].hp==ARENA_HEALTH&&a.p[1].block_seq==1);
    /* Buffered taps survive short recovery, but cannot fire long afterwards. */
    start(&a);a.p[0].cooldown=6;step(&a,ATTACK,0);
    for(int i=0;i<5;i++)step(&a,0,0);
    assert(a.p[0].attack_seq==1&&a.p[0].attack_kind==HIT_LIGHT);
    start(&a);a.p[0].cooldown=12;step(&a,ATTACK,0);
    for(int i=0;i<12;i++)step(&a,0,0);
    assert(!a.p[0].attack_seq&&!a.p[0].queued&&!a.p[0].queue_timer);
    /* Close heavy throws grounded shields, with the same outcome in both slots. */
    for(int owner=0;owner<2;owner++){
        start(&a);a.p[1].x=a.p[0].x+20;
        step(&a,owner?BLOCK:HEAVY,owner?HEAVY:BLOCK);
        assert(a.p[owner].attack_kind==HIT_THROW&&a.p[owner].pose==POSE_THROW);
        for(int i=0;i<5;i++)step(&a,owner?BLOCK:0,owner?0:BLOCK);
        assert(a.p[1-owner].hp==ARENA_HEALTH);
        step(&a,owner?BLOCK:0,owner?0:BLOCK);
        assert(a.p[1-owner].hp==ARENA_HEALTH-18&&a.p[1-owner].vz==10);
        assert(a.p[1-owner].knockback==(owner?-6:6));
        assert(a.p[1-owner].pose==POSE_AIRHURT&&!a.p[1-owner].block_seq);
        for(int i=0;i<40;i++)step(&a,0,0);
        assert(a.p[1-owner].hit_seq==1);
    }
    start(&a);a.p[1].x=a.p[0].x+30;step(&a,HEAVY,0);
    assert(a.p[0].attack_kind==HIT_HEAVY);
    start(&a);a.p[1].x=a.p[0].x+20;a.p[0].combo=1;a.p[0].combo_timer=20;
    step(&a,HEAVY,0);assert(a.p[0].attack_kind==HIT_LAUNCH);
    start(&a);a.p[1].x=a.p[0].x+20;step(&a,HEAVY,0);
    step(&a,0,JUMP);for(int i=0;i<8;i++)step(&a,0,0);
    assert(a.p[1].hp==ARENA_HEALTH); /* Jump escapes the uncommitted grab. */
    start(&a);a.p[1].x=a.p[0].x+20;step(&a,HEAVY,HEAVY);
    for(int i=0;i<6;i++)step(&a,0,0);
    assert(a.p[0].hp==ARENA_HEALTH-18&&a.p[1].hp==a.p[0].hp);
    assert(a.p[0].vz==10&&a.p[1].vz==10&&a.p[0].z==a.p[1].z);
    start(&a);a.p[1].x=a.p[0].x+20;step(&a,HEAVY,ATTACK);
    for(int i=0;i<8;i++)step(&a,0,0);
    assert(a.p[0].hp==ARENA_HEALTH-15&&a.p[1].hp==ARENA_HEALTH);
    /* Recovery roll escapes prone state, remains collision-safe and ends. */
    for(int owner=0;owner<2;owner++){
        start(&a);Fighter *p=&a.p[owner];p->pose=POSE_KNOCKDOWN;p->cooldown=18;
        int x=p->x;
        step(&a,owner?0:JUMP|LEFT,owner?JUMP|RIGHT:0);
        assert(p->pose==POSE_ROLL&&p->invulnerable==18);
        assert(owner?p->x>x:p->x<x);
        for(int j=0;j<18;j++)step(&a,0,0);
        assert(p->pose==POSE_IDLE&&!p->invulnerable&&!p->cooldown);
    }
    /* Six chained hits force a protected knockdown rather than perpetual stun. */
    start(&a);a.p[1].x=a.p[0].x+20;
    a.p[1].stun_chain=5;a.p[1].stun_timer=20;
    step(&a,ATTACK,0);step(&a,0,0);step(&a,0,0);
    assert(a.p[1].vz==6&&a.p[1].invulnerable==30&&!a.p[1].stun_chain);
    for(int j=0;j<25&&a.p[1].pose!=POSE_KNOCKDOWN;j++)step(&a,0,0);
    assert(a.p[1].pose==POSE_KNOCKDOWN&&a.p[1].invulnerable==30);
    step(&a,0,JUMP|RIGHT);assert(a.p[1].pose==POSE_ROLL);
    /* Source light-to-heavy aerial uppercut; spin belongs to the second jump. */
    start(&a);a.p[0].z=50;a.p[0].combo=1;a.p[0].combo_timer=20;
    step(&a,HEAVY,0);assert(a.p[0].pose==POSE_UPPERCUT&&a.p[0].cooldown==15);
    start(&a);a.p[0].z=20;a.p[0].combo=1;a.p[0].combo_timer=20;
    step(&a,HEAVY,0);assert(a.p[0].pose==POSE_UPPERCUT);
    start(&a);step(&a,JUMP,0);step(&a,ATTACK,0);
    for(int j=0;j<8;j++)step(&a,0,0);
    step(&a,HEAVY,0);
    assert(a.p[0].pose==POSE_UPPERCUT&&a.p[0].z>=38);
    start(&a);a.p[0].combo=2;a.p[0].combo_timer=20;
    step(&a,JUMP,0);for(int j=0;j<5;j++)step(&a,0,0);step(&a,HEAVY,0);
    assert(a.p[0].pose==POSE_AIRHEAVY); /* Ground combo cannot authorize spin. */
    /* A qualified simultaneous heavy trade occasionally locks swords.
       Each side is evaluated before threshold resolution; equal inputs tie. */
    for(int winner=0;winner<3;winner++){
        start(&a);a.p[1].x=a.p[0].x+30;a.clash_contacts=2;
        step(&a,HEAVY,HEAVY);for(int j=0;j<6;j++)step(&a,0,0);
        assert(a.clash_timer==90&&a.p[0].pose==POSE_CLASH&&a.p[1].pose==POSE_CLASH);
        assert(a.p[0].hp==ARENA_HEALTH&&a.p[1].hp==ARENA_HEALTH);
        if(winner<2){
            for(int j=0;j<5;j++)step(&a,winner?0:ATTACK|ATTACK_EDGE,winner?ATTACK|ATTACK_EDGE:0);
            assert(!a.clash_timer&&a.p[1-winner].hp==ARENA_HEALTH-18);
            assert(a.p[winner].hp==ARENA_HEALTH);
        }else{
            for(int j=0;j<90;j++)step(&a,0,0);
            assert(!a.clash_timer&&a.p[0].hp==ARENA_HEALTH&&a.p[1].hp==ARENA_HEALTH);
        }
        assert(a.clash_cooldown==180);
    }
    start(&a);a.clash_timer=90;a.p[0].previous=a.p[1].previous=ATTACK;
    for(int j=0;j<10;j++)step(&a,ATTACK|ATTACK_EDGE,ATTACK|ATTACK_EDGE);
    assert(!a.clash_timer&&a.clash_score[0]==10&&a.clash_score[1]==10);
    assert(a.p[0].hp==ARENA_HEALTH&&a.p[1].hp==ARENA_HEALTH);
    start(&a);a.clash_timer=90;
    for(int j=0;j<10;j++)step(&a,ATTACK,0);
    assert(a.clash_score[0]==1); /* Holding a button is not mashing. */
    /* Holding attack or magic never repeats; timed separate taps chain. */
    start(&a);for(int j=0;j<90;j++)step(&a,ATTACK,0);
    assert(a.p[0].attack_seq==1&&!a.p[0].cooldown);
    start(&a);for(int j=0;j<90;j++)step(&a,MAGIC|ATTACK,0);
    assert(a.p[0].magic_seq==1);
    start(&a);step(&a,HEAVY,0);
    for(int j=0;j<9;j++)step(&a,0,0);
    step(&a,HEAVY,0); /* Six recovery ticks left: valid one-slot buffer. */
    for(int j=0;j<6;j++)step(&a,0,0);
    assert(a.p[0].attack_seq==2&&a.p[0].pose==POSE_HEAVY2);
    for(int j=0;j<30;j++)step(&a,HEAVY,0);
    assert(a.p[0].attack_seq==2); /* Early held press cannot produce a third attack. */
    start(&a);step(&a,HEAVY,0);step(&a,0,0);step(&a,HEAVY,0);
    for(int j=0;j<35;j++)step(&a,HEAVY,0);
    assert(a.p[0].attack_seq==1); /* Early spam cannot reserve a follow-up. */
    start(&a);step(&a,HEAVY,0);for(int j=0;j<35;j++)step(&a,0,0);
    step(&a,HEAVY,0);assert(a.p[0].pose==POSE_HEAVY); /* Combo expires. */
    start(&a);step(&a,HEAVY,0);for(int j=0;j<9;j++)step(&a,0,0);
    step(&a,HEAVY,0);step(&a,ATTACK,0);
    for(int j=0;j<5;j++)step(&a,0,0);
    assert(a.p[0].pose==POSE_HEAVY2); /* First buffered action is not overwritten. */
    start(&a);a.p[1].x=a.p[0].x+25;
    step(&a,HEAVY,0);for(int j=0;j<9;j++)step(&a,0,0);
    step(&a,HEAVY,0);for(int j=0;j<11;j++)step(&a,0,0);
    assert(a.p[1].hit_seq==3&&a.p[1].hp==ARENA_HEALTH-46);
    assert(a.p[1].invulnerable==4); /* A legitimate double-heavy no longer forces escape. */
    /* Guard permits deliberate slow motion in all four directions. */
    const int movement[4]={LEFT,RIGHT,UP,DOWN};
    for(int d=0;d<4;d++){
        start(&a);int x=a.p[0].x,y=a.p[0].y;
        for(int j=0;j<5;j++)step(&a,BLOCK|movement[d],0);
        assert(a.p[0].guard&&a.p[0].pose==POSE_BLOCK);
        assert(a.p[0].x==x+(d==0?-5:d==1?5:0));
        assert(a.p[0].y==y+(d==2?-5:d==3?5:0));
        if(d<2)assert(a.p[0].facing==(d==0?-1:1));
    }
    start(&a);a.p[1].x=a.p[0].x+16;
    for(int j=0;j<60;j++){step(&a,BLOCK|RIGHT,BLOCK|LEFT);assert(a.p[1].x-a.p[0].x>=16);}
    start(&a);
    for(int j=0;j<600;j++){
        step(&a,BLOCK|(j<300?UP:LEFT),0);
        assert(stage_position_open(a.p[0].x,a.p[0].y));
    }
    /* Authored weapon rectangles have distinct reach, mirrored exactly. */
    const int poses[]={POSE_ATTACK,POSE_LIGHT2,POSE_LIGHT3,POSE_FINISHER,POSE_HEAVY,POSE_HEAVY2,POSE_UPPERCUT,POSE_AIRATTACK,POSE_AIRSPIN,POSE_AIRATTACK2,POSE_AIRHEAVY};
    const int reach[]={70,67,86,89,72,77,89,70,70,67,63};
    for(int n=0;n<11;n++)for(int facing=-1;facing<=1;facing+=2){
        Fighter p={.x=210,.y=250,.facing=facing,.pose=poses[n],.attack_kind=HIT_LIGHT};
        Fighter q={.x=210+facing*reach[n],.y=250};
        assert(arena_melee_overlap(&p,&q));q.x+=facing;assert(!arena_melee_overlap(&p,&q));
        q.x=210+facing*30;q.y=265;assert(arena_melee_overlap(&p,&q));
        q.y=266;assert(!arena_melee_overlap(&p,&q));
        q.y=250;q.x=210-facing*20;
        assert(arena_melee_overlap(&p,&q)==(poses[n]==POSE_AIRSPIN));
    }
    /* Height overlap is body-vs-weapon, not equal foot heights. */
    Fighter test_p={.x=180,.y=250,.facing=1,.pose=POSE_AIRATTACK,.attack_kind=HIT_AIR,.z=30};
    Fighter test_q={.x=210,.y=250};
    assert(arena_melee_overlap(&test_p,&test_q)); /* Low aerial slash reaches a standing body. */
    test_p.z=75;assert(!arena_melee_overlap(&test_p,&test_q)); /* High whiff above the head. */
    test_p.z=0;test_p.pose=POSE_ATTACK;test_q.z=60;
    assert(!arena_melee_overlap(&test_p,&test_q));
    test_p.pose=POSE_UPPERCUT;assert(arena_melee_overlap(&test_p,&test_q)); /* Rising blade reaches higher. */
    /* Contact preserves descending momentum; a victim never shares the float.
       A subsequent tap therefore cannot keep both fighters suspended. */
    start(&a);a.p[1].x=a.p[0].x+30;
    a.p[0].z=40;a.p[0].vz=-3;a.p[0].pose=POSE_AIRATTACK;
    a.p[0].attack_kind=HIT_AIR;a.p[0].cooldown=7;a.p[0].animation=1;
    a.p[1].z=25;a.p[1].pose=POSE_AIRHURT;
    step(&a,0,0);
    assert(a.p[1].hit_seq==1&&a.p[0].vz==-4&&a.p[0].float_timer==7);
    assert(a.p[1].vz==4&&!a.p[1].float_timer&&a.p[1].knockback==6);
    int attacker_height=a.p[0].z,victim_height=a.p[1].z;
    step(&a,0,0);
    assert(a.p[0].z==attacker_height&&a.p[1].z>victim_height);
    for(int j=0;j<80;j++)step(&a,j%6==0?ATTACK_EDGE:0,0);
    assert(!a.p[0].z&&!a.p[0].float_timer);
    /* Initial grounded launch is higher than an airborne re-hit. */
    start(&a);a.p[1].x=a.p[0].x+30;a.p[0].z=35;
    a.p[0].pose=POSE_AIRATTACK;a.p[0].attack_kind=HIT_AIR;
    a.p[0].cooldown=7;a.p[0].animation=1;step(&a,0,0);
    assert(a.p[1].hit_seq==1&&a.p[1].vz==9&&!a.p[1].float_timer);
    /* Entering range one tick after the authored contact cannot become a hit. */
    start(&a);a.p[0].z=35;a.p[0].pose=POSE_AIRATTACK;
    a.p[0].attack_kind=HIT_AIR;a.p[0].cooldown=7;a.p[0].animation=1;
    step(&a,0,0);assert(!a.p[1].hit_seq);
    a.p[1].x=a.p[0].x+30;step(&a,0,0);assert(!a.p[1].hit_seq);
    /* Air misses still fall naturally without contact float. */
    start(&a);step(&a,JUMP,0);step(&a,ATTACK,0);
    for(int j=0;j<40;j++)step(&a,(j%9==0)?ATTACK_EDGE:0,0);
    assert(!a.p[0].z&&!a.p[0].float_timer);
    /* Bow releases on source frame11, costs no mana and never hold-repeats. */
    start(&a);a.p[0].bow_equipped=1;step(&a,BOW|MAGIC,0);
    assert(a.p[0].pose==POSE_BOW&&a.p[0].mana==ARENA_MANA&&!a.p[0].magic_seq);
    for(int j=0;j<9;j++)step(&a,BOW,0);
    assert(!a.p[0].arrow_seq);
    step(&a,BOW,0);assert(a.p[0].arrow_seq==1);
    for(int j=0;j<90;j++)step(&a,BOW,0);
    assert(a.p[0].arrow_seq==1);
    /* Midair bow holds briefly, releases, then eventually lands. */
    start(&a);a.p[0].bow_equipped=1;step(&a,JUMP,0);step(&a,BOW,0);
    assert(a.p[0].float_timer==11);
    for(int j=0;j<11;j++)step(&a,0,0);
    assert(a.p[0].arrow_seq==1&&a.p[0].z>0);
    for(int j=0;j<60;j++)step(&a,0,0);
    assert(!a.p[0].z&&!a.p[0].float_timer);
    /* Shield side follows incoming travel for arrows and magic, both slots.
       A shooter can move after firing: projectile travel stays authoritative. */
    for(int kind=0;kind<=SHOT_ARROW;kind++)for(int direction=-1;direction<=1;direction+=2)for(int front=0;front<2;front++){
        start(&a);a.p[0].x=120;a.p[1].x=240;a.p[1].facing=front?-direction:direction;
        a.shots[0]=(Projectile){.x=240-direction*30,.y=285,.z=30,.vx=direction*(kind==SHOT_ARROW?16:5),.life=47,.owner=0,.kind=kind};
        for(int j=0;j<3;j++)step(&a,0,BLOCK);
        assert(front?a.p[1].hp==ARENA_HEALTH:a.p[1].hp==ARENA_HEALTH-(kind==SHOT_ARROW?12:18));
        assert(a.p[1].block_seq==(unsigned)front);
    }
    /* Front/rear sword defense mirrors; spin direction comes from its origin. */
    for(int direction=-1;direction<=1;direction+=2)for(int front=0;front<2;front++){
        start(&a);a.p[0].x=210-direction*30;a.p[1].x=210;
        a.p[0].facing=direction;a.p[1].facing=front?-direction:direction;
        step(&a,ATTACK,BLOCK);step(&a,0,BLOCK);step(&a,0,BLOCK);
        assert(front?a.p[1].hp==ARENA_HEALTH:a.p[1].hp==ARENA_HEALTH-15);
        start(&a);a.p[0].x=210-direction*20;a.p[1].x=210;
        a.p[0].facing=-direction; /* Spin can connect behind attacker. */
        a.p[0].z=20;a.p[0].pose=POSE_AIRSPIN;a.p[0].attack_kind=HIT_SPIN;a.p[0].cooldown=10;a.p[0].animation=1;
        a.p[1].facing=front?-direction:direction;step(&a,0,BLOCK);
        assert(front?a.p[1].hp==ARENA_HEALTH:a.p[1].hp==ARENA_HEALTH-20);
    }
    /* Arrow path sweeps through a close target instead of skipping its body. */
    start(&a);a.p[0].bow_equipped=1;a.p[1].x=a.p[0].x+25;step(&a,BOW,0);
    for(int j=0;j<10;j++)step(&a,0,0);
    assert(a.p[1].hp==ARENA_HEALTH-12&&a.p[0].arrow_seq==1);
    /* Natural aerial light taps alternate the two original sword poses. */
    start(&a);step(&a,JUMP,0);step(&a,ATTACK,0);
    for(int j=0;j<8;j++)step(&a,0,0);
    step(&a,ATTACK,0);assert(a.p[0].pose==POSE_AIRATTACK2);
    for(int j=0;j<5;j++)step(&a,0,0);
    step(&a,ATTACK,0);assert(a.p[0].pose==POSE_AIRATTACK&&a.p[0].attack_seq==3);
    /* Heavy starts slamdown; its authored frame9 marker enables X follow-up. */
    start(&a);step(&a,JUMP,0);for(int j=0;j<9;j++)step(&a,0,0);
    step(&a,HEAVY,0);assert(a.p[0].pose==POSE_AIRHEAVY&&a.p[0].vz==0);
    for(int j=0;j<6;j++)step(&a,0,0);
    step(&a,HEAVY,0);assert(a.p[0].pose==POSE_AIRHEAVY&&a.p[0].attack_seq==1);
    step(&a,0,0);assert(a.p[0].pose==POSE_HEAVY2&&a.p[0].attack_seq==2);
    start(&a);a.p[1].x=a.p[0].x+25;
    step(&a,JUMP,0);for(int j=0;j<9;j++)step(&a,0,0);step(&a,HEAVY,0);
    for(int j=0;j<4;j++)step(&a,0,0);
    assert(a.p[1].hp==ARENA_HEALTH-22&&a.p[1].vz==-16);
    step(&a,0,0);assert(a.p[1].pose==POSE_KNOCKDOWN&&a.p[1].invulnerable==30);
    /* A valid tap near landing survives recovery but doesn't bypass it. */
    start(&a);step(&a,JUMP,0);for(int j=0;j<14;j++)step(&a,0,0);
    step(&a,ATTACK,0);for(int j=0;j<3;j++)step(&a,0,0);
    step(&a,ATTACK,0);step(&a,0,0);
    assert(a.p[0].pose==POSE_LAND&&a.p[0].attack_seq==1);
    for(int j=0;j<5;j++)step(&a,0,0);
    assert(a.p[0].attack_seq==1);
    step(&a,0,0);assert(a.p[0].pose==POSE_ATTACK&&a.p[0].attack_seq==2);
    /* Neutral fire uses source59ticks and tests cadence before decrement. */
    start(&a);a.p[0].burn=ARENA_BURN_TICKS;
    for(int j=0;j<29;j++)step(&a,0,0);
    assert(a.p[0].hp==ARENA_HEALTH&&a.p[0].burn==30);
    step(&a,0,0);assert(a.p[0].hp==ARENA_HEALTH-1);
    for(int j=0;j<29;j++)step(&a,0,0);
    assert(!a.p[0].burn&&a.p[0].hp==ARENA_HEALTH-1);
    /* Y does nothing while unequipped, preserving the original light-heavy launcher. */
    start(&a);step(&a,BOW|MAGIC,0);
    assert(a.p[0].pose==POSE_IDLE&&a.p[0].attack_kind==HIT_NONE&&a.p[0].equip_timer==30);
    assert(a.p[0].mana==ARENA_MANA&&!a.p[0].attack_seq&&!a.p[0].arrow_seq);
    start(&a);step(&a,ATTACK,0);step(&a,BOW,0);
    assert(a.p[0].combo==1&&a.p[0].pose==POSE_ATTACK&&a.p[0].cooldown==12&&a.p[0].equip_timer==30);
    for(int j=0;j<5;j++)step(&a,0,0);
    step(&a,HEAVY,0);for(int j=0;j<6;j++)step(&a,0,0);
    assert(a.p[0].pose==POSE_UPPERCUT); /* No-item hint cannot erase AX combo. */
    start(&a);step(&a,EQUIP,0);
    assert(a.p[0].bow_equipped&&a.p[0].equip_timer==30);
    for(int j=0;j<5;j++)step(&a,EQUIP,0);
    assert(a.p[0].bow_equipped); /* Holding the touch action never retoggles. */
    step(&a,BOW,0);assert(a.p[0].pose==POSE_BOW);
    step(&a,EQUIP,0);assert(!a.p[0].bow_equipped&&a.p[0].pose==POSE_BOW&&a.p[0].cooldown==19);
    for(int j=0;j<9;j++)step(&a,0,0);
    assert(a.p[0].arrow_seq==1); /* Equipment changes never cancel committed shots. */
    for(int j=0;j<21;j++)step(&a,0,0);
    assert(!a.p[0].equip_timer);
    step(&a,BOW,0);assert(a.p[0].pose==POSE_IDLE);
    start(&a);step(&a,EQUIP|EQUIP_EDGE,0);assert(a.p[0].bow_equipped);
    step(&a,EQUIP|EQUIP_EDGE,0);assert(!a.p[0].bow_equipped); /* A second explicit captured tap. */
    step(&a,0,EQUIP);assert(a.p[1].bow_equipped&&!a.p[0].bow_equipped);
    arena_start_round(&a);
    assert(!a.p[0].bow_equipped&&!a.p[1].bow_equipped&&!a.p[1].equip_timer);
    step(&a,EQUIP,0);assert(a.countdown==89&&a.p[0].bow_equipped);
    for(int j=0;j<5;j++)step(&a,EQUIP,0);
    assert(a.p[0].bow_equipped); /* Countdown supports one equipment edge too. */
    start(&a);step(&a,EQUIP,0);step(&a,ATTACK,0);
    assert(a.p[0].pose==POSE_ATTACK); /* Equipped bow does not replace A's sword. */
    start(&a);step(&a,EQUIP,0);step(&a,MAGIC|ATTACK,0);
    assert(a.p[0].pose==POSE_CAST&&a.p[0].mana==ARENA_MANA-16);
    /* Air L+A uses every selected element, in either slot/facing, and blocks only in front. */
    for(int knight=0;knight<KNIGHT_COUNT;knight++)for(int slot=0;slot<2;slot++)for(int direction=-1;direction<=1;direction+=2)for(int front=0;front<2;front++){
        start(&a);Fighter *caster=&a.p[slot],*target=&a.p[1-slot];
        caster->x=210;caster->z=40;caster->facing=direction;caster->knight_id=knight;
        target->x=210+direction*70;target->facing=front?-direction:direction;
        step(&a,slot?BLOCK:MAGIC|ATTACK,slot?MAGIC|ATTACK:BLOCK);
        assert(caster->pose==POSE_CASTDOWN&&caster->float_timer==10&&caster->mana==ARENA_MANA-16);
        for(int j=0;j<14;j++)step(&a,slot?BLOCK:0,slot?0:BLOCK);
        assert(target->hp==ARENA_HEALTH-(front?0:18));
        assert(target->block_seq==(unsigned)front);
        assert((target->freeze>0)==(!front&&knight==KNIGHT_BLUE));
        assert((target->burn>0)==(!front&&knight==KNIGHT_ORANGE));
    }
    for(int direction=-1;direction<=1;direction+=2){
        start(&a);a.p[0].x=210;a.p[0].z=40;a.p[0].facing=direction;a.p[1].y=305;
        step(&a,MAGIC|ATTACK,0);step(&a,0,0);step(&a,0,0);
        assert(!a.shots[0].life);step(&a,0,0);
        assert(a.shots[0].vx==direction*13&&a.shots[0].vz==-9&&a.shots[0].x==210+direction*22);
        assert(a.shots[0].z==49&&a.shots[0].fraction==1);
        step(&a,0,0);assert(a.shots[0].x==210+direction*29&&a.shots[0].z==45&&!a.shots[0].fraction);
        step(&a,0,0);assert(a.shots[0].x==210+direction*35&&a.shots[0].z==40&&a.shots[0].fraction==1);
    }
    start(&a);a.p[1].y=305;step(&a,MAGIC|ATTACK,0);
    for(int j=0;j<5;j++)step(&a,0,0);
    assert(a.p[0].pose==POSE_CAST&&a.shots[0].vx==5&&!a.shots[0].vz); /* Ground cast unchanged. */
    start(&a);a.p[0].z=40;step(&a,MAGIC|ATTACK,0);
    for(int j=0;j<90;j++)step(&a,MAGIC|ATTACK,0);
    assert(a.p[0].magic_seq==1&&!a.p[0].float_timer&&!a.p[0].z);
    /* Clip descent at floor: cannot hit a body beyond the point of impact. */
    start(&a);a.p[1].x=127;
    a.shots[0]=(Projectile){.x=100,.y=285,.z=1,.vx=13,.life=47,.owner=0,.kind=SHOT_ICE,.vz=-9};
    step(&a,0,0);assert(!a.shots[0].life&&a.p[1].hp==ARENA_HEALTH&&a.shots[0].x==101);
    /* A rising arrow still collides with the below-rock-height part of its path. */
    int rock_edge=80;while(rock_edge<320&&!stage_projectile_blocked(rock_edge,rock_edge+1,230))rock_edge++;
    assert(rock_edge<320);start(&a);
    a.shots[0]=(Projectile){.x=rock_edge-2,.y=230,.z=55,.vx=16,.life=47,.owner=0,.kind=SHOT_ARROW,.vz=5};
    step(&a,0,0);assert(!a.shots[0].life);
    /* Downward magic cannot cross a rock while below its height. */
    start(&a);a.shots[0]=(Projectile){.x=rock_edge-2,.y=230,.z=30,.vx=13,.life=47,.owner=0,.kind=SHOT_FIRE,.vz=-9};
    step(&a,0,0);assert(!a.shots[0].life);
    /* Holding B makes one normal jump, never the second-tap spin. */
    start(&a);for(int j=0;j<35;j++)step(&a,JUMP,0);
    assert(!a.p[0].attack_seq&&!a.p[0].z&&!a.p[0].air_spin_used);
    for(int slot=0;slot<2;slot++)for(int knight=0;knight<KNIGHT_COUNT;knight++){
        start(&a);a.p[slot].knight_id=knight;
        step(&a,slot?0:JUMP,slot?JUMP:0);step(&a,0,0);
        step(&a,slot?0:JUMP,slot?JUMP:0);
        assert(a.p[slot].air_spin_used==1&&a.p[slot].z<38&&!a.p[slot].attack_seq);
        step(&a,0,0);step(&a,0,0);
        assert(a.p[slot].pose==POSE_AIRSPIN&&a.p[slot].air_spin_used==2&&a.p[slot].attack_seq==1);
        assert(a.p[slot].vz==5); /* No invented second jump impulse. */
        int landed=0,apex=a.p[slot].z;
        for(int j=0;j<60;j++){
            step(&a,slot?0:(j%2?0:JUMP),slot?(j%2?0:JUMP):0);
            if(a.p[slot].z>apex)apex=a.p[slot].z;
            if(!a.p[slot].z){landed=1;break;}
        }
        assert(landed&&apex==55&&a.p[slot].attack_seq==1&&!a.p[slot].air_spin_used);
    }
    /* Another attack consumes an early pending spin instead of firing it later. */
    start(&a);step(&a,JUMP,0);step(&a,0,0);step(&a,JUMP,0);step(&a,ATTACK,0);
    assert(a.p[0].pose==POSE_AIRATTACK&&a.p[0].air_spin_used==2);
    for(int j=0;j<8;j++)step(&a,0,0);
    step(&a,JUMP,0);assert(a.p[0].pose!=POSE_AIRSPIN&&a.p[0].attack_seq==1);
    /* Taking a hit prevents second-jump escape until the next landing. */
    start(&a);a.p[0].z=40;
    a.shots[0]=(Projectile){.x=a.p[0].x+25,.y=285,.z=45,.vx=-5,.life=47,.owner=1,.kind=SHOT_FIRE};
    step(&a,0,0);assert(a.p[0].hitstun&&a.p[0].air_spin_used==2);
    step(&a,JUMP,0);assert(a.p[0].pose!=POSE_AIRSPIN);
    puts("PASS: physics, authored aerial/slam hits and bounded ground hit windows, bounded input buffer, close throws, shield breaks, recovery rolls, stun escape, air spin, fair sword locks and symmetric trades");
}

