#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_thieves.h"
#include "campaign_scene.h"
#include "campaign_war.h"
#include "campaign_thieves_scenes.h"
#include "campaign_breaks.h"
#include "pets.h"
#include "species.h"
#include "stage.h"
#include "../generated/thieves_geometry.h"
#include <stdlib.h>
#include <string.h>
/* LEVEL4 f_Main: each waypoint locks the screen (f_SetLeash), spawns its enemies and releases the lock when the encounter
   is cleared (kills_goal). Waypoints without enemies in this table are cutscene-only in the source (owl, bear, deer) and are
   not played. Positions are source offsets halved; spawns outside the legal floor snap to the nearest legal ground. */
enum {TS_THIEF,TS_VINE,TS_SCARED,TS_TROLL,TS_IMP,TS_MOTHER};
enum {REF_WP,REF_SCREEN,REF_ABS,REF_MARK};
typedef struct {int16_t dx,dy;uint8_t kind,ref;uint16_t delay;} TSpawn;
typedef struct {uint8_t wp;int8_t left_wp,right_wp;int16_t left_extra,right_extra;uint8_t count;const TSpawn *spawns;} TEvent;
/* WP1: the thief hiding in the bush. */
static const TSpawn ev_bush[]={{20,0,TS_THIEF,REF_WP,0}};
/* WP5: one hanging thief plus four walkers. */
static const TSpawn ev_five[]={{200,0,TS_VINE,REF_WP,0},{37,15,TS_THIEF,REF_SCREEN,0},{125,-15,TS_THIEF,REF_SCREEN,0},{-37,15,TS_THIEF,REF_SCREEN,0},{-100,-10,TS_THIEF,REF_SCREEN,0}};
/* WP6: four walkers and three thieves dropping from vines. */
static const TSpawn ev_six[]={{37,15,TS_THIEF,REF_SCREEN,0},{42,-15,TS_THIEF,REF_SCREEN,0},{-37,15,TS_THIEF,REF_SCREEN,0},{-100,-10,TS_THIEF,REF_SCREEN,0},
    {150,0,TS_VINE,REF_WP,0},{-100,20,TS_VINE,REF_WP,20},{160,-20,TS_VINE,REF_WP,80}};
/* WP9: six vine thieves, dropping 20 to 140 ticks apart. */
static const TSpawn ev_nine[]={{150,-15,TS_VINE,REF_WP,0},{200,20,TS_VINE,REF_WP,20},{175,-10,TS_VINE,REF_WP,50},{250,25,TS_VINE,REF_WP,80},{200,0,TS_VINE,REF_WP,120},{150,-20,TS_VINE,REF_WP,140}};
/* WP11..14: one troll each from trollspawn1..4. */
static const TSpawn ev_troll1[]={{THIEVES_TROLLSPAWN1_X,THIEVES_TROLLSPAWN1_Y,TS_TROLL,REF_ABS,0}};
static const TSpawn ev_troll2[]={{THIEVES_TROLLSPAWN2_X,THIEVES_TROLLSPAWN2_Y,TS_TROLL,REF_ABS,0}};
static const TSpawn ev_troll3[]={{THIEVES_TROLLSPAWN3_X,THIEVES_TROLLSPAWN3_Y,TS_TROLL,REF_ABS,0}};
static const TSpawn ev_troll4[]={{THIEVES_TROLLSPAWN4_X,THIEVES_TROLLSPAWN4_Y,TS_TROLL,REF_ABS,0}};
/* WP19: the imp. */
static const TSpawn ev_imp[]={{50,0,TS_IMP,REF_SCREEN,0}};
/* WP20: the three washing thieves of sprite 178. */
static const TSpawn ev_wash[]={{THIEVES_THIEFWASH_X+2,THIEVES_THIEFWASH_Y-10,TS_THIEF,REF_ABS,0},{THIEVES_THIEFWASH_X+200,THIEVES_THIEFWASH_Y-25,TS_THIEF,REF_ABS,0},{THIEVES_THIEFWASH_X+225,THIEVES_THIEFWASH_Y-30,TS_THIEF,REF_ABS,0}};
/* WP22: three thieves in a panic. */
static const TSpawn ev_panic[]={{37,-20,TS_SCARED,REF_SCREEN,0},{37,35,TS_SCARED,REF_SCREEN,0},{50,15,TS_SCARED,REF_SCREEN,0}};
/* WP23: the Troll Mother at WP25 with two fleeing thieves. */
static const TSpawn ev_mother[]={{0,25,TS_MOTHER,REF_MARK,0},{37,-10,TS_SCARED,REF_SCREEN,0},{37,35,TS_SCARED,REF_SCREEN,0}};
#define EV(wp,l,r,le,re,s) {wp,l,r,le,re,(uint8_t)(sizeof(s)/sizeof(s[0])),s}
static const TEvent events[THIEVES_EVENT_COUNT]={
    EV(1,0,1,0,0,ev_bush),EV(5,4,5,0,300,ev_five),EV(6,5,6,0,150,ev_six),EV(9,7,10,0,0,ev_nine),
    EV(11,10,11,0,0,ev_troll1),EV(12,11,12,0,0,ev_troll2),EV(13,12,13,0,0,ev_troll3),EV(14,11,15,0,0,ev_troll4),
    EV(19,18,20,0,0,ev_imp),EV(20,18,21,0,0,ev_wash),EV(22,21,23,0,0,ev_panic),EV(23,22,25,100,-75,ev_mother)};
