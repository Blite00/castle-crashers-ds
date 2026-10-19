#include "arena.h"
#include "stage.h"
#include <stdlib.h>
#include <string.h>
static int clamp(int n,int lo,int hi){return n<lo?lo:n>hi?hi:n;}
const char *arena_knight_name(unsigned knight){
    static const char *names[]={"Blue Knight","Orange Knight","Red Knight","Green Knight"};
    return knight<KNIGHT_COUNT?names[knight]:"Unknown Knight";
}
int arena_spell_kind(unsigned knight){
    static const int kinds[]={SHOT_ICE,SHOT_FIRE,SHOT_LIGHTNING,SHOT_POISON};
    return knight<KNIGHT_COUNT?kinds[knight]:SHOT_ICE;
}
static void fighters_init(Arena *a){
    unsigned selected[2]={a->p[0].knight_id,a->p[1].knight_id};
    unsigned ready[2]={a->p[0].ready,a->p[1].ready};
    a->clash_timer=a->clash_cooldown=a->clash_contacts=0;
    a->clash_score[0]=a->clash_score[1]=0;
    memset(a->p,0,sizeof(a->p));memset(a->shots,0,sizeof(a->shots));
    a->p[0]=(Fighter){.x=80,.y=239,.hp=ARENA_HEALTH,.mana=ARENA_MANA,.facing=1};
    a->p[1]=(Fighter){.x=344,.y=239,.hp=ARENA_HEALTH,.mana=ARENA_MANA,.facing=-1};
    for(int i=0;i<2;i++){a->p[i].knight_id=selected[i];a->p[i].ready=ready[i];}
}
void arena_init(Arena *a){stage_set_map(0);memset(a,0,sizeof(*a));fighters_init(a);a->p[0].ready=a->p[1].ready=1;a->p[1].knight_id=KNIGHT_ORANGE;}
void arena_lobby_init(Arena *a){arena_init(a);a->p[0].ready=a->p[1].ready=0;}
int arena_players_ready(const Arena *a){return a&&a->p[0].ready&&a->p[1].ready&&a->p[0].knight_id<KNIGHT_COUNT&&a->p[1].knight_id<KNIGHT_COUNT;}
int arena_select_knight(Arena *a,unsigned slot,unsigned knight,int confirmed){
    if(!a||slot>1||knight>=KNIGHT_COUNT||(confirmed!=0&&confirmed!=1)||a->running)return 0;
    if(confirmed)a->p[slot].knight_id=knight;
    a->p[slot].ready=confirmed;return 1;
}
void arena_start_round(Arena *a){if(!arena_players_ready(a))return;fighters_init(a);a->running=1;a->winner=0;a->countdown=90;a->round++;}
static void pose(Fighter *p,int next){
    if(next!=p->pose){p->pose=next;p->animation=0;}
    else p->animation=(p->animation+1)%30000;
}
/* Authored punch_pt rectangles, half-pixel units after the DS 1/2 scale.
   Generated audit: original-hitboxes-audit.json. Outward rounding is <=0.5px.
   Target w=45,h=120 original units -> 22.5px halfwidth,60px height.
   Humanoid lane check uses original base30 ->15; unknown global offset=0.
   Front-center gating, throw/splash volumes remain arena adaptations. */
