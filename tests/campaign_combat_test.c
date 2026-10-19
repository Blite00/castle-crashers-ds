#include "campaign_combat.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void setup(Arena *a,Fighter e[6]){
    arena_init(a);a->running=1;a->map_id=3;a->p[0].x=150;a->p[1].x=100;
    for(int i=0;i<6;i++)e[i]=(Fighter){.x=180+i*5,.y=239,.hp=120,.mana=85,.facing=-1,.ready=i<4};
}
static void corpse_tests(void){
    Arena a;Fighter enemies[6],ally={0};CampaignCombatState state={0};
    uint8_t targets[6]={0},levels[2]={1,1},hits[2];uint16_t keys[2]={0};
    setup(&a,enemies);for(int i=0;i<6;i++)enemies[i].ready=0;
    enemies[0]=(Fighter){.x=200,.y=250,.ready=1,.pose=POSE_DEAD};
    ally=(Fighter){.x=300,.y=250,.ready=1,.pose=POSE_DEAD};
    a.p[1].hp=0;a.p[1].pose=POSE_DEAD;
    for(int frame=1;frame<=44;frame++){
        if(frame==11)enemies[1]=(Fighter){.x=240,.y=250,.ready=1,.pose=POSE_DEAD};
        campaign_combat_step(&state,&a,enemies,&ally,targets,keys,levels,hits);
        assert(enemies[0].animation==frame&&ally.animation==frame);
        int visible=frame<44&&(frame<33?(frame!=19&&frame!=23&&frame!=26&&frame!=29&&frame!=32):(frame&1));
        assert(campaign_npc_visible(&enemies[0])==visible&&campaign_npc_visible(&ally)==visible);
        assert(!hits[0]&&!hits[1]&&a.p[1].ready); /* Human corpse remains revivable. */
        if(frame>10)assert(enemies[1].animation==frame-10&&enemies[1].ready);
    }
    assert(!enemies[0].ready&&!ally.ready&&enemies[1].ready);
    for(int i=0;i<10;i++)campaign_combat_step(&state,&a,enemies,&ally,targets,keys,levels,hits);
    assert(!enemies[1].ready);
    /* Airborne death must land before its independent blink timeline starts. */
    enemies[0]=(Fighter){.x=200,.y=250,.z=30,.vz=3,.ready=1,.pose=POSE_AIRHURT};
    campaign_combat_step(&state,&a,enemies,&ally,targets,keys,levels,hits);
    assert(enemies[0].z>30&&!state.death_age[0]&&campaign_npc_visible(&enemies[0]));
    for(int i=0;i<30&&enemies[0].z;i++)campaign_combat_step(&state,&a,enemies,&ally,targets,keys,levels,hits);
    assert(!enemies[0].z&&state.death_age[0]==1);
}
static void clash_tests(void){
 Arena a;Fighter enemies[6]={0},ally={0};arena_init(&a);a.running=1;a.map_id=3;a.p[1].ready=0;a.clash_contacts=2;
 a.p[0]=(Fighter){.x=150,.y=250,.hp=100,.ready=1,.facing=1,.pose=POSE_HEAVY,.attack_kind=HIT_HEAVY,.cooldown=12,.animation=5};
 enemies[0]=(Fighter){.x=180,.y=250,.hp=100,.ready=1,.facing=-1,.pose=POSE_HEAVY,.attack_kind=HIT_HEAVY,.cooldown=12,.animation=5};
 uint16_t inputs[9]={0};uint8_t targets[9]={2,0,0},levels[2]={1,1},hits[2];
 arena_group_step(&a,enemies,&ally,inputs,targets,levels,hits);
 assert(a.clash_timer==90&&a.p[0].pose==POSE_CLASH&&enemies[0].pose==POSE_CLASH&&a.p[0].hp==100&&enemies[0].hp==100);
 CampaignCombatState ai={0};uint8_t chosen[6]={0};uint16_t keys[2]={0};
 for(int t=0;t<5;t++){campaign_combat_step(&ai,&a,enemies,&ally,chosen,keys,levels,hits);assert(a.clash_timer&&enemies[0].pose==POSE_CLASH);}
 for(int t=0;t<30&&a.clash_timer;t++){keys[0]=t%2?0:ATTACK|ATTACK_EDGE;campaign_combat_step(&ai,&a,enemies,&ally,chosen,keys,levels,hits);}
 assert(!a.clash_timer&&enemies[0].hp<100&&a.p[0].hp==100&&a.clash_cooldown);
}
int main(void){clash_tests();corpse_tests();setvbuf(stdout,NULL,_IONBF,0);
    Arena a;Fighter e[6],ally={0};CampaignCombatState state={0},state_copy={0};uint16_t input[9]={0};uint8_t target[9]={2,2,0,0,0},levels[2]={1,1},hits[2];
    setup(&a,e);a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].cooldown=8;a.p[0].animation=2;
    arena_group_step(&a,e,&ally,input,target,levels,hits);
    assert(e[0].hp==116&&e[1].hp==116&&e[2].hp==116&&hits[0]==4);
    assert(a.p[1].hp==ARENA_HEALTH&&a.tick==1);
    /* Every timer/vertical integration advances exactly once, independent of count. */
    setup(&a,e);a.p[0].z=30;a.p[0].vz=5;a.p[0].invulnerable=20;a.p[0].burn=31;
    for(int i=0;i<4;i++){e[i].z=30;e[i].vz=5;e[i].invulnerable=20;e[i].burn=31;}
    arena_group_step(&a,e,&ally,input,target,levels,hits);
    assert(a.p[0].z==35&&a.p[0].vz==4&&a.p[0].invulnerable==19&&a.p[0].burn==30);
    for(int i=0;i<4;i++)assert(e[i].z==35&&e[i].vz==4&&e[i].invulnerable==19&&e[i].burn==30);
    /* Simultaneous opposing hits trade; another enemy is independently hit. */
    setup(&a,e);a.p[0].pose=e[0].pose=POSE_ATTACK;a.p[0].attack_kind=e[0].attack_kind=HIT_LIGHT;
    a.p[0].animation=e[0].animation=2;a.p[0].cooldown=e[0].cooldown=8;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[0].hp==ARENA_HEALTH-4&&e[0].hp==116&&e[1].hp==116);
    /* Ally in projectile path is harmless; shot moves once and hits enemy. */
    setup(&a,e);a.p[1].x=170;a.shots[0]=(Projectile){.x=150,.y=239,.z=25,.vx=16,.vz=0,.life=30,.owner=0,.kind=SHOT_ARROW};
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[1].hp==ARENA_HEALTH&&e[0].hp==115&&hits[0]==1&&!a.shots[0].life);
    /* All three enemies pursue a living hero in the wrapper. */
    setup(&a,e);uint8_t targets[6]={0};uint16_t keys[2]={0};for(int i=0;i<4;i++)e[i].x=300+i*20;
    for(int t=0;t<3;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    for(int i=0;i<4;i++){assert(e[i].x<300+i*20||state.ranged[i]);if(!state.windup[i])assert(e[i].pose==POSE_WALK&&e[i].animation==2);}
    /* Campaign-only unlock gates do not discard the base heavy input. */
    setup(&a,e);a.p[0].combo=1;a.p[0].combo_timer=20;input[0]=HEAVY|HEAVY_EDGE;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[0].pose==POSE_HEAVY);
    setup(&a,e);a.p[0].combo=1;a.p[0].combo_timer=20;levels[0]=8;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[0].pose==POSE_UPPERCUT);
    setup(&a,e);levels[0]=1;a.p[0].z=40;a.p[0].vz=4;input[0]=JUMP|JUMP_EDGE;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[0].pose!=POSE_AIRSPIN);
    setup(&a,e);levels[0]=2;a.p[0].z=40;a.p[0].vz=4;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(a.p[0].pose==POSE_AIRSPIN);
    /* Repeated group ticks stay deterministic through movement and hit reactions. */
    setup(&a,e);campaign_combat_reset(&state);state_copy=state;Arena copy=a;Fighter others[6];memcpy(others,e,sizeof(e));uint8_t ta[6]={0},tb[6]={0},h2[2];
    unsigned random=713;
    for(int tick=0;tick<10000;tick++){
        random=random*1664525u+1013904223u;keys[0]=(random>>8)&INPUT_MASK;keys[1]=(random>>19)&INPUT_MASK;
        campaign_combat_step(&state,&a,e,&ally,ta,keys,levels,hits);campaign_combat_step(&state_copy,&copy,others,&ally,tb,keys,levels,h2);
        assert(!memcmp(&a,&copy,sizeof(a))&&!memcmp(e,others,sizeof(e))&&!memcmp(hits,h2,2)&&!memcmp(&state,&state_copy,sizeof(state)));
    }
    /* A lone enemy visibly anticipates a strike, then leaves a recovery gap. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;
    for(int i=1;i<4;i++)e[i].ready=0;
    keys[0]=keys[1]=0;levels[0]=1;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(state.windup[0]==10&&e[0].pose==POSE_ATTACK&&e[0].animation==0&&e[0].attack_kind==HIT_NONE);
    for(int t=0;t<9;t++){
        campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
        assert(!e[0].attack_seq&&a.p[0].hp==ARENA_HEALTH);
    }
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(e[0].attack_seq==1&&state.recovery[0]==30);
    for(int t=0;t<29;t++){campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(e[0].attack_seq==1);}
    /* Four surrounding enemies still cannot create repeated stun chains. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;int last_hit=-100,last_attack=-100,attacks=0,arrows=0;
    unsigned previous_seq[4]={0},previous_arrow[4]={0};uint16_t previous_hit=a.p[0].hit_seq;
    for(int t=0;t<600;t++){
        campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
        int telegraphs=0,active=0;
        for(int i=0;i<4;i++){
            if(e[i].arrow_seq!=previous_arrow[i]){arrows++;previous_arrow[i]=e[i].arrow_seq;}
            telegraphs+=!!state.windup[i];active+=e[i].cooldown&&e[i].attack_kind!=HIT_NONE;
            if(e[i].attack_seq!=previous_seq[i]){assert(t-last_attack>=30);last_attack=t;attacks++;previous_seq[i]=e[i].attack_seq;}
        }
        assert(telegraphs+active<=1);
        if(previous_hit!=a.p[0].hit_seq){assert(t-last_hit>=24);last_hit=t;previous_hit=a.p[0].hit_seq;assert(a.p[0].invulnerable>=15);}
        assert(a.p[0].stun_chain<6);
    }
    printf("Campaign pressure: %d attacks, %d damage across 600 ticks\n",attacks,ARENA_HEALTH-a.p[0].hp);
    assert(attacks+arrows>=12&&attacks+arrows<=17&&a.p[0].hp>=ARENA_HEALTH-68);
    /* A player can walk clear during anticipation instead of being tracked. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;for(int i=1;i<4;i++)e[i].ready=0;
    keys[0]=0;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    keys[0]=UP;for(int t=0;t<25;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(a.p[0].hp==ARENA_HEALTH&&e[0].attack_seq==1);
    /* Hitting during anticipation cancels it instead of producing a free trade. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;for(int i=1;i<4;i++)e[i].ready=0;
    keys[0]=0;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].animation=2;a.p[0].cooldown=8;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(!state.windup[0]&&e[0].hitstun&&state.recovery[0]>=15&&!e[0].attack_seq);
    /* Blocking a telegraphed swing still grants the full attacker recovery. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;for(int i=1;i<4;i++)e[i].ready=0;
    keys[0]=BLOCK;a.p[0].facing=1;
    for(int t=0;t<30;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(a.p[0].hp==ARENA_HEALTH&&a.p[0].block_seq==1&&e[0].attack_seq==1&&state.recovery[0]);
    /* Six shop enemies and a real friendly actor all advance exactly once. */
    setup(&a,e);memset(input,0,sizeof(input));for(int i=0;i<6;i++){e[i].ready=1;e[i].z=30;e[i].vz=5;e[i].invulnerable=20;}
    ally=(Fighter){.x=110,.y=239,.z=30,.vz=5,.hp=60,.facing=1,.ready=1,.invulnerable=20};
    arena_group_step(&a,e,&ally,input,target,levels,hits);
    assert(ally.z==35&&ally.vz==4&&ally.invulnerable==19);
    for(int i=0;i<6;i++)assert(e[i].z==35&&e[i].vz==4&&e[i].invulnerable==19);
    /* Ally attacks barbarians without hitting nearby heroes or awarding XP. */
    setup(&a,e);a.p[0].x=100;a.p[1].x=120;
    ally=(Fighter){.x=150,.y=239,.hp=60,.facing=1,.ready=1,.pose=POSE_ATTACK,.attack_kind=HIT_LIGHT,.animation=2,.cooldown=8};
    arena_group_step(&a,e,&ally,input,target,levels,hits);
    assert(e[0].hp==116&&a.p[0].hp==ARENA_HEALTH&&a.p[1].hp==ARENA_HEALTH&&!hits[0]&&!hits[1]);
    /* Barbarian attacks can kill the ally; it is not scenery or invincible. */
    setup(&a,e);a.p[0].x=90;a.p[1].x=100;
    ally=(Fighter){.x=150,.y=239,.hp=4,.facing=1,.ready=1};
    e[0].pose=POSE_ATTACK;e[0].attack_kind=HIT_LIGHT;e[0].animation=2;e[0].cooldown=8;
    arena_group_step(&a,e,&ally,input,target,levels,hits);assert(!ally.hp);
    /* The ally participates through its own input policy; nearby enemies target it. */
    setup(&a,e);campaign_combat_reset(&state);a.p[0].x=50;a.p[1].ready=0;keys[0]=0;
    ally=(Fighter){.x=150,.y=239,.hp=60,.facing=1,.ready=1};
    for(int t=0;t<50;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(ally.attack_seq>0&&targets[0]==8&&e[0].hp<120);
    /* Hero-owned projectiles pass through the gray ally; stale third shots are cleared. */
    setup(&a,e);ally=(Fighter){.x=168,.y=239,.hp=60,.facing=1,.ready=1};
    a.shots[0]=(Projectile){.x=150,.y=239,.z=25,.vx=16,.life=30,.owner=0,.kind=SHOT_ARROW};a.shots[2]=a.shots[0];
    arena_group_step(&a,e,&ally,input,target,levels,hits);
    assert(ally.hp==60&&e[0].hp==115&&!a.shots[2].life);
    /* Host replays remain identical with all nine actors and a live ally. */
    setup(&a,e);campaign_combat_reset(&state);for(int i=0;i<6;i++)e[i].ready=1;
    ally=(Fighter){.x=120,.y=260,.hp=60,.facing=1,.ready=1};Fighter ally_copy=ally;
    copy=a;memcpy(others,e,sizeof(e));state_copy=state;memset(ta,0,sizeof(ta));memset(tb,0,sizeof(tb));
    for(int t=0;t<5000;t++){
        random=random*1664525u+1013904223u;keys[0]=(random>>8)&INPUT_MASK;keys[1]=(random>>19)&INPUT_MASK;
        campaign_combat_step(&state,&a,e,&ally,ta,keys,levels,hits);
        campaign_combat_step(&state_copy,&copy,others,&ally_copy,tb,keys,levels,h2);
        assert(!memcmp(&a,&copy,sizeof(a))&&!memcmp(e,others,sizeof(e))&&!memcmp(&ally,&ally_copy,sizeof(ally))&&!memcmp(&state,&state_copy,sizeof(state)));
    }
    /* Waiting enemies spread into both depth lanes instead of queuing in a line. */
    setup(&a,e);campaign_combat_reset(&state);ally=(Fighter){0};a.p[1].ready=0;keys[0]=keys[1]=0;
    state.pressure[0]=30;int initial_y[6];for(int i=0;i<6;i++){e[i].ready=1;e[i].x=190+i*20;initial_y[i]=e[i].y;}
    for(int t=0;t<15;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    int moved_up=0,moved_down=0;
    for(int i=0;i<6;i++){moved_up+=e[i].y<initial_y[i];moved_down+=e[i].y>initial_y[i];assert(!e[i].attack_seq);}
    assert(moved_up&&moved_down);
    /* Repositioning reverses at real corridor edges on both campaign maps. */
    for(int map=3;map<=4;map++){
        setup(&a,e);a.map_id=map;a.p[1].ready=0;campaign_combat_reset(&state);state.pressure[0]=30;
        a.p[0].x=map==3?150:650;for(int y=0;y<448;y++)if(stage_position_open_map(map,a.p[0].x,y)){a.p[0].y=y;break;}
        for(int i=0;i<6;i++){
            e[i].ready=i==0;e[i].x=a.p[0].x+40;
            for(int y=0;y<448;y++)if(stage_position_open_map(map,e[i].x,y)){e[i].y=y;break;}
        }
        int edge_y=e[0].y;
        for(int t=0;t<15;t++){
            campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
            assert(stage_position_open_map(map,e[0].x,e[0].y));assert(stage_position_open_map(map,a.p[0].x,a.p[0].y));
        }
        assert(e[0].y>edge_y);
    }
    /* Repositioning never grants an extra attack token in nine-actor co-op. */
    setup(&a,e);campaign_combat_reset(&state);ally=(Fighter){.x=270,.y=269,.hp=100,.ready=1,.facing=-1};
    for(int i=0;i<6;i++){e[i].ready=1;e[i].x=180+i*15;e[i].y=239+(i%3)*10;}
    for(int t=0;t<600;t++){
        campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);int committed[3]={0};
        for(int i=0;i<6;i++)if(e[i].ready&&e[i].hp&&(state.windup[i]||(e[i].cooldown&&e[i].attack_kind!=HIT_NONE))){
            int target_slot=targets[i]==8?2:targets[i];assert(target_slot>=0&&target_slot<3);committed[target_slot]++;
        }
        for(int h=0;h<3;h++)assert(committed[h]<=1);
    }
    /* Source priority2 flanks the primary hunter; priority3 waits farther out. */
    setup(&a,e);campaign_combat_reset(&state);ally=(Fighter){0};a.p[1].ready=0;keys[0]=keys[1]=0;memset(targets,0,sizeof(targets));
    for(int i=0;i<6;i++){e[i].ready=i<3;e[i].x=180+i*20;}
    state.pressure[0]=60;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(e[1].x<200&&e[2].x>220&&!e[1].attack_seq&&!e[2].attack_seq);
    /* Nearby co-op movement cannot flip prey every frame; meaningful changes
       are picked up on the bounded reevaluation clock. */
    setup(&a,e);campaign_combat_reset(&state);a.p[0].x=80;a.p[1].x=300;a.p[1].y=239;
    for(int i=0;i<6;i++){e[i].ready=i==0;}e[0].x=150;memset(targets,0,sizeof(targets));
    state.pressure[0]=state.pressure[1]=200;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(targets[0]==0&&state.target_clock[0]==30);
    a.p[1].x=145;
    for(int t=0;t<29;t++){e[0].x=150;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(targets[0]==0);}
    e[0].x=150;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(targets[0]==1);
    /* Losing prey during anticipation cancels the old swing before retargeting. */
    setup(&a,e);campaign_combat_reset(&state);for(int i=1;i<6;i++)e[i].ready=0;memset(targets,0,sizeof(targets));
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(state.windup[0]);
    a.p[0].hp=0;a.p[1].x=150;a.p[1].y=239;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(targets[0]==1&&!state.windup[0]&&!e[0].attack_seq&&state.recovery[0]);
    /* Barbarians can guard a visibly committed frontal swing, with a bounded
       hold; merely holding the player's attack input is not a guard trigger. */
    setup(&a,e);campaign_combat_reset(&state);ally=(Fighter){0};a.p[1].ready=0;for(int i=1;i<6;i++)e[i].ready=0;memset(targets,0,sizeof(targets));
    keys[0]=ATTACK;campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(!state.guard[0]);
    campaign_combat_reset(&state);e[0].cooldown=0;e[0].attack_kind=HIT_NONE;a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].animation=2;a.p[0].cooldown=8;a.p[0].attack_seq=12;a.p[0].facing=1;keys[0]=0;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(state.guard[0]==29&&e[0].pose==POSE_BLOCK&&e[0].block_seq==1&&e[0].hp==120);
    for(int t=0;t<30;t++){campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);}assert(!state.guard[0]);
    /* Priority2 can use its configured bow while the nearest hunter recovers.
       It uses the same two shared projectile slots and120-tick shot clock. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;memset(targets,0,sizeof(targets));
    for(int i=0;i<6;i++){e[i].ready=i<2;}e[0].x=180;e[1].x=260;state.recovery[0]=100;
    campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(state.ranged[1]&&state.windup[1]==10&&e[1].pose==POSE_BOW);
    for(int t=0;t<22;t++)campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);
    assert(e[1].arrow_seq==1&&state.shot_wait[1]>90);int enemy_arrows=0;
    for(int s=0;s<2;s++){enemy_arrows+=a.shots[s].life&&a.shots[s].owner==3;}assert(enemy_arrows==1&&!a.shots[2].life);
    /* The three authored jab choices map to three existing light poses rather
       than every barbarian repeatedly using only the first slash. */
    setup(&a,e);campaign_combat_reset(&state);a.p[1].ready=0;for(int i=1;i<6;i++)e[i].ready=0;
    const int slash_poses[3]={POSE_ATTACK,POSE_LIGHT2,POSE_LIGHT3};
    for(int strike=0;strike<3;strike++){
        state.windup[0]=1;e[0].cooldown=0;e[0].attack_kind=HIT_NONE;
        campaign_combat_step(&state,&a,e,&ally,targets,keys,levels,hits);assert(e[0].pose==slash_poses[strike]);
    }
    puts("PASS: campaign group combat, deterministic replay, readable anticipation, recovery spacing, mob escape, interruption and directional blocking");
}