#define WPX(i) (thieves_waypoints[i][0])
#define WPY(i) (thieves_waypoints[i][1])
static int living(const Campaign *c){int n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0&&c->enemies[i].knight_id!=3;return n;}
static int progress(const Arena *a){int x=0;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&a->p[h].x>x)x=a->p[h].x;return x;}
static int clear_of_heroes(const Arena *a,int x,int y){for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(x-a->p[h].x)<56&&abs(y-a->p[h].y)<28)return 0;return 1;}
static int place(Fighter *p,const Arena *a,int x,int y,int safe){
    for(int dx=0;dx<=224;dx+=16)for(int side=1;side>=-1;side-=2){
        int xx=x+side*dx;if(xx<0||xx>=THIEVES_WIDTH)continue;
        for(int dy=0;dy<160;dy++)for(int sy=-1;sy<=1;sy+=2){
            int yy=y+sy*dy;if(yy<0||yy>=448)continue;
            if(stage_position_open_map(a->map_id,xx,yy)&&(!safe||clear_of_heroes(a,xx,yy))){p->x=xx;p->y=yy;return 1;}
        }
    }
    return 0;
}
/* Returns the enemy slot plus one, or zero when none is free or no ground was found. */
static int spawn(Campaign *c,Arena *a,int x,int y,unsigned kind,int height){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    unsigned coop=a->p[1].ready;
    Fighter p={.hp=coop?60:50,.mana=40,.ready=1,.facing=-1,.invulnerable=8,.knight_id=1};
    uint8_t species=SPECIES_DEFAULT;
    if(kind==TS_TROLL){species=SPECIES_TROLL;}
    else if(kind==TS_IMP){species=SPECIES_IMP;}
    else if(kind==TS_MOTHER){species=SPECIES_TROLL_MOTHER;p.hp=THIEVES_MOTHER_HEALTH;}
    if(!place(&p,a,x,y,kind!=TS_MOTHER))return 0;
    if(height>80)height=80; /* Wire format limit. */
    if(height>0){p.z=height;p.vz=0;p.pose=POSE_AIRHURT;p.knockback=0;}
    c->enemies[i]=p;c->species[i]=species;c->targets[i]=0;c->combat.death_age[i]=0;
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1;
}
/* Cutscene clips (campaign_thieves_scenes.h): the intro at the start and the owl, deer and bear business at waypoints 2, 7 and 17.
   The clip number and frame ride in spawned/timer while WAR_TRAVEL, so guests draw them too. */
static unsigned scene_done;
static void scene_step(Campaign *c,const Arena *a){
    int s=thieves_scene_playing(c);
    if(s<0){
        int x=progress(a);
        for(int i=1;i<THIEVES_SCENE_COUNT;i++)
            if(!(scene_done&(1u<<i))&&x>=WPX(thieves_scenes[i].wp)&&x<WPX(thieves_scenes[i].wp)+200){scene_done|=1u<<i;c->spawned=(uint8_t)(i+1);c->timer=0;return;}
        return;
    }
    if(++c->timer>=thieves_scenes[s].release){c->spawned=0;c->timer=0;}
}
int campaign_thieves_waypoint_x(unsigned wp){return wp<27?WPX(wp):0;}
void campaign_thieves_leash(const Campaign *c,int *left,int *right){
    *left=54;*right=THIEVES_WIDTH-71;
    {int s=thieves_scene_playing(c);if(s>=0&&thieves_scenes[s].wp>=0){*right=WPX(thieves_scenes[s].wp+1);return;}}
    if(c->phase==WAR_FIGHT&&c->group<THIEVES_EVENT_COUNT){
        const TEvent *e=&events[c->group];
        if(e->left_wp>=0)*left=WPX(e->left_wp)+e->left_extra;
        if(e->right_wp>=0)*right=WPX(e->right_wp)+e->right_extra;
    }
}