int arena_melee_overlap_body(const Fighter *p,const Fighter *q,int halfwidth,int height){
    int dx=(q->x-p->x)*p->facing,dy=q->y-p->y;
    if(p->attack_kind==HIT_STOMP)return !p->z&&abs(q->x-p->x)<30&&abs(dy)<10;
    if(p->attack_kind==HIT_THROW)return !p->z&&!q->z&&dx>=0&&dx<27&&abs(dy)<10;
    if(p->attack_kind==HIT_SPLASH)return dx>=0&&dx<56&&abs(dy)<19&&abs(q->z-p->z)<45;
    int left,top,right,bottom;
    switch(p->pose){
    case POSE_DASHPUNCH:
        if(p->animation>=4){left=-45;top=-84;right=134;bottom=4;}
        else{left=-47;top=-87;right=50;bottom=0;}break;
    case POSE_DASHSLAM:left=-21;top=-103;right=114;bottom=1;break;
    case POSE_ATTACK: left=-30;top=-93;right=97;bottom=14;break;
    case POSE_LIGHT2: left=-31;top=-96;right=91;bottom=7;break;
    case POSE_LIGHT3: left=-16;top=-99;right=128;bottom=1;break;
    case POSE_FINISHER: left=-30;top=-103;right=134;bottom=6;break;
    case POSE_HEAVY: left=-14;top=-111;right=100;bottom=5;break;
    case POSE_HEAVY3: left=-10;top=-122;right=135;bottom=15;break;
    case POSE_HEAVY2: left=-2;top=-96;right=110;bottom=10;break;
    case POSE_UPPERCUT: left=-12;top=-127;right=134;bottom=11;break;
    case POSE_AIRATTACK: left=-30;top=-99;right=97;bottom=20;break;
    case POSE_AIRATTACK2: left=-31;top=-122;right=91;bottom=16;break;
    case POSE_AIRHEAVY: left=-47;top=-82;right=83;bottom=19;break;
    case POSE_AIRSPIN: left=-44;top=-91;right=96;bottom=21;break;
    default:return 0;
    }
    if(abs(dy)>15||(dx<0&&p->pose!=POSE_AIRSPIN))return 0;
    int foot=2*(dy+p->z-q->z);
    return 2*dx+halfwidth>left&&2*dx-halfwidth<right&&foot>top&&foot-height*2<bottom;
}
int arena_melee_overlap(const Fighter *p,const Fighter *q){return arena_melee_overlap_body(p,q,45,60);}
int arena_campaign_strike(const Fighter *attacker,Fighter *q,int damage,int launch,int direction,int magic,int unblockable){
    int stomp=attacker->attack_kind==HIT_STOMP&&q->pose==POSE_KNOCKDOWN;
    if(q->hp<=0||(!stomp&&(q->invulnerable||q->pose==POSE_KNOCKDOWN)))return 0;
    if(q->guard&&q->facing==-direction&&!unblockable&&q->z==0){q->block_seq++;q->knockback=direction*2;return 0;}
    q->hp=q->hp>damage?q->hp-damage:0;q->flash=5;q->hit_seq++;
    q->hitstun=launch?15:8;q->knockback=direction*(launch?4:2);
    if(attacker->attack_kind==HIT_AIR&&q->z)q->knockback=direction*6;
    q->air_spin_used=2;q->float_timer=0;q->heavy_chain=0;q->combo=0;q->combo_timer=0;
    q->cooldown=0;q->attack_kind=HIT_NONE;q->queued=0;q->queue_timer=0;q->guard=0;q->invulnerable=4;
    q->stun_chain++;q->stun_timer=45;
    /* A sixth hit forces a protected knockdown: no endless ground/juggle loop. */
    int escape=q->stun_chain>=6;
    if(escape){q->freeze=0;launch=6;q->hitstun=15;q->invulnerable=30;q->stun_chain=0;}
    if(launch){q->vz=launch;if(!q->z)q->z=1;}
    if(magic&&!escape){if(attacker->knight_id==KNIGHT_BLUE){q->freeze=ARENA_FREEZE_TICKS;q->burn=0;}else if(attacker->knight_id==KNIGHT_ORANGE){q->burn=ARENA_BURN_TICKS;q->freeze=0;}}
    pose(q,q->z?POSE_AIRHURT:POSE_HURT);
    if(!q->hp){q->pose=q->z?POSE_AIRHURT:POSE_DEAD;q->animation=0;}
    return 1;
}
static int strike(Arena *a,int owner,int damage,int launch,int direction,int magic,int unblockable){
    return arena_campaign_strike(&a->p[owner],&a->p[1-owner],damage,launch,direction,magic,unblockable);
}
/* Original unlocks: ground magic at magic stat 1, air-down cast 10, splash 15, elemental chain 20 (f_SetupMagic magic_unlocks). */
static void begin_attack(Fighter *p,const Fighter *q,int buttons,unsigned level,int sprint,unsigned magic){
    if((buttons&BOW)&&!p->bow_equipped){p->queued=0;p->queue_timer=0;return;}
    if(p->air_spin_used==1)p->air_spin_used=2; /* Another committed action cancels the early second-jump tap. */
    int heavy=!!(buttons&HEAVY);
    p->hit_done=0;p->animation=0;p->queued=0;p->queue_timer=0;
    if(buttons&BOW){
        p->attack_kind=HIT_BOW;p->pose=POSE_BOW;p->cooldown=20;p->combo=0;p->heavy_chain=0;
        if(p->z)p->float_timer=11;
    }else if(buttons&MAGIC){
        int cost=heavy?25:16;if(p->mana<cost||(heavy&&magic<15)||(!heavy&&p->z>0&&magic<10))return;
        p->mana-=cost;p->attack_kind=heavy?HIT_SPLASH:HIT_CAST;
        int downward=!heavy&&p->z>0;
        p->pose=heavy?POSE_SPLASH:downward?POSE_CASTDOWN:POSE_CAST;p->cooldown=heavy?12:downward?13:15;
        if(downward)p->float_timer=10;
        p->magic_seq++;p->combo=0;p->heavy_chain=0;
    }else{
        if(sprint&&level>=4&&!p->z){
            /* Original combo slot2: true grounded sprint+A/X dispatches
               group4/1 dashpunch or4/2 dashslam before normal close attacks. */
            p->pose=heavy?POSE_DASHSLAM:POSE_DASHPUNCH;p->attack_kind=HIT_LIGHT;
            p->cooldown=12;p->combo=0;p->heavy_chain=0;p->combo_timer=0;
            if(heavy){p->z=1;p->vz=-2;p->float_timer=10;}
            p->attack_seq++;return;
        }
        if(!p->combo_timer){p->combo=0;p->heavy_chain=0;}
        int air_chain=p->heavy_chain;
        int followup=heavy&&!p->z?p->heavy_chain:0;
        p->heavy_chain=0;
        if(p->z&&heavy&&air_chain==2){p->attack_kind=HIT_HEAVY2;p->pose=POSE_HEAVY2;p->cooldown=12;p->combo=0;p->heavy_chain=3;}
        else if(p->z&&heavy&&air_chain==3){p->combo=p->combo==1?2:1;p->attack_kind=HIT_AIR;p->pose=p->combo==2?POSE_AIRATTACK2:POSE_AIRATTACK;p->cooldown=p->combo==2?12:9;p->heavy_chain=3;}
        else if(level>=8&&p->z&&heavy&&p->combo==1){p->attack_kind=HIT_LAUNCH;p->pose=POSE_UPPERCUT;p->cooldown=15;p->combo=0;}
        else if(p->z&&heavy){
            /* Source f_PunchSet2: first airborne heavy is slamdown, group12/1. */
            p->combo=0;p->heavy_chain=2;p->attack_kind=HIT_AIRHEAVY;p->pose=POSE_AIRHEAVY;p->cooldown=14;
            p->vz=0;p->float_timer=0;
        }
        else if(p->z){
            /* Source f_PunchSet1 alternates group11's two authored slashes. */
            p->combo=p->combo==1?2:1;p->attack_kind=HIT_AIR;p->pose=p->combo==2?POSE_AIRATTACK2:POSE_AIRATTACK;p->cooldown=p->combo==2?12:9;
        }
        else if(heavy&&q->hp>0&&q->pose==POSE_KNOCKDOWN&&abs(q->x-p->x)<30&&abs(q->y-p->y)<10&&!stage_segment_blocked(p->x,p->y,q->x,q->y)){
            p->attack_kind=HIT_STOMP;p->pose=POSE_STOMP;p->cooldown=24;p->combo=0;
        }
        else if(followup==1){p->attack_kind=HIT_HEAVY2;p->pose=POSE_HEAVY2;p->cooldown=12;p->combo=0;p->heavy_chain=2;}
        else if(followup==2){p->attack_kind=HIT_HEAVY3;p->pose=POSE_HEAVY3;p->cooldown=12;p->combo=0;}
        else if(heavy&&!p->z&&p->combo==3&&level>=50&&magic>=20){p->attack_kind=HIT_SPLASH;p->pose=POSE_SPLASH;p->cooldown=14;p->combo=0;p->magic_seq++;} /* Elemental chain: A,A,A,X (slot6). */
        else if(heavy&&!p->z&&p->combo==2&&level>=16){p->attack_kind=HIT_HEAVY3;p->pose=POSE_HEAVY3;p->cooldown=12;p->combo=0;p->heavy_chain=3;} /* A,A,X (slot4). */
        else if(followup==3&&level>=32){p->attack_kind=HIT_LAUNCH;p->pose=POSE_UPPERCUT;p->cooldown=15;p->combo=0;} /* A,A,X,X (slot5). */
        else if(level>=8&&heavy&&p->combo==1){p->attack_kind=HIT_LAUNCH;p->pose=POSE_UPPERCUT;p->cooldown=15;p->combo=0;}
        else if(heavy&&!q->z&&q->hp>0&&!q->invulnerable&&
                abs(q->x-p->x)<=22&&abs(q->y-p->y)<10&&(q->x-p->x)*p->facing>=0&&
                !stage_segment_blocked(p->x,p->y,q->x,q->y)){
            /* Adapted close-X grab; original throw sprite releases at frame 7. */
            p->attack_kind=HIT_THROW;p->pose=POSE_THROW;p->cooldown=17;p->combo=0;
        }
        else if(heavy){p->attack_kind=HIT_HEAVY;p->pose=POSE_HEAVY;p->cooldown=16;p->combo=0;p->heavy_chain=1;}
        else{
            p->combo=p->combo%4+1;const int poses[]={POSE_ATTACK,POSE_LIGHT2,POSE_LIGHT3,POSE_FINISHER};
            p->pose=poses[p->combo-1];p->attack_kind=p->combo==4?HIT_FINISH:HIT_LIGHT;p->cooldown=p->combo==4?18:p->combo==3?14:13;
        }
        p->attack_seq++;p->combo_timer=32;
    }
}
static int hit_frame(int pose){
    // Zero-based fp_PunchHit/fp_MagicMove events audited in original PLAYER.swf.
    switch(pose){
    case POSE_DASHPUNCH:case POSE_DASHSLAM:return 1;
    case POSE_HEAVY3:return 3;case POSE_STOMP:return 8;
    case POSE_ATTACK:return 2;case POSE_LIGHT2:return 4;case POSE_LIGHT3:return 3;
    case POSE_FINISHER:return 9;case POSE_HEAVY:return 6;case POSE_UPPERCUT:return 2;
    case POSE_CAST:return 5;case POSE_SPLASH:return 6;case POSE_AIRATTACK:return 2;
    case POSE_CASTDOWN:return 3;case POSE_AIRATTACK2:case POSE_AIRHEAVY:return 4;case POSE_BOW:return 10;case POSE_HEAVY2:return 1;case POSE_THROW:return 6;case POSE_AIRSPIN:return 2;
    default:return 3;
    }
}
/* A standard original jump is speed_launch=-20, gravity=2 at 30 Hz.
   Our world/art scale is one half, so velocity is +10 and gravity is 1. */
