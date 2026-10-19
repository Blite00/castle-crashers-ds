#pragma GCC optimize("Os") /* Once-per-tick encounter logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_forest.h"
#include "stage.h"
#include "campaign_scene.h"
#include "campaign_breaks.h"
#include <stdlib.h>
#include <string.h>

/* LEVEL18 f_Main/f_Cinema converted to DS world units (source/2 plus origin). Waypoints WP0..WP8:
   (183,234) (362,232) (628,238) (1011,239) (1050,247) (1373,296) (1789,293) (2147,298) (2192,288). */
enum {K_BARB=0,K_THIEF=1,K_BEEFY=2,K_AMBUSH=3};
typedef struct {int16_t x,y;uint8_t kind,coop;uint16_t delay;} ForestSpawn;
/* Wave 1: croc-riding thief and two thieves (WP1); two-player adds two thieves. */
static const ForestSpawn wave_croc[]={
    {778,232,K_THIEF,0,0},{678,207,K_THIEF,0,0},{728,277,K_THIEF,0,0},{878,257,K_THIEF,1,0},{903,207,K_THIEF,1,0}};
/* After the cutscene: the beefy thief (200 HP) and the two-player reinforcement thieves. */
static const ForestSpawn wave_ambush[]={
    {748,238,K_BEEFY,0,0},{961,219,K_THIEF,1,0},{936,284,K_THIEF,1,0}};
/* WP5: six thieves plus four archers that arrive from the right. */
static const ForestSpawn wave_archers[]={
    {1814,273,K_THIEF,0,0},{1789,338,K_THIEF,0,0},{1789,248,K_THIEF,0,0},{1700,260,K_THIEF,0,60},{1650,300,K_THIEF,0,90},{1864,308,K_THIEF,0,0},
    {1960,250,K_THIEF,0,120},{1990,285,K_THIEF,0,150},{1975,320,K_THIEF,0,180},{2010,265,K_THIEF,0,210}};
/* WP6: two thieves at WP7, then staggered arrivals at WP8; two players add three more. */
static const ForestSpawn wave_final[]={
    {2147,288,K_THIEF,0,0},{2147,308,K_THIEF,0,0},{2192,288,K_THIEF,0,0},{2192,288,K_THIEF,0,60},{2192,288,K_THIEF,0,120},{2192,288,K_THIEF,0,180},
    {1739,260,K_THIEF,1,150},{1739,292,K_THIEF,1,150},{1739,324,K_THIEF,1,150}};
/* Cutscene ambush: barbarians drop from the trees at ambush1..5 (x,tree y, delay) onto the road at y194. */
static const int16_t ambush_trees[5][3]={{642,91,10},{890,121,30},{550,125,50},{816,97,70},{733,84,80}};
static const int16_t cinema_stops[2][2]={{570,225},{555,242}};
static const int16_t wp_x[9]={183,362,628,1011,1050,1373,1789,2147,2192};