/* Level pickups appear when the heroes come near and are withdrawn once passed, so they never starve the four loot slots:
   weapon30 at weapon_pt, animals 9 and 21 at trollpet and animal_pt (source f_Init/f_LevelItems), six coin bags. */
static const int16_t bags[6][2]={{THIEVES_GOLD1_X,THIEVES_GOLD1_Y},{THIEVES_GOLD2_X,THIEVES_GOLD2_Y},{THIEVES_GOLD10_X,THIEVES_GOLD10_Y},{THIEVES_GOLD11_X,THIEVES_GOLD11_Y},{THIEVES_GOLD12_X,THIEVES_GOLD12_Y},{THIEVES_GOLD13_X,THIEVES_GOLD13_Y}};
static const uint8_t bag_bits[6]={1,8,16,32,64,128}; /* story_flags bits 1 and 2 belong to the Keep. */
int campaign_thieves_bag(const Campaign *c,unsigned i,int *x,int *y){
    if(i>=6||(c->story_flags&bag_bits[i]))return 0;
    *x=bags[i][0];*y=bags[i][1];return 1;
}
static int owns_pet(const Campaign *c,unsigned id){for(int h=0;h<2;h++)if(pets_owned(&c->pets[h],id))return 1;return 0;}
static int has_drop(const Campaign *c,unsigned kind,unsigned value){for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind==kind&&(kind==DROP_WEAPON?c->drops[d].value==value:(c->drops[d].value&31)==value))return d+1;return 0;}
static void pickups(Campaign *c,Arena *a){
    int x=progress(a);
    struct {int x,y;unsigned kind,value;int owned;uint8_t bit;} item[3]={
        {THIEVES_TROLLPET_X,THIEVES_TROLLPET_Y,DROP_PET,PET_TROLL,owns_pet(c,PET_TROLL),1},
        {THIEVES_ANIMAL_PT_X,THIEVES_ANIMAL_PT_Y,DROP_PET,PET_DINO,owns_pet(c,PET_DINO),2},
        {THIEVES_WEAPON_PT_X,THIEVES_WEAPON_PT_Y,DROP_WEAPON,WEAPON_TROLL,weapon_set_has(&c->owned_weapons[0],WEAPON_TROLL)||weapon_set_has(&c->owned_weapons[1],WEAPON_TROLL),4}};
    for(int i=0;i<3;i++){
        int near=abs(x-item[i].x)<260,slot=has_drop(c,item[i].kind,item[i].value);
        if(item[i].owned){if(slot){c->drops[slot-1].kind=0;c->drop_age[slot-1]=0;}continue;}
        if(near&&!slot&&!(c->war.events&item[i].bit)){
            if(item[i].kind==DROP_PET)campaign_pet_place(c,item[i].value,item[i].x,item[i].y,1);else campaign_weapon_place(c,item[i].value,item[i].x,item[i].y);
        }
        if(slot&&!near&&abs(x-item[i].x)>600){c->drops[slot-1].kind=0;c->drop_age[slot-1]=0;}
    }
    for(unsigned i=0;i<6;i++){
        int bx,by;if(!campaign_thieves_bag(c,i,&bx,&by))continue;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(a->p[h].x-bx)<18&&abs(a->p[h].y-by)<14&&!a->p[h].z){
            c->story_flags|=bag_bits[i];unsigned gold=c->profiles[h].gold+5;c->profiles[h].gold=gold>65535?65535:gold;
            c->pickup_kind[h]=DROP_BAG;c->pickup_value[h]=5;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;break;
        }
    }
}
/* Intro (sprite 300, f_PlayersEnter): the carriers dash across and the knights run in behind them to their marks. */
static void intro_chase(Campaign *c,Arena *a){
    if(thieves_scene_playing(c)!=THIEVES_SCENE_INTRO||c->timer>=100)return;
    static const int16_t start[2][2]={{THIEVES_P1_START_X,THIEVES_P1_START_Y},{THIEVES_P2_START_X,THIEVES_P2_START_Y}};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready)continue;
        int back=(100-(int)c->timer)*2+h*18;
        p->x=start[h][0]-back;if(p->x<140)p->x=140;p->y=start[h][1];p->z=0;p->vz=0;p->facing=1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;p->invulnerable=2;p->cooldown=0;p->attack_kind=HIT_NONE;
    }
}
/* Ledges of the corridor (steep stretches of the floor profile, tools/thieves_shear.py): x range and the change of floor height going
   right (positive = down). Going down a ledge the hero drops off it; going up one needs a jump at least three quarters of its height. */