static int vertical_step(Fighter *p){
    if(p->float_timer){p->float_timer--;return 0;}
    if(!p->z&&!p->vz)return 0;
    p->z=clamp(p->z+p->vz,0,80);
    /* Source f_Jumping applies 0.3 gravity while a spin descends. Integer DS
       physics approximates it with one gravity step per three spin frames. */
    if(p->pose!=POSE_AIRSPIN||p->vz>0||p->animation%3==0)p->vz=clamp(p->vz-1,-16,10);
    if(!p->z){p->vz=0;p->air_spin_used=0;return 1;}
    return 0;
}
static void separate_fighters(Fighter *first,Fighter *second,const int old_x[2]){
    Fighter *left=first,*right=second;
    /* Adapted collision cylinders: jumping or changing lane permits passing. */
    if(abs(left->y-right->y)>=10||abs(left->z-right->z)>=24)return;
    if(old_x[0]>old_x[1]||(old_x[0]==old_x[1]&&first->facing<0)){left=second;right=first;}
    int overlap=16-(right->x-left->x);if(overlap<=0)return;
    int lx=left->x,rx=right->x;
    left->x=clamp(left->x-(overlap+1)/2,stage_min_x(),stage_max_x());stage_move(left,lx,left->y);
    right->x=clamp(right->x+overlap/2,stage_min_x(),stage_max_x());stage_move(right,rx,right->y);
    if(right->x-left->x<16){
        rx=right->x;right->x=clamp(left->x+16,stage_min_x(),stage_max_x());stage_move(right,rx,right->y);
        lx=left->x;left->x=clamp(right->x-16,stage_min_x(),stage_max_x());stage_move(left,lx,left->y);
    }
}
static void settled_pose(Fighter *p){
    if(!p->hp){
        pose(p,p->z?POSE_AIRHURT:POSE_DEAD);
        if(p->pose==POSE_DEAD&&p->animation>1)p->animation=1;
    }
}
/* Preserve taps captured between simulation ticks, including network replies. */
static uint16_t input_pressed(Fighter *p,uint16_t input){
    uint16_t pressed=(input&INPUT_MASK)&~p->previous;
    if(input&ATTACK_EDGE)pressed|=ATTACK;
    if(input&HEAVY_EDGE)pressed|=HEAVY;
    if(input&JUMP_EDGE)pressed|=JUMP;
    if(input&BOW_EDGE)pressed|=BOW;
    p->previous=input&INPUT_MASK;return pressed;
}
static void clash_step(Arena *a,const uint16_t input[2]){
    for(int i=0;i<2;i++){
        if(input_pressed(&a->p[i],input[i])&(ATTACK|HEAVY|JUMP|BOW))a->clash_score[i]++;
        pose(&a->p[i],POSE_CLASH);
    }
    a->clash_timer--;
    int difference=(int)a->clash_score[0]-a->clash_score[1];
    if(a->clash_timer&&abs(difference)<5&&a->clash_score[0]<10&&a->clash_score[1]<10)return;
    a->clash_timer=0;a->clash_cooldown=180;
    for(int i=0;i<2;i++){a->p[i].pose=POSE_IDLE;a->p[i].animation=0;a->p[i].previous=input[i]&INPUT_MASK;}
    if(difference){
        int winner=difference>0?0:1;
        strike(a,winner,18,8,a->p[winner].facing,0,1);
        if(!a->p[1-winner].hp){a->winner=winner+1;a->wins[winner]++;}
        a->p[winner].pose=POSE_FINISHER;a->p[winner].cooldown=12;a->p[winner].invulnerable=12;
    }else{
        for(int i=0;i<2;i++){a->p[i].knockback=-a->p[i].facing*4;a->p[i].invulnerable=12;}
    }
}
int arena_fighter_running(const Fighter *p){
    return p->hp>0&&p->pose==POSE_WALK&&p->animation>=ARENA_RUN_TICKS&&!p->z&&!p->guard&&!p->hitstun&&!p->freeze&&!p->cooldown;
}
static void advance_fighter(Fighter *p,const Fighter *q,uint16_t input,uint32_t tick,unsigned level,const ArenaCampaignAttributes *attributes,int can_run){
        int from_x=p->x,from_y=p->y;uint16_t k=input&INPUT_MASK,pressed=input_pressed(p,input);
        int run=0,walking_reset=0;
        if(!p->bow_equipped){if(pressed&BOW)p->equip_timer=30;pressed&=~BOW;}
        if(p->stun_timer&&!--p->stun_timer)p->stun_chain=0;
        if(p->flash)p->flash--;
        if(p->invulnerable)p->invulnerable--;
        if(p->combo_timer)p->combo_timer--;else{p->combo=0;p->heavy_chain=0;}
        if(p->queue_timer&&!--p->queue_timer)p->queued=0;
        // Original magic-10 regeneration is .55 mana/frame.
        if(attributes){
            unsigned rate=45+attributes->magic,maximum=35+5*attributes->magic;
            if((tick*rate)%100<rate&&p->mana<(int)maximum)p->mana++;
        }else if(((tick%20)*11)%20<11&&p->mana<ARENA_MANA)p->mana++;
        if(p->burn){if(p->burn%30==0&&p->hp>0){p->hp--;p->hit_seq++;}p->burn--;}
        if(!p->hp){vertical_step(p);settled_pose(p);return;}
        if(p->freeze){p->freeze--;p->guard=0;pose(p,p->z?POSE_AIRHURT:POSE_HURT);return;}
        if(p->hitstun)p->hitstun--;
        if(p->knockback){p->x=clamp(p->x+p->knockback,stage_min_x(),stage_max_x());p->knockback+=p->knockback>0?-1:1;}
        if(p->cooldown){p->cooldown--;p->animation++;}
        int was_knocked=p->pose==POSE_AIRHURT&&p->z;
        int landed=vertical_step(p);
        if(landed&&(was_knocked||!p->hitstun)){
            p->pose=was_knocked?POSE_KNOCKDOWN:POSE_LAND;
            p->animation=0;p->cooldown=was_knocked?21:6;p->hitstun=0;
            if(was_knocked){p->invulnerable=30;p->stun_chain=0;}
            p->attack_kind=HIT_NONE;p->hit_done=1;p->combo=0;p->heavy_chain=0;
            if(was_knocked){p->queued=0;p->queue_timer=0;}
            else if(p->queued){p->queue_timer=8;} /* One valid late tap survives the six-tick landing. */
        }
        if(p->pose==POSE_KNOCKDOWN&&p->cooldown&&((pressed&JUMP)||(k&BLOCK))){
            p->pose=POSE_ROLL;p->animation=0;p->cooldown=12;p->invulnerable=18;
            p->knockback=0;p->stun_chain=0;
            if(k&(LEFT|RIGHT))p->facing=(k&RIGHT)?1:-1;
        }
        if((p->pose==POSE_DASHPUNCH||p->pose==POSE_DASHSLAM)&&p->cooldown&&!p->hitstun){
            /* Source dashpunch speed15 decays1 each tick; dashslam speed18
               remains constant through its ten-tick airborne dive. Halfscale. */
            int speed=p->pose==POSE_DASHSLAM?9:(16-p->animation)/2;
            if(speed<1)speed=1;
            p->x=clamp(p->x+p->facing*speed,stage_min_x(),stage_max_x());
            stage_move(p,from_x,from_y);
            return;
        }
        if(p->pose==POSE_ROLL&&p->cooldown){
            p->x=clamp(p->x+p->facing*4,stage_min_x(),stage_max_x());
            stage_move(p,from_x,from_y);return;
        }
        int knocked=p->pose==POSE_AIRHURT&&p->z;
        /* Original second-jump topspin requires height >75 source pixels.
           An early rising tap waits for38 DS units; it never boosts velocity. */
        if(level>=2&&(pressed&JUMP)&&p->z&&!p->hitstun&&!p->cooldown&&!knocked&&!p->air_spin_used&&(p->z>=38||p->vz>0))p->air_spin_used=1;
        if(p->air_spin_used==1&&p->z>=38&&!p->hitstun&&!p->cooldown&&!knocked){
            p->air_spin_used=2;p->pose=POSE_AIRSPIN;p->animation=0;p->cooldown=13;
            p->attack_kind=HIT_SPIN;p->hit_done=0;p->attack_seq++;p->combo=0;p->heavy_chain=0;
            p->queued=0;p->queue_timer=0;
        }
        p->guard=!!(k&BLOCK)&&!p->z&&!p->hitstun&&!p->cooldown;
        int dx=!!(k&RIGHT)-!!(k&LEFT),dy=!!(k&DOWN)-!!(k&UP);
        if(!p->hitstun&&!p->cooldown&&!knocked){
            if(dx)p->facing=dx;
            if(p->guard){
                p->x=clamp(p->x+dx,stage_min_x(),stage_max_x());
                p->y=clamp(p->y+dy,stage_min_y(),stage_max_y());
            }else{
                // Half-scale horizontal speed; adapted depth movement.
                int sx=tick%4?3:2,sy=tick%3?2:1;
                if(attributes){
                    unsigned agility=attributes->agility>25?25:attributes->agility;
                    unsigned speed=45+agility;
                    sx=speed/20+((tick*(speed%20))%20<speed%20);
                    sy=(speed*3)/100+((tick*((speed*3)%100))%100<(speed*3)%100);
                }
                /* Source dash1.8 -> true speed2.2 after30 grounded ticks. Preserve
                   that22/18 increase over DS baseline pacing, rather than an
                   abrupt2.2x jump. Direction changes do not reset source timer. */
                run=can_run&&p->pose==POSE_WALK&&p->animation>=ARENA_RUN_TICKS-1&&!walking_reset&&!p->z&&!(k&BLOCK)&&!(pressed&(ATTACK|HEAVY|BOW|JUMP))&&!p->queued;
                if(run){
                    unsigned agility=attributes?(attributes->agility>25?25:attributes->agility):10;
                    unsigned speed=(45+agility)*22;
                    sx=speed/360+((tick*(speed%360))%360<speed%360);
                    sy=(speed*3)/1800+((tick*((speed*3)%1800))%1800<(speed*3)%1800);
                }
                p->x=clamp(p->x+dx*sx,stage_min_x(),stage_max_x());
                p->y=clamp(p->y+dy*sy,stage_min_y(),stage_max_y());
                if((pressed&JUMP)&&!p->z){p->z=10;p->vz=9;p->air_spin_used=0;p->combo=0;p->heavy_chain=0;}
            }
        }
        stage_move(p,from_x,from_y);
        /* Sliding along a wall or turning into a depth lane keeps momentum. */
        if((dx||dy)&&p->x==from_x&&p->y==from_y)walking_reset=1;
        /* One late-recovery tap is buffered. Early spam cannot refresh it or
           turn a held button into an automatic combo. */
        if((pressed&(ATTACK|HEAVY|BOW))&&!p->queued&&(p->cooldown<=7||(p->pose==POSE_STOMP&&p->animation>=14))&&p->hitstun<=7&&!p->guard&&!knocked){
            p->queued=(pressed&(ATTACK|HEAVY|BOW))|(k&MAGIC);p->queue_timer=8;
        }
        /* Authored punching=false markers permit only these aerial follow-ups;
           otherwise the full pose recovery still plays. */
        int air_chain_ready=(p->pose==POSE_HEAVY3&&p->animation>=9)||(p->pose==POSE_STOMP&&p->animation>=14)||(p->z&&((p->pose==POSE_AIRATTACK&&p->animation>=5)||(p->pose==POSE_AIRATTACK2&&p->animation>=6)||(p->pose==POSE_AIRHEAVY&&p->animation>=8)));
        if((!p->cooldown||air_chain_ready)&&!p->hitstun&&!p->guard&&!knocked){
            if(!p->cooldown)p->attack_kind=HIT_NONE;
            int buttons=p->queued;
            if(buttons&(ATTACK|HEAVY|BOW))begin_attack(p,q,buttons,level,can_run&&!walking_reset&&arena_fighter_running(p),attributes?attributes->magic:99);
        }
        if(!p->cooldown){
            if(p->hitstun||knocked)pose(p,p->z?POSE_AIRHURT:POSE_HURT);
            else {
                pose(p,p->guard?POSE_BLOCK:p->z?POSE_JUMP:(dx||dy)?POSE_WALK:POSE_IDLE);
                if(p->pose==POSE_WALK){
                    if(walking_reset)p->animation=0;
                    else if(run)p->animation=(p->animation+1)%30000; /* Faster existing walk cycle. */
                }
            }
        }
}
static void duel_step(Arena *a,const uint16_t input[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2]){
    stage_set_map(a->map_id);
    if(!a->running)return;
    /* Equipment is synchronized input, independent of action recovery. It
       changes the next contextual Y action, never cancels an active move. */
    for(int i=0;i<2;i++){
        Fighter *p=&a->p[i];
        if(p->equip_timer)p->equip_timer--;
        if(!attributes&&!a->winner&&p->hp>0&&(((input[i]&EQUIP)&&!(p->previous&EQUIP))||(input[i]&EQUIP_EDGE))){
            p->bow_equipped=!p->bow_equipped;p->equip_timer=30;
        }
        p->previous=(p->previous&~EQUIP)|(input[i]&EQUIP);
    }
    if(a->winner){
        a->tick++;
        for(int i=0;i<2;i++){
            Fighter *p=&a->p[i];
            if(p->cooldown){p->cooldown--;p->animation++;}
            int landed=vertical_step(p);
            if(!p->hp)settled_pose(p);
            else if(landed){p->pose=POSE_LAND;p->animation=0;p->cooldown=6;}
            else if(!p->cooldown)pose(p,p->z?POSE_JUMP:POSE_IDLE);
        }
        return;
    }
    int old_x[2]={a->p[0].x,a->p[1].x};
    a->tick++;
    if(a->clash_timer){clash_step(a,input);return;}
    if(a->clash_cooldown)a->clash_cooldown--;
    if(a->countdown){a->countdown--;return;}
    for(int i=0;i<2;i++)advance_fighter(&a->p[i],&a->p[1-i],input[i],a->tick,levels?levels[i]:99,attributes?&attributes[i]:0,1);
    separate_fighters(&a->p[0],&a->p[1],old_x);
    struct {int damage,launch,direction,magic,air,unblockable,throwing;} impacts[2]={{0}};
    for(int i=0;i<2;i++){
        Fighter *p=&a->p[i],*q=&a->p[1-i];
        if((p->attack_kind==HIT_SPIN&&p->animation==8)||(p->attack_kind==HIT_HEAVY2&&p->animation==5))p->hit_done=0;
        int onset=p->attack_kind==HIT_SPIN&&p->animation>=8?8:p->attack_kind==HIT_HEAVY2&&p->animation>=5?5:hit_frame(p->pose);
        if(p->attack_kind==HIT_NONE||p->hit_done||p->animation<onset||p->hitstun||p->freeze||p->hp<=0)continue;
        /* Air light and the new source slams use a single authored hit event.
           Other moves retain the DS three-tick responsiveness window. */
        if(p->animation>=onset+(p->pose==POSE_DASHPUNCH?5:p->pose==POSE_DASHSLAM?10:(p->attack_kind==HIT_AIR||p->attack_kind==HIT_HEAVY3||p->attack_kind==HIT_STOMP)?1:3)){p->hit_done=1;continue;}
        if(p->attack_kind==HIT_CAST||p->attack_kind==HIT_BOW){
            p->hit_done=1;
            for(int s=0;s<(attributes?2:ARENA_SHOTS);s++)if(!a->shots[s].life){
                int arrow=p->attack_kind==HIT_BOW;
                int downward=p->pose==POSE_CASTDOWN;
                /* Agility10: original damage12/speed32.5, halfscale rounded16.
                   Vertical launch5/gravity1 become2.5/0.5 via half-unit state. */
                a->shots[s]=(Projectile){.x=p->x,.y=p->y,.z=p->z+(arrow?25:14),.vx=p->facing*(arrow?16:downward?13:5),.life=48,.owner=i,.kind=arrow?SHOT_ARROW:arena_spell_kind(p->knight_id),.vz=arrow?5:downward?-9:0};
                if(arrow)p->arrow_seq++;
                break;
            }
        }else{
            int splash=p->attack_kind==HIT_SPLASH;
            int spin=p->attack_kind==HIT_SPIN;
            int throwing=p->attack_kind==HIT_THROW;
            int dx=q->x-p->x;
            if((!q->invulnerable||(p->attack_kind==HIT_STOMP&&q->pose==POSE_KNOCKDOWN))&&q->hp>0&&arena_melee_overlap(p,q)&&
               (p->z>=56||q->z>=56||!stage_segment_blocked(p->x,p->y,q->x,q->y))){
                p->hit_done=1;
                int damage=(p->attack_kind==HIT_HEAVY3||p->attack_kind==HIT_STOMP)?18:p->attack_kind==HIT_AIRHEAVY?22:p->attack_kind==HIT_HEAVY2?(p->animation>=5?16:12):spin?20:throwing?18:p->attack_kind==HIT_HEAVY?18:p->attack_kind==HIT_FINISH?26:p->attack_kind==HIT_LAUNCH?24:splash?24:15;
                int launch=p->pose==POSE_DASHPUNCH?4:p->pose==POSE_DASHSLAM?13:p->attack_kind==HIT_HEAVY3?-16:p->attack_kind==HIT_STOMP?5:p->attack_kind==HIT_AIRHEAVY?-16:p->attack_kind==HIT_HEAVY2?(p->z&&p->animation>=5?5:0):spin?6:throwing?10:p->attack_kind==HIT_LAUNCH?8:p->attack_kind==HIT_FINISH?5:p->attack_kind==HIT_AIR?(q->z?4:9):0;
                if(attributes){int base=3+(levels?levels[i]:1)+attributes[i].strength;damage=damage*base/15;damage=damage*(120-attributes[1-i].defense)/100;if(damage<1)damage=1;}
                impacts[i].damage=damage;impacts[i].launch=launch;impacts[i].direction=dx?(dx<0?-1:1):p->facing;impacts[i].magic=splash;
                impacts[i].air=p->attack_kind==HIT_AIR||p->attack_kind==HIT_AIRHEAVY;
                impacts[i].unblockable=splash||throwing||p->attack_kind==HIT_STOMP;
                impacts[i].throwing=throwing;
            }
        }
    }
    /* Every third eligible ground trade can lock swords; ordinary trades remain.
       Source f_SwordLockMash uses a five-point lead or ten total taps.
       The 90-tick timeout and deterministic cadence are DS arena adaptations. */
    if(!a->clash_cooldown&&impacts[0].damage&&impacts[1].damage&&
       !impacts[0].unblockable&&!impacts[1].unblockable&&!a->p[0].z&&!a->p[1].z&&
       (a->p[0].attack_kind==HIT_HEAVY||a->p[1].attack_kind==HIT_HEAVY||
        a->p[0].attack_kind==HIT_FINISH||a->p[1].attack_kind==HIT_FINISH||
        a->p[0].attack_kind==HIT_HEAVY2||a->p[1].attack_kind==HIT_HEAVY2)){
        a->clash_contacts=(a->clash_contacts+1)%3;
        if(!a->clash_contacts){
            a->clash_timer=90;a->clash_score[0]=a->clash_score[1]=0;
            for(int i=0;i<2;i++){
                Fighter *p=&a->p[i];p->pose=POSE_CLASH;p->animation=0;p->cooldown=0;
                p->attack_kind=HIT_NONE;p->guard=0;p->knockback=0;p->queued=0;p->queue_timer=0;
            }
            return;
        }
    }
    // Evaluate both hitboxes before applying hits; neither console wins a trade by slot number.
    int accepted[2]={0};
    for(int i=0;i<2;i++)if(impacts[i].damage){
        accepted[i]=strike(a,i,impacts[i].damage,impacts[i].launch,impacts[i].direction,impacts[i].magic,impacts[i].unblockable);
        /* Source default thrown velocity 12 becomes six at half world scale. */
        if(accepted[i]&&impacts[i].throwing)a->p[1-i].knockback=impacts[i].direction*6;
    }
    for(int i=0;i<2;i++)if(accepted[i]&&impacts[i].air&&a->p[i].hp>0&&!a->p[i].hitstun){
        /* Source contact pause preserves the attacker's momentum. The victim
           follows its own toss arc; never synchronize or freeze both actors. */
        if(a->p[i].attack_kind==HIT_AIRHEAVY)a->p[i].vz=7;
        else a->p[i].float_timer=7;
    }
    for(int s=0;s<(attributes?2:ARENA_SHOTS);s++){
        Projectile *p=&a->shots[s];if(!p->life)continue;
        int previous_x=p->x,previous_height=2*p->z+p->fraction;
        int downward=p->kind!=SHOT_ARROW&&p->vz==-9;
        /* Include the entire muzzle path on the creation tick. */
        if(p->life==48)p->x+=p->vx>0?16:-16;
        p->life--;
        int movement=downward?(p->vx>0?1:-1)*(p->life&1?6:7):p->vx;
        p->x+=movement;
        int height=previous_height;
        if(p->kind==SHOT_ARROW||downward){
            height+=p->vz;
            if(p->kind==SHOT_ARROW)p->vz--;
        }
        int floor_hit=height<0;
        if(floor_hit){
            /* Clip the collision segment at the floor rather than hitting past it. */
            p->x=previous_x+(p->x-previous_x)*previous_height/(previous_height-height);
            height=0;
        }
        p->z=height/2;p->fraction=height%2;
        int rock_from=previous_x,rock_to=p->x;
        if(previous_height>=112&&height<112)
            rock_from=previous_x+(p->x-previous_x)*(previous_height-112)/(previous_height-height);
        if(previous_height<112&&height>=112)
            rock_to=previous_x+(p->x-previous_x)*(112-previous_height)/(height-previous_height);
        if(p->x<0||p->x>=stage_world_width(a->map_id)||((height<112||previous_height<112)&&stage_projectile_blocked(rock_from,rock_to,p->y))){p->life=0;continue;}
        Fighter *q=&a->p[1-p->owner];
        /* Clip a swept diagonal to the body's horizontal slab, then compare
           heights only on that interval; its bounding box alone overhits corners. */
        int direction=p->x>=previous_x?1:-1;
        int length=2*abs(p->x-previous_x);
        int enter=direction>0?2*q->x-45-2*previous_x:2*previous_x-(2*q->x+45);
        int leave=direction>0?2*q->x+45-2*previous_x:2*previous_x-(2*q->x-45);
        if(enter<0)enter=0;
        if(leave>length)leave=length;
        int touches=0;
        if(length&&enter<=leave){
            int first=previous_height*length+(height-previous_height)*enter;
            int last=previous_height*length+(height-previous_height)*leave;
            int low=first<last?first:last,high=first>last?first:last;
            touches=high>=2*q->z*length&&low<(2*q->z+120)*length;
        }else if(!length&&abs(p->x-q->x)<=22){
            int low=previous_height<height?previous_height:height;
            int high=previous_height>height?previous_height:height;
            touches=high>=2*q->z&&low<2*q->z+120;
        }
        if(touches&&abs(q->y-p->y)<=15){
            int damage=p->kind==SHOT_ARROW?12:18;if(attributes){damage=p->kind==SHOT_ARROW?2+attributes[p->owner].agility:3+(levels?levels[p->owner]:1)+attributes[p->owner].magic;damage=damage*(120-attributes[1-p->owner].defense)/100;if(damage<1)damage=1;}
            strike(a,p->owner,damage,0,p->vx>0?1:-1,p->kind!=SHOT_ARROW,0);p->life=0;
        }
        if(floor_hit)p->life=0;
    }
    if(!a->p[0].hp||!a->p[1].hp){a->winner=!a->p[0].hp?(!a->p[1].hp?3:2):1;if(a->winner<3)a->wins[a->winner-1]++;}
}
void arena_step(Arena *a,const uint16_t input[2]){duel_step(a,input,0,0);}
void arena_campaign_duel_step(Arena *a,const uint16_t input[2],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2]){duel_step(a,input,levels,attributes);}
/* Humanoid campaign locks use the existing arena counters. The two clashing
   poses identify the pair, so no permanent actor-index storage is required. */