static int living(const Campaign *c){int n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static int progress(const Arena *a){int x=0;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&a->p[h].x>x)x=a->p[h].x;return x;}
static int clear_of_heroes(const Arena *a,int x,int y){for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(x-a->p[h].x)<56&&abs(y-a->p[h].y)<28)return 0;return 1;}
static int place(Fighter *p,const Arena *a,int x,int y,int safe){
    for(int dx=0;dx<=224;dx+=16)for(int side=1;side>=-1;side-=2){
        int xx=x+side*dx;if(xx<0||xx>=FOREST_WIDTH)continue;
        for(int dy=0;dy<160;dy++)for(int sy=-1;sy<=1;sy+=2){
            int yy=y+sy*dy;if(yy<0||yy>=448)continue;
            if(stage_position_open_map(a->map_id,xx,yy)&&(!safe||clear_of_heroes(a,xx,yy))){p->x=xx;p->y=yy;return 1;}
        }
    }
    return 0;
}
static int spawn(Campaign *c,Arena *a,int x,int y,unsigned kind,int height){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    Fighter p={.hp=a->p[1].ready?60:50,.mana=40,.ready=1,.facing=-1,.invulnerable=8,.knight_id=kind==K_BARB||kind==K_AMBUSH?0:1};
    if(kind==K_BEEFY)p.hp=200;
    if(!place(&p,a,x,y,1))return 0;
    if(height>80)height=80; /* Wire format limit; the drop still starts above head height. */
    if(height>0){p.z=height;p.vz=0;p.pose=POSE_AIRHURT;p.knockback=0;}
    c->enemies[i]=p;c->targets[i]=0;c->combat.death_age[i]=0;
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1; /* Slot plus one. */
}
static const ForestSpawn *table(unsigned wave,unsigned *count){
    switch(wave){
    case FOREST_WAVE_CROC:*count=sizeof wave_croc/sizeof *wave_croc;return wave_croc;
    case FOREST_WAVE_AMBUSH:*count=sizeof wave_ambush/sizeof *wave_ambush;return wave_ambush;
    case FOREST_WAVE_ARCHERS:*count=sizeof wave_archers/sizeof *wave_archers;return wave_archers;
    default:*count=sizeof wave_final/sizeof *wave_final;return wave_final;
    }
}
/* c->spawned counts entries already placed; entries skipped for two players still advance it. */
static void feed(Campaign *c,Arena *a){
    unsigned count;const ForestSpawn *t=table(c->group,&count);
    while(c->spawned<count){
        const ForestSpawn *s=&t[c->spawned];
        if(s->coop&&!a->p[1].ready){c->spawned++;continue;}
        int slot=c->timer<s->delay?0:spawn(c,a,s->x,s->y,s->kind,0);
        if(!slot)break;
        if(c->group==FOREST_WAVE_CROC&&c->spawned==0)c->mount_mask|=1u<<(slot-1); /* Source f_MakeMount2 under the first thief. */
        c->spawned++;
    }
}
/* The ambush clips keep running after the cutscene ends: runner_mask records which of the five have landed. */
static void ambush_feed(Campaign *c,Arena *a,unsigned clock){
    for(unsigned i=0;i<5;i++)if(!(c->war.runner_mask&(1u<<i))&&clock>=75u+(unsigned)ambush_trees[i][2]+36u){
        /* Barbarians land on the road at ambush_y (194); the fall is their tree height above it. */
        if(spawn(c,a,ambush_trees[i][0],194,K_AMBUSH,194-ambush_trees[i][1]))c->war.runner_mask|=1u<<i;
    }
}
static int wave_done(const Campaign *c,const Arena *a){
    if(c->group==FOREST_WAVE_AMBUSH&&c->war.runner_mask!=31)return 0;
    unsigned count;const ForestSpawn *t=table(c->group,&count);
    for(unsigned i=c->spawned;i<count;i++)if(!t[i].coop||a->p[1].ready)return 0;
    return !living(c);
}
static void begin(Campaign *c,unsigned wave){c->group=wave;c->spawned=0;c->timer=0;c->phase=WAR_FIGHT;}
void campaign_forest_start(Campaign *c,Arena *a){
    a->map_id=FOREST_MAP;a->round=FOREST_MAP;stage_set_map(FOREST_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=FOREST_WAVE_CROC;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_TRAVEL;
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));
    memset(c->owlet_state,0,sizeof(c->owlet_state));memset(c->owlet_timer,0,sizeof(c->owlet_timer));c->fruit_taken=0;c->mount_mask=0;campaign_break_reset(c);
    memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));
    static const int16_t start[2][2]={{176,243},{156,258}};
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,start[h][0],start[h][1],0);
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};
    }
    if(!weapon_set_has(&c->owned_weapons[0],WEAPON_LEAF)&&!weapon_set_has(&c->owned_weapons[1],WEAPON_LEAF))campaign_weapon_place(c,WEAPON_LEAF,1717,265);
    /* Source animal orb 2 (Owlet) waits at the road's far end until someone has taken it. */
    if(!pets_owned(&c->pets[0],PET_OWLET)&&!pets_owned(&c->pets[1],PET_OWLET))campaign_pet_place(c,PET_OWLET,2026,253,1);
}
int campaign_forest_dig(Campaign *c,const Arena *a,unsigned hero){
    if(hero>=2||a->map_id!=FOREST_MAP||!(c->tools[hero]&1)||!a->p[hero].ready||!a->p[hero].hp||abs(a->p[hero].x-FOREST_DIG_X)>=25||abs(a->p[hero].y-FOREST_DIG_Y)>=10)return 0;
    if(weapon_set_has(&c->owned_weapons[0],WEAPON_THIEF)||weapon_set_has(&c->owned_weapons[1],WEAPON_THIEF))return 0;
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind==DROP_WEAPON&&c->drops[d].value==WEAPON_THIEF)return 0;
    return campaign_weapon_place(c,WEAPON_THIEF,FOREST_DIG_X,FOREST_DIG_Y);
}
/* Thieves that arrive from beyond the screen edge walk in on their own: the shared fighting AI only engages near its target. */
static void march(Campaign *c,Arena *a){
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(!p->ready||p->hp<=0||p->z||p->hitstun||p->cooldown||p->pose==POSE_KNOCKDOWN||c->phase==WAR_FOREST_CINEMA)continue;
        const Fighter *t=0;int best=100000;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-p->x)+abs(a->p[h].y-p->y);if(d<best){best=d;t=&a->p[h];}}
        if(!t||best<170)continue;
        int ox=p->x,oy=p->y;p->x+=t->x>p->x?2:-2;if(t->y>p->y+3)p->y+=1;else if(t->y<p->y-3)p->y-=1;
        stage_move(p,ox,oy);p->facing=t->x>ox?1:-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
    }
}
static void keep_in_leash(Campaign *c,Arena *a){
    int left,right;campaign_forest_leash(c,&left,&right);
    for(int h=0;h<2;h++)if(a->p[h].ready){
        if(a->p[h].x>right)place(&a->p[h],a,right,a->p[h].y,0);
        if(a->p[h].x<left)place(&a->p[h],a,left,a->p[h].y,0);
    }
}
/* Source sprite234 (cinema): heroes walk to their p*_4 marks, barbarians drop from the trees at frame 75
   plus each ambush delay, release at 163, end at 165. Combat runs with the heroes held and protected. */