typedef struct {int16_t x0,x1,drop;} Ledge;
static const Ledge ledges[]={{1292,1303,24},{1342,1353,23},{2309,2320,-23},{2366,2377,-24},{3398,3409,30},{3976,3990,17},{4161,4173,20},{4810,4821,46}};
static int ledge_x[2];
int campaign_thieves_wall_ahead(int x){for(unsigned i=0;i<sizeof ledges/sizeof ledges[0];i++)if(ledges[i].drop<0&&x>=ledges[i].x0-16&&x<ledges[i].x0)return 1;return 0;}
static uint8_t carrying[6],hanging[6];
/* LEVEL4 river (x 2654..2838): wading slows everyone to 60 percent of their pace, as in the source f_Move ground-type check, except
   for a hero whose pet is the Dino (f_Move keeps speed_x for pet 21). */
static int river_x[2],river_y[2];
static void river_step(Campaign *c,Arena *a,const int old_x[6],const int old_y[6]){
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        int previous=river_x[h],previous_y=river_y[h];river_x[h]=p->x;river_y[h]=p->y;
        if(!previous||c->pets[h].equipped==PET_DINO||abs(p->x-previous)>8||p->z)continue;
        if(previous>=2654&&previous<=2838&&p->x>=2654&&p->x<=2838){int d=p->x-previous;if(abs(d)>=2){p->x=previous+d*6/10;stage_move(p,previous,previous_y);}}
    }
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];if(!p->ready||p->hp<=0||p->z||hanging[i])continue;
        int d=p->x-old_x[i];
        if(abs(d)>=2&&abs(d)<=8&&old_x[i]>=2654&&old_x[i]<=2838&&p->x>=2654&&p->x<=2838){p->x=old_x[i]+d*6/10;stage_move(p,old_x[i],old_y[i]);}
    }
}
static void ledge_step(Campaign *c,Arena *a){
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        int previous=ledge_x[h];ledge_x[h]=p->x;
        if(!previous||previous==p->x||abs(previous-p->x)>12||thieves_scene_playing(c)==THIEVES_SCENE_INTRO)continue; /* Larger steps are teleports (scripted moves, tests). */
        for(unsigned i=0;i<sizeof ledges/sizeof ledges[0];i++){
            const Ledge *l=&ledges[i];int right=previous<l->x0&&p->x>=l->x0,left=previous>l->x1&&p->x<=l->x1;
            if(!right&&!left)continue;
            int down=(right&&l->drop>0)||(left&&l->drop<0),height=abs(l->drop);
            if(down){p->z+=height;if(p->z>80)p->z=80;p->vz=0;}
            else if(p->z*4<height*3){p->x=previous;ledge_x[h]=previous;} /* Too high to climb on foot. */
        }
    }
}
static void keep_in_leash(Campaign *c,Arena *a){
    int left,right;campaign_thieves_leash(c,&left,&right);
    for(int h=0;h<2;h++)if(a->p[h].ready){
        if(a->p[h].x>right)place(&a->p[h],a,right,a->p[h].y,0);
        if(a->p[h].x<left)place(&a->p[h],a,left,a->p[h].y,0);
    }
}
/* f_EnemyRunAwayLeft: the panicked thieves bolt left for a moment before they regroup and fight. */
static void run_scared(Campaign *c,Arena *a,const int old_x[6],const int old_y[6]){
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(!(c->war.runner_mask&(1u<<i)))continue;
        if(!p->ready||p->hp<=0||c->war.runner_spawned>=200){c->war.runner_mask&=(uint8_t)~(1u<<i);continue;}
        if(p->z||p->hitstun||p->pose==POSE_KNOCKDOWN||p->pose==POSE_AIRHURT)continue;
        p->x=old_x[i]-3;p->y=old_y[i];stage_move(p,old_x[i],old_y[i]);p->facing=-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;p->cooldown=0;p->attack_kind=HIT_NONE;
    }
    if(c->war.runner_mask&&c->war.runner_spawned<255)c->war.runner_spawned++;else if(!c->war.runner_mask)c->war.runner_spawned=0;
    (void)a;
}
/* e_imp speed 8 (against 2..5 for the others): imps close the gap quickly before the shared AI takes over at melee range.
   f_Imp: an imp that sees a coin bag (item 8) lying on screen runs for it instead of the heroes, grabs it and bolts
   ("bagrun") until it leaves the screen, taking the bag with it; killing it first drops the bag again (f_ImpStealDeath). */