static void group_clash_tick(Arena*a,Fighter*const f[9],const uint16_t input[9],uint8_t hits[2]){
 int hero=-1,enemy=-1;for(int i=0;i<8;i++)if(f[i]->ready&&f[i]->hp>0&&f[i]->pose==POSE_CLASH){if(i<2)hero=i;else if(f[i]->knight_id!=3)enemy=i;}
 if(hero<0||enemy<0){a->clash_timer=0;for(int i=0;i<9;i++)if(f[i]->pose==POSE_CLASH){f[i]->pose=POSE_IDLE;f[i]->invulnerable=12;}return;}
 Fighter*p=f[hero],*q=f[enemy];
 if(input_pressed(p,input[hero])&(ATTACK|HEAVY|JUMP|BOW))a->clash_score[0]++;
 if(a->tick%8==0)a->clash_score[1]++;
 p->invulnerable=q->invulnerable=2;p->animation++;q->animation++;
 a->clash_timer--;int difference=(int)a->clash_score[0]-a->clash_score[1];
 if(a->clash_timer&&abs(difference)<5&&a->clash_score[0]<10&&a->clash_score[1]<10)return;
 a->clash_timer=0;a->clash_cooldown=180;p->pose=q->pose=POSE_IDLE;p->animation=q->animation=0;p->invulnerable=q->invulnerable=0;
 if(difference){Fighter*w=difference>0?p:q,*loser=difference>0?q:p;
  if(arena_campaign_strike(w,loser,6,8,w->facing,0,1)&&difference>0)hits[hero]++;
  w->pose=POSE_FINISHER;w->animation=0;w->cooldown=12;w->invulnerable=12;
 }else{p->knockback=-p->facing*4;q->knockback=-q->facing*4;p->invulnerable=q->invulnerable=12;}
}