static void cinema(Campaign *c,Arena *a){
    uint16_t none[2]={0,0};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        p->invulnerable=2;
        int dx=cinema_stops[h][0]-p->x,dy=cinema_stops[h][1]-p->y,x=p->x,y=p->y;
        if(abs(dx)>1||abs(dy)>1){p->x+=dx>2?2:dx<-2?-2:0;p->y+=dy>2?2:dy<-2?-2:0;stage_move(p,x,y);p->pose=POSE_WALK;p->facing=dx<0?-1:1;p->animation=(p->animation+1)%16;}
        else{p->pose=POSE_IDLE;p->animation=0;p->facing=1;}
    }
    campaign_combat_tick(c,a,none);
    ambush_feed(c,a,c->timer);
    if(++c->timer>=FOREST_CINEMA_TICKS){begin(c,FOREST_WAVE_AMBUSH);}
}
void campaign_forest_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(FOREST_MAP);
    for(int i=0;i<6;i++)if(!c->enemies[i].ready||c->enemies[i].hp<=0){
        if(c->mount_mask&(1u<<i)&&c->enemies[i].ready)campaign_croc_place(c,c->enemies[i].x,c->enemies[i].y,c->enemies[i].facing); /* The mount outlives its rider. */
        c->mount_mask&=(uint8_t)~(1u<<i);
    }
    if(c->phase==WAR_COMPLETE)return;
    if(c->phase==WAR_FOREST_CINEMA){cinema(c,a);return;}
    if(c->phase==WAR_FIGHT)c->timer++;
    uint16_t k2[2]={keys[0],keys[1]};campaign_mount_step(c,a,k2);
    campaign_combat_tick(c,a,k2);
    campaign_mount_arc(c,a);
    campaign_break_step(c,a);
    march(c,a);
    keep_in_leash(c,a);
    int x=progress(a);
    if(c->phase==WAR_FIGHT){
        feed(c,a);
        if(c->group==FOREST_WAVE_AMBUSH)ambush_feed(c,a,FOREST_CINEMA_TICKS+c->timer);
        if(wave_done(c,a)){
            if(c->group==FOREST_WAVE_CROC){c->phase=WAR_FOREST_CINEMA;c->timer=0;c->spawned=0;}
            else{c->group++;c->phase=WAR_TRAVEL;c->timer=0;c->spawned=0;}
        }
        return;
    }
    if(living(c))return;
    if(c->group==FOREST_WAVE_CROC&&x>=wp_x[1])begin(c,FOREST_WAVE_CROC);
    else if(c->group==FOREST_WAVE_ARCHERS&&x>=wp_x[5])begin(c,FOREST_WAVE_ARCHERS);
    else if(c->group==FOREST_WAVE_FINAL&&x>=wp_x[6])begin(c,FOREST_WAVE_FINAL);
    else if(c->group==FOREST_WAVE_EXIT&&x>=FOREST_EXIT_X){c->phase=WAR_COMPLETE;c->completed|=16;}
}
const char *campaign_forest_objective(const Campaign *c){
    if(c->phase==WAR_FOREST_CINEMA)return "Thieves ambush the road";
    if(c->phase==WAR_FIGHT)return "Defeat the thieves";
    if(c->phase==WAR_COMPLETE)return "Forest Entrance complete";
    return c->group==FOREST_WAVE_EXIT?"Walk into the bushes":"Continue down the forest road";
}