static void imps_dash(Campaign *c,Arena *a){
    int x=progress(a);
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(c->species[i]!=SPECIES_IMP){carrying[i]=0;continue;}
        if(carrying[i]&&(!p->ready||p->hp<=0)){campaign_bag_place(c,p->x,p->y);carrying[i]=0;continue;}
        if(!p->ready||p->hp<=0||p->z||p->hitstun||p->cooldown||p->pose==POSE_KNOCKDOWN)continue;
        int ox=p->x,oy=p->y;
        if(carrying[i]){
            const Fighter *t=&a->p[0];if(a->p[1].ready&&(!a->p[0].ready||abs(a->p[1].x-p->x)<abs(a->p[0].x-p->x)))t=&a->p[1];
            p->facing=t->x>p->x?-1:1;p->x+=p->facing*3;stage_move(p,ox,oy);p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
            if(abs(p->x-x)>300||p->x==ox){carrying[i]=0;p->ready=0;p->hp=0;}
            continue;
        }
        int bag=-1,bx=0,by=0;
        for(unsigned b=0;b<6;b++){int tx,ty;if(campaign_thieves_bag(c,b,&tx,&ty)&&abs(tx-p->x)<280&&abs(ty-p->y)<120){bag=(int)b;bx=tx;by=ty;break;}}
        if(bag>=0){
            if(abs(bx-p->x)<10&&abs(by-p->y)<8){c->story_flags|=bag_bits[bag];carrying[i]=1;continue;}
            p->x+=bx>p->x?3:-3;if(by>p->y)p->y+=1;else if(by<p->y)p->y-=1;p->facing=bx>ox?1:-1;
            stage_move(p,ox,oy);p->pose=POSE_WALK;p->animation=(p->animation+1)%16;continue;
        }
        const Fighter *t=0;int best=100000;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-p->x)+abs(a->p[h].y-p->y);if(d<best){best=d;t=&a->p[h];}}
        if(!t||best<70)continue;
        p->x+=t->x>p->x?2:-2;if(t->y>p->y+3)p->y+=1;else if(t->y<p->y-3)p->y-=1;
        stage_move(p,ox,oy);
    }
}
/* Source "vine" label: the thieves of WP5/6/9 dangle from the canopy, untouchable, until a hero comes near (or a while
   passes), and only then let go and drop. */