/* Campaign teams share the duel's fighter update and hit reactions. All nine
   actors advance once, and all melee contacts are collected before damage. */
void arena_group_step_attributes(Arena *a,Fighter enemies[6],Fighter *ally,const uint16_t input[9],const uint8_t targets[9],const uint8_t levels[2],const ArenaCampaignAttributes attributes[2],uint8_t landed_hits[2]){
    stage_set_map(a->map_id);
    Fighter *fighters[9]={&a->p[0],&a->p[1],&enemies[0],&enemies[1],&enemies[2],&enemies[3],&enemies[4],&enemies[5],ally};
    int old_x[9];for(int i=0;i<9;i++)old_x[i]=fighters[i]->x;
    Projectile *shots=a->shots;
    for(int s=2;s<ARENA_SHOTS;s++)shots[s].life=0;
    landed_hits[0]=landed_hits[1]=0;
    a->tick++;
    if(a->clash_cooldown)a->clash_cooldown--;
    if(a->clash_timer)group_clash_tick(a,fighters,input,landed_hits);
    for(int i=0;i<9;i++){
        Fighter *p=fighters[i];if(!p->ready)continue;
        if(p->pose==POSE_CLASH)continue;
        if(p->equip_timer)p->equip_timer--;
        if(p->hp>0&&(((input[i]&EQUIP)&&!(p->previous&EQUIP))||(input[i]&EQUIP_EDGE))){p->bow_equipped=!p->bow_equipped;p->equip_timer=30;}
        p->previous=(p->previous&~EQUIP)|(input[i]&EQUIP);
        Fighter *target=fighters[targets[i]<9?targets[i]:0];
        if(!p->z&&((input[i]|p->queued)&HEAVY))for(int j=0;j<9;j++){
            Fighter *fallen=fighters[j];
            if(fallen->ready&&fallen->hp>0&&fallen->pose==POSE_KNOCKDOWN&&
               ((i<2||(i==8&&!(a->map_id==6&&!ally->ready))||((i>=2&&i<8)&&fighters[i]->knight_id==3))!=(j<2||(j==8&&!(a->map_id==6&&!ally->ready))||((j>=2&&j<8)&&fighters[j]->knight_id==3)))&&abs(fallen->x-p->x)<30&&abs(fallen->y-p->y)<10&&
               !stage_segment_blocked(p->x,p->y,fallen->x,fallen->y)){target=fallen;break;}
        }
        advance_fighter(p,target,input[i],a->tick,i<2?levels[i]:99,i<2&&attributes?&attributes[i]:0,i<2);
    }
    for(int i=0;i<9;i++)for(int j=i+1;j<9;j++){
        if(!fighters[i]->ready||!fighters[j]->ready||!fighters[i]->hp||!fighters[j]->hp||fighters[i]->pose==POSE_CLASH||fighters[j]->pose==POSE_CLASH)continue;
        int before[2]={old_x[i],old_x[j]};
        separate_fighters(fighters[i],fighters[j],before);
    }
    struct {int8_t damage,launch,direction,magic,air,unblockable,throwing;} impacts[9][9]={{{0}}};
    for(int i=0;i<9;i++){
        Fighter *p=fighters[i];if(!p->ready)continue;
        if((p->attack_kind==HIT_SPIN&&p->animation==8)||(p->attack_kind==HIT_HEAVY2&&p->animation==5))p->hit_done=0;
        int onset=p->attack_kind==HIT_SPIN&&p->animation>=8?8:p->attack_kind==HIT_HEAVY2&&p->animation>=5?5:hit_frame(p->pose);
        if(p->attack_kind==HIT_NONE||p->hit_done||p->animation<onset||p->hitstun||p->freeze||p->hp<=0)continue;
        /* Air light and the new source slams use a single authored hit event.
           Other moves retain the DS three-tick responsiveness window. */
        if(p->animation>=onset+(p->pose==POSE_DASHPUNCH?5:p->pose==POSE_DASHSLAM?10:(p->attack_kind==HIT_AIR||p->attack_kind==HIT_HEAVY3||p->attack_kind==HIT_STOMP)?1:3)){p->hit_done=1;continue;}
        if(p->attack_kind==HIT_CAST||p->attack_kind==HIT_BOW){
            p->hit_done=1;
            for(int s=0;s<2;s++)if(!shots[s].life){
                int arrow=p->attack_kind==HIT_BOW;
                int downward=p->pose==POSE_CASTDOWN;
                /* Agility10: original damage12/speed32.5, halfscale rounded16.
                   Vertical launch5/gravity1 become2.5/0.5 via half-unit state. */
                shots[s]=(Projectile){.x=p->x,.y=p->y,.z=p->z+(arrow?25:14),.vx=p->facing*(arrow?16:downward?13:5),.life=48,.owner=i,.kind=arrow?SHOT_ARROW:arena_spell_kind(p->knight_id),.vz=arrow?5:downward?-9:0};
                if(arrow){if(i>=2&&i<8)shots[s].vx=p->facing*10;if(i<2&&attributes)shots[s].vx=p->facing*((2100+115*attributes[i].agility+100)/200);p->arrow_seq++;}
                break;
            }
        }else{
            for(int j=0;j<9;j++){
            if(!fighters[j]->ready||((i<2||(i==8&&!(a->map_id==6&&!ally->ready))||((i>=2&&i<8)&&fighters[i]->knight_id==3))==(j<2||(j==8&&!(a->map_id==6&&!ally->ready))||((j>=2&&j<8)&&fighters[j]->knight_id==3))))continue;
            Fighter *q=fighters[j];
            int splash=p->attack_kind==HIT_SPLASH;
            int spin=p->attack_kind==HIT_SPIN;
            int throwing=p->attack_kind==HIT_THROW;
            int dx=q->x-p->x;
            if((!q->invulnerable||(p->attack_kind==HIT_STOMP&&q->pose==POSE_KNOCKDOWN))&&q->hp>0&&arena_melee_overlap(p,q)&&
               (p->z>=56||q->z>=56||!stage_segment_blocked(p->x,p->y,q->x,q->y))){
                p->hit_done=1;
                int damage=(p->attack_kind==HIT_HEAVY3||p->attack_kind==HIT_STOMP)?18:p->attack_kind==HIT_AIRHEAVY?22:p->attack_kind==HIT_HEAVY2?(p->animation>=5?16:12):spin?20:throwing?18:p->attack_kind==HIT_HEAVY?18:p->attack_kind==HIT_FINISH?26:p->attack_kind==HIT_LAUNCH?24:splash?24:15;
                int launch=p->pose==POSE_DASHPUNCH?4:p->pose==POSE_DASHSLAM?13:p->attack_kind==HIT_HEAVY3?-16:p->attack_kind==HIT_STOMP?5:p->attack_kind==HIT_AIRHEAVY?-16:p->attack_kind==HIT_HEAVY2?(p->z&&p->animation>=5?5:0):spin?6:throwing?10:p->attack_kind==HIT_LAUNCH?8:p->attack_kind==HIT_FINISH?5:p->attack_kind==HIT_AIR?(q->z?4:9):0;
                impacts[i][j].damage=damage;impacts[i][j].launch=launch;impacts[i][j].direction=dx?(dx<0?-1:1):p->facing;impacts[i][j].magic=splash;
                impacts[i][j].air=p->attack_kind==HIT_AIR||p->attack_kind==HIT_AIRHEAVY;
                impacts[i][j].unblockable=splash||throwing||p->attack_kind==HIT_STOMP;
                impacts[i][j].throwing=throwing;
                if(throwing)break;
            }
        }
        }

    }
    if(!a->clash_timer&&!a->clash_cooldown)for(int h=0;h<2;h++)for(int e=2;e<8;e++){
        Fighter*p=fighters[h],*q=fighters[e];
        if(!p->ready||!q->ready||q->knight_id==3||p->z||q->z||!impacts[h][e].damage||!impacts[e][h].damage||impacts[h][e].unblockable||impacts[e][h].unblockable)continue;
        if(p->attack_kind!=HIT_HEAVY&&q->attack_kind!=HIT_HEAVY&&p->attack_kind!=HIT_HEAVY2&&q->attack_kind!=HIT_HEAVY2&&p->attack_kind!=HIT_FINISH&&q->attack_kind!=HIT_FINISH)continue;
        a->clash_contacts=(a->clash_contacts+1)%3;if(a->clash_contacts)continue;
        a->clash_timer=90;a->clash_score[0]=a->clash_score[1]=0;
        Fighter*pair[2]={p,q};for(int n=0;n<2;n++){Fighter*v=pair[n];v->pose=POSE_CLASH;v->animation=v->cooldown=v->attack_kind=v->guard=v->knockback=v->queued=v->queue_timer=0;v->invulnerable=2;}
        for(int n=0;n<9;n++){impacts[h][n].damage=impacts[n][h].damage=impacts[e][n].damage=impacts[n][e].damage=0;}
        h=2;break;
    }
    for(int i=0;i<9;i++)for(int j=0;j<9;j++)if(impacts[i][j].damage){
        int raw=impacts[i][j].magic?3+((i<2?levels[i]:1)/10):impacts[i][j].damage;
        if(!impacts[i][j].magic){
            raw=impacts[i][j].throwing?10:raw>=24?10:raw>=18?6:4;
            if(i<2)raw=raw+levels[i]/10;
        }
        if(attributes&&i<2){
            unsigned strength=attributes[i].strength,level=levels[i];
            int kind=fighters[i]->attack_kind;
            if(impacts[i][j].magic)raw=(30+level+4*attributes[i].magic)/10;
            else if(kind==HIT_LIGHT||kind==HIT_AIR||kind==HIT_HEAVY2)raw=(30+level+10*strength)/10;
            else if(kind==HIT_HEAVY||kind==HIT_HEAVY3||kind==HIT_STOMP)raw=(500+10*level+115*strength)/100;
            else if(kind==HIT_FINISH)raw=(1200+10*level+125*strength)/100;
            else raw=(1000+10*level+120*strength)/100;
            unsigned divisor=attributes[i].critical_divisor;
            unsigned roll=a->tick*1664525u+fighters[i]->attack_seq*1013904223u+j*2246822519u;
            roll^=roll>>16;
            if(!impacts[i][j].magic&&divisor&&roll%divisor==0)raw*=4;
        }
        if(attributes&&j<2)raw=(raw*(120-attributes[j].defense)+50)/100;
        int accepted=arena_campaign_strike(fighters[i],fighters[j],raw,impacts[i][j].launch,impacts[i][j].direction,impacts[i][j].magic,impacts[i][j].unblockable);
        if(!accepted)continue;
        if(i<2)landed_hits[i]++;
        if(impacts[i][j].throwing)fighters[j]->knockback=impacts[i][j].direction*6;
        if(impacts[i][j].air&&fighters[i]->hp>0&&!fighters[i]->hitstun){
            if(fighters[i]->attack_kind==HIT_AIRHEAVY)fighters[i]->vz=7;
            else fighters[i]->float_timer=7;
        }
    }
    for(int s=0;s<2;s++){
        Projectile *p=&shots[s];if(!p->life)continue;
        if(a->map_id==6&&p->owner==8){
            /* Ram cannonball (source f_WarMachine guns): lobbed high above the screen, it comes down on the spot it was aimed at and
               bursts there, hurting every hero near the mark. life 48..9 is the flight (t 1..39), the blast follows for eight ticks. */
            p->life--;int t=48-p->life;
            if(t<40){p->x+=p->vx;p->z=(int16_t)(ARENA_CANNON_Z0*(40-t)/40+t*(40-t)/2);}
            else if(t==40){
                p->x=p->vz;p->z=0;p->vx=0;
                for(int h=0;h<2;h++){
                    Fighter *q=fighters[h];
                    if(!q->ready||q->hp<=0||q->z>30||abs(q->x-p->x)>ARENA_CANNON_RADIUS_X||abs(q->y-p->y)>ARENA_CANNON_RADIUS_Y)continue;
                    int damage=3;if(attributes)damage=(damage*(120-attributes[h].defense)+50)/100;if(damage<1)damage=1;
                    arena_campaign_strike(fighters[8],q,damage,5,q->x>=p->x?1:-1,0,0);
                }
            }
            continue;
        }
        int previous_x=p->x,previous_height=2*p->z+p->fraction;
        int downward=p->kind!=SHOT_ARROW&&p->vz==-9;
        /* Include the entire muzzle path on the creation tick. */
        if(p->life==48)p->x+=p->vx>0?16:-16;
        p->life--;
        int movement=downward?(p->vx>0?1:-1)*(p->life&1?6:7):p->vx;
        p->x+=movement;
        int height=previous_height;
        if(p->kind==SHOT_ARROW||downward){
            height+=p->vz;
            if(p->kind==SHOT_ARROW)p->vz--;
        }
        int floor_hit=height<0;
        if(floor_hit){
            /* Clip the collision segment at the floor rather than hitting past it. */
            p->x=previous_x+(p->x-previous_x)*previous_height/(previous_height-height);
            height=0;
        }
        p->z=height/2;p->fraction=height%2;
        int rock_from=previous_x,rock_to=p->x;
        if(previous_height>=112&&height<112)
            rock_from=previous_x+(p->x-previous_x)*(previous_height-112)/(previous_height-height);
        if(previous_height<112&&height>=112)
            rock_to=previous_x+(p->x-previous_x)*(112-previous_height)/(height-previous_height);
        if(p->x<0||p->x>=stage_world_width(a->map_id)||((height<112||previous_height<112)&&stage_projectile_blocked(rock_from,rock_to,p->y))){p->life=0;continue;}
        if(p->owner<0||p->owner>=9){p->life=0;continue;}
        for(int target=0;target<9;target++){
        if(!fighters[target]->ready||!fighters[target]->hp||((target<2||(target==8&&!(a->map_id==6&&!ally->ready))||((target>=2&&target<8)&&fighters[target]->knight_id==3))==(p->owner<2||(p->owner==8&&!(a->map_id==6&&!ally->ready))||((p->owner>=2&&p->owner<8)&&fighters[p->owner]->knight_id==3))))continue;
        Fighter *q=fighters[target];
        /* Clip a swept diagonal to the body's horizontal slab, then compare
           heights only on that interval; its bounding box alone overhits corners. */
        int direction=p->x>=previous_x?1:-1;
        int length=2*abs(p->x-previous_x);
        int enter=direction>0?2*q->x-45-2*previous_x:2*previous_x-(2*q->x+45);
        int leave=direction>0?2*q->x+45-2*previous_x:2*previous_x-(2*q->x-45);
        if(enter<0)enter=0;
        if(leave>length)leave=length;
        int touches=0;
        if(length&&enter<=leave){
            int first=previous_height*length+(height-previous_height)*enter;
            int last=previous_height*length+(height-previous_height)*leave;
            int low=first<last?first:last,high=first>last?first:last;
            touches=high>=2*q->z*length&&low<(2*q->z+120)*length;
        }else if(!length&&abs(p->x-q->x)<=22){
            int low=previous_height<height?previous_height:height;
            int high=previous_height>height?previous_height:height;
            touches=high>=2*q->z&&low<2*q->z+120;
        }
        if(touches&&abs(q->y-p->y)<=15){
            int damage=p->kind==SHOT_ARROW?(p->owner>=2&&p->owner<8?2:5):3+(p->owner<2?levels[p->owner]/10:0);
            if(attributes&&p->owner<2)damage=p->kind==SHOT_ARROW?2+attributes[p->owner].agility:(30+levels[p->owner]+4*attributes[p->owner].magic)/10;
            if(a->map_id==6&&p->owner==8)damage=3; /* Source ram ball attack_pow 10 vs thief arrow 6 (port 2). */
            if(attributes&&target<2)damage=(damage*(120-attributes[target].defense)+50)/100;
            int accepted=arena_campaign_strike(fighters[p->owner],q,damage,0,p->vx>0?1:-1,p->kind!=SHOT_ARROW,0);
            if(accepted&&p->owner<2)landed_hits[p->owner]++;
            p->life=0;break;
        }
        }
        if(floor_hit)p->life=0;
    }
}

void arena_group_step(Arena *a,Fighter enemies[6],Fighter *ally,const uint16_t input[9],const uint8_t targets[9],const uint8_t levels[2],uint8_t landed_hits[2]){
    arena_group_step_attributes(a,enemies,ally,input,targets,levels,0,landed_hits);
}