static void hang_step(Campaign *c,Arena *a,const int old_x[6],const int old_y[6]){
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];if(!hanging[i])continue;
        if(!p->ready||p->hp<=0){hanging[i]=0;continue;}
        int near=0;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(a->p[h].x-p->x)<90)near=1;
        if(--hanging[i]==0||near){hanging[i]=0;p->z=70;p->vz=0;continue;}
        p->x=old_x[i];p->y=old_y[i];p->z=70;p->vz=0;p->pose=POSE_JUMP;p->animation=0;p->invulnerable=2;p->cooldown=4;p->attack_kind=HIT_NONE;p->hitstun=0;
    }
}
/* Thieves that arrive from beyond the screen edge walk in: the shared fighting AI only engages near its target. */
static void march(Campaign *c,Arena *a){
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(!p->ready||p->hp<=0||p->z||p->hitstun||p->cooldown||p->pose==POSE_KNOCKDOWN||hanging[i]||carrying[i]||(c->war.runner_mask&(1u<<i)))continue;
        if(c->species[i]==SPECIES_IMP||c->species[i]==SPECIES_TROLL_MOTHER)continue;
        const Fighter *t=0;int best=100000;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-p->x)+abs(a->p[h].y-p->y);if(d<best){best=d;t=&a->p[h];}}
        static int rest[6],spot[6];
        if(p->x==spot[i]&&(p->pose==POSE_IDLE||p->pose==POSE_WALK)){if(++rest[i]>180&&t){place(p,a,t->x+(t->x>p->x?-40:40),t->y,1);rest[i]=0;}}else{rest[i]=0;spot[i]=p->x;}
        if(!t||best<170)continue;
        int ox=p->x,oy=p->y;p->x+=t->x>p->x?2:-2;if(t->y>p->y+3)p->y+=1;else if(t->y<p->y-3)p->y-=1;
        stage_move(p,ox,oy);p->facing=t->x>ox?1:-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
    }
}
static void spawn_one(Campaign *c,Arena *a,const TEvent *e,const TSpawn *s,int base_x){
    int x,y;
    switch(s->ref){
    case REF_WP:x=WPX(e->wp)+s->dx;y=WPY(e->wp)+s->dy;break;
    case REF_SCREEN:x=base_x+160+(s->dx>0?s->dx:0);y=WPY(e->wp)+s->dy;break; /* Always beyond the right edge of the screen, then they walk in. */
    case REF_MARK:x=WPX(25)+s->dx;y=WPY(25)+s->dy;break;
    default:x=s->dx;y=s->dy;break;
    }
    int height=s->kind==TS_VINE?70:s->kind==TS_TROLL?60:0;
    int slot=spawn(c,a,x,y,s->kind,height);
    if(slot&&s->kind==TS_SCARED)c->war.runner_mask|=(uint8_t)(1u<<(slot-1));
    if(slot&&s->kind==TS_VINE)hanging[slot-1]=120;
}
static void feed(Campaign *c,Arena *a){
    const TEvent *e=&events[c->group];
    while(c->spawned<e->count){
        const TSpawn *s=&e->spawns[c->spawned];
        if(c->timer<s->delay)break;
        int before=living(c);
        spawn_one(c,a,e,s,progress(a));
        if(living(c)==before)break; /* No free slot or ground yet; try again next tick. */
        c->spawned++;
    }
}
static int wave_done(const Campaign *c){
    return c->spawned>=events[c->group].count&&!living(c);
}
/* The mother (e_trollx): a baby troll is tossed out every 60 ticks while there is room (source spawn_limit18, held to the six
   enemy slots); each 20 points of damage she charges 20 ticks across the clearing at speed 14, untouchable; her death takes the brood with her. */
static void mother_step(Campaign *c,Arena *a){
    int m=-1;for(int i=0;i<6;i++)if(c->species[i]==SPECIES_TROLL_MOTHER&&c->enemies[i].ready)m=i;
    if(m<0)return;
    Fighter *q=&c->enemies[m];
    if(q->hp<=0){
        if(!(c->war.flags&1)){c->war.flags|=1;for(int i=0;i<6;i++)if(i!=m&&c->enemies[i].ready&&c->enemies[i].hp>0){c->enemies[i].hp=0;c->enemies[i].pose=POSE_AIRHURT;c->enemies[i].z=1;c->enemies[i].vz=8;}}
        return;
    }
    if(!(c->war.flags&2)){c->war.flags|=2;c->war.summon_timer=THIEVES_MOTHER_HEALTH-20;} /* health_cp */
    if(c->war.shot_timer){
        c->war.shot_timer--;q->invulnerable=2;
        int ox=q->x,oy=q->y;q->x+=c->war.target?-7:7;stage_move(q,ox,oy);q->pose=POSE_WALK;q->animation=(q->animation+1)%16;q->facing=c->war.target?-1:1;
        for(int h=0;h<2;h++){Fighter *p=&a->p[h];if(p->ready&&p->hp>0&&!p->z&&!p->invulnerable&&abs(p->x-q->x)<42&&abs(p->y-q->y)<26)arena_campaign_strike(q,p,10,6,q->facing,0,1);}
        return;
    }
    static int stand_x,stand_y;
    if(c->war.gate_hp){ /* Wind-up: she rears and a red mark shows over her head before the charge (gate_hp doubles as the wire's wind-up clock). */
        c->war.gate_hp--;q->invulnerable=2;q->pose=POSE_IDLE;q->animation=0;q->x=stand_x;q->y=stand_y;
        if(!c->war.gate_hp){
            c->war.shot_timer=20;
            int left=0,right=0;campaign_thieves_leash(c,&left,&right);
            c->war.target=abs(q->x-left)<abs(q->x-right)?0:1; /* Away from the nearer edge, as in the source. */
        }
        return;
    }
    if(q->hp<(int)c->war.summon_timer){
        c->war.summon_timer=c->war.summon_timer>20?c->war.summon_timer-20:0;c->war.gate_hp=24;q->invulnerable=24;stand_x=q->x;stand_y=q->y;
        return;
    }
    if(++c->war.mode_timer>=60){
        int slot=spawn(c,a,q->x,q->y,TS_TROLL,40);
        if(slot){c->war.mode_timer=0;q->invulnerable=8;}
    }
}
void campaign_thieves_start(Campaign *c,Arena *a){
    a->map_id=THIEVES_MAP;a->round=THIEVES_MAP;stage_set_map(THIEVES_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(c->species,0,sizeof(c->species));memset(&c->ally,0,sizeof(c->ally));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    campaign_break_reset(c);ledge_x[0]=ledge_x[1]=river_x[0]=river_x[1]=0;scene_done=1;memset(carrying,0,sizeof carrying);memset(hanging,0,sizeof hanging);
    c->war.events=0;c->group=0;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_TRAVEL;c->spawned=1;c->timer=0; /* The intro clip plays first. */
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));
    memset(c->owlet_state,0,sizeof(c->owlet_state));memset(c->owlet_timer,0,sizeof(c->owlet_timer));c->fruit_taken=0;c->mount_mask=0;
    memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));
    static const int16_t start[2][2]={{THIEVES_P1_START_X,THIEVES_P1_START_Y},{THIEVES_P2_START_X,THIEVES_P2_START_Y}};
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,start[h][0],start[h][1],0);
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};
    }
}
void campaign_thieves_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(THIEVES_MAP);
    if(c->phase==WAR_COMPLETE)return;
    if(c->phase==WAR_FIGHT)c->timer++;
    int old_x[6],old_y[6];for(int i=0;i<6;i++){old_x[i]=c->enemies[i].x;old_y[i]=c->enemies[i].y;}
    uint16_t idle[2]={0,0};
    campaign_combat_tick(c,a,thieves_scene_frozen(c)?idle:keys);
    intro_chase(c,a);
    campaign_break_step(c,a);
    river_step(c,a,old_x,old_y);
    ledge_step(c,a);
    hang_step(c,a,old_x,old_y);
    run_scared(c,a,old_x,old_y);
    imps_dash(c,a);
    march(c,a);
    keep_in_leash(c,a);
    pickups(c,a);
    int x=progress(a);
    if(c->phase==WAR_FIGHT){
        feed(c,a);
        if(c->group==THIEVES_EVENT_COUNT-1)mother_step(c,a);
        if(wave_done(c)){c->group++;c->phase=WAR_TRAVEL;c->timer=0;c->spawned=0;}
        return;
    }
    if(living(c))return;
    scene_step(c,a);
    if(thieves_scene_playing(c)>=0)return;
    if(c->group<THIEVES_EVENT_COUNT&&x>=WPX(events[c->group].wp)){c->phase=WAR_FIGHT;c->spawned=0;c->timer=0;}
    else if(c->group>=THIEVES_EVENT_COUNT&&x>=THIEVES_EXIT_DOOR_X){c->phase=WAR_COMPLETE;c->completed|=32;}
}
int campaign_thieves_boss_hp(const Campaign *c){
    if(c->phase!=WAR_FIGHT||c->group!=THIEVES_EVENT_COUNT-1)return 0;
    for(int i=0;i<6;i++)if(c->species[i]==SPECIES_TROLL_MOTHER&&c->enemies[i].ready&&c->enemies[i].hp>0)return c->enemies[i].hp;
    return 0;
}
const char *campaign_thieves_objective(const Campaign *c){
    if(c->phase==WAR_COMPLETE)return "Thieves' Forest complete";
    if(c->phase==WAR_FIGHT)return c->group==THIEVES_EVENT_COUNT-1?"Defeat the Troll Mother":"Defeat the thieves";
    if(c->group>=THIEVES_EVENT_COUNT)return "Walk into the cabin";
    return "Follow the forest path";
}
