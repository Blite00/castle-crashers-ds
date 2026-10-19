#pragma GCC optimize("Os") /* Once-per-tick encounter logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_flood.h"
#include "stage.h"
#include "campaign_scene.h"
#include <stdlib.h>
#include <string.h>

/* LEVEL23 sprite 540 timeline (frame -> event); the numbers are source frames, one per tick. */
enum {EV_BAT_R,EV_BAT_L,EV_FISH,EV_FOOD,EV_WAIT,EV_BOSS};
typedef struct {uint16_t at;uint8_t kind,arg;} FloodEvent;
static const FloodEvent timeline[]={
    {57,EV_FOOD,0},{130,EV_FOOD,1},{440,EV_BAT_R,0},{458,EV_BAT_R,0},{473,EV_BAT_R,0},{488,EV_BAT_R,0},{514,EV_FOOD,2},{538,EV_BAT_R,0},
    {620,EV_BAT_L,0},{649,EV_BAT_R,0},{698,EV_BAT_R,0},{744,EV_FOOD,3},{752,EV_FISH,0},{755,EV_WAIT,0},{764,EV_BAT_R,0},{766,EV_FOOD,4},
    {769,EV_BAT_L,0},{777,EV_FISH,1},{780,EV_WAIT,0},{852,EV_BAT_R,0},{862,EV_BAT_R,0},{873,EV_BAT_L,0},{884,EV_BAT_R,0},{915,EV_FOOD,5},
    {945,EV_BAT_R,0},{995,EV_BAT_L,0},{1002,EV_BAT_R,0},{1009,EV_BAT_R,0},{1018,EV_BAT_R,0},{1079,EV_BAT_R,0},{1084,EV_FOOD,6},
    {1094,EV_FISH,0},{1097,EV_WAIT,0},{1103,EV_FOOD,7},{1109,EV_BAT_R,0},{1123,EV_BAT_R,0},{1212,EV_BAT_R,0},{1219,EV_BAT_R,0},
    {1226,EV_BAT_R,0},{1232,EV_BAT_R,0},{1239,EV_FOOD,8},{1342,EV_BAT_R,0},{1349,EV_BAT_R,0},{1356,EV_BAT_R,0},{1370,EV_BAT_L,0},
    {1374,EV_FOOD,9},{1384,EV_FISH,1},{1389,EV_FISH,0},{1393,EV_WAIT,0},{1407,EV_FOOD,10},{1482,EV_FOOD,11},{1751,EV_BOSS,0}};
#define TIMELINE_COUNT (sizeof timeline/sizeof *timeline)
/* Shark fins: spawn tick; singles, a pair, three, then the diagonal sets (sprite 540 frames 366..1305). */
static const uint16_t fin_at[]={366,571,571,659,659,659,1145,1165,1185,1205,1246,1266,1286,1305};
#define FIN_COUNT (sizeof fin_at/sizeof *fin_at)
#define ROCK_FIRST 30
static unsigned mix(unsigned v){v=v*2654435761u+0x9e3779b9u;v^=v>>15;v*=2246822519u;v^=v>>13;return v;}
static int lane(unsigned k,int lo,int hi){return lo+(int)(mix(k)%(unsigned)(hi-lo+1));}
unsigned campaign_flood_rocks(unsigned timer,FloodObject out[FLOOD_ROCK_LIMIT]){
    unsigned n=0,t=ROCK_FIRST;
    for(unsigned k=0;t<=timer;k++){
        int x=FLOOD_ROCK_SPAWN_X-FLOOD_ROCK_SPEED*(int)(timer-t);
        if(x>-40&&n<FLOOD_ROCK_LIMIT){out[n].x=(int16_t)x;out[n].y=(int16_t)lane(k,200,296);n++;}
        t+=30+mix(k+7)%90;
    }
    return n;
}
unsigned campaign_flood_fins(unsigned timer,FloodObject out[FLOOD_FIN_LIMIT]){
    unsigned n=0;
    for(unsigned k=0;k<FIN_COUNT&&n<FLOOD_FIN_LIMIT;k++){
        if(timer<fin_at[k])continue;
        int x=FLOOD_ROCK_SPAWN_X+10-FLOOD_FIN_SPEED*(int)(timer-fin_at[k]);
        if(x<40)continue;
        out[n].x=(int16_t)x;out[n].y=(int16_t)lane(k+100,205,295);n++;
    }
    return n;
}
static int living(const Campaign *c){int n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static int place(Fighter *p,const Arena *a,int x,int y){
    for(int dx=0;dx<=224;dx+=16)for(int side=1;side>=-1;side-=2){
        int xx=x+side*dx;if(xx<0||xx>=FLOOD_WIDTH)continue;
        for(int dy=0;dy<100;dy++)for(int sy=-1;sy<=1;sy+=2){
            int yy=y+sy*dy;if(yy<0||yy>=448)continue;
            if(stage_position_open_map(a->map_id,xx,yy)){p->x=xx;p->y=yy;return 1;}
        }
    }
    return 0;
}
static int spawn(Campaign *c,Arena *a,int x,int y,unsigned kind,int hp){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    Fighter p={.hp=hp,.mana=40,.ready=1,.facing=-1,.invulnerable=8,.knight_id=kind};
    if(!place(&p,a,x,y))return 0;
    c->enemies[i]=p;c->targets[i]=0;c->combat.death_age[i]=0;
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1;
}
void campaign_flood_start(Campaign *c,Arena *a){
    a->map_id=FLOOD_MAP;a->round=FLOOD_MAP;stage_set_map(FLOOD_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=0;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_FIGHT;
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));
    memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));c->mount_mask=0;
    static const int16_t start[2][2]={{FLOOD_P1_START_X,FLOOD_P1_START_Y},{FLOOD_P2_START_X,FLOOD_P2_START_Y}};
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,start[h][0],start[h][1]);
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=-1};
    }
}
static void hazards(Campaign *c,Arena *a){
    FloodObject rock[FLOOD_ROCK_LIMIT],fin[FLOOD_FIN_LIMIT];
    unsigned nr=campaign_flood_rocks(c->timer,rock),nf=campaign_flood_fins(c->timer,fin);
    Fighter hazard={.facing=-1};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        for(unsigned i=0;i<nr;i++)if(p->x<rock[i].x&&rock[i].x-p->x<=FLOOD_ROCK_SPEED&&abs(p->y-rock[i].y)<10)arena_campaign_strike(&hazard,p,5,4,-1,0,0);
        for(unsigned i=0;i<nf;i++)if(p->x<fin[i].x&&fin[i].x-p->x<=FLOOD_FIN_SPEED&&abs(p->y-fin[i].y)<8)arena_campaign_strike(&hazard,p,8,6,-1,0,0);
    }
}
static void march(Campaign *c,Arena *a){
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(!p->ready||p->hp<=0||p->z||p->hitstun||p->cooldown||p->pose==POSE_KNOCKDOWN)continue;
        const Fighter *t=0;int best=100000;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-p->x)+abs(a->p[h].y-p->y);if(d<best){best=d;t=&a->p[h];}}
        if(!t||best<170)continue;
        int ox=p->x,oy=p->y;p->x+=t->x>p->x?2:-2;if(t->y>p->y+3)p->y+=1;else if(t->y<p->y-3)p->y-=1;
        stage_move(p,ox,oy);p->facing=t->x>ox?1:-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
    }
}
static void run_timeline(Campaign *c,Arena *a){
    uint16_t *cur=&c->break_tick;
    while(c->npc_ticks<TIMELINE_COUNT){
        const FloodEvent *e=&timeline[c->npc_ticks];
        if(*cur<e->at)break;
        if(e->kind==EV_WAIT){if(living(c))return;}
        else if(e->kind==EV_FOOD){
            Fighter f={0};if(place(&f,a,FLOOD_P1_START_X+((int)e->arg*47)%260,220+((int)e->arg*13)%60))campaign_queue_drop(c,f.x,f.y,DROP_FOOD,10|((e->arg%5)<<5));
        }else if(e->kind==EV_BOSS){
            if(living(c))return;
            if(spawn(c,a,FLOOD_BOSS_X,FLOOD_BOSS_Y,1,a->p[1].ready?900:600)){c->group=1;}else return;
        }else{
            int x=e->kind==EV_BAT_L?120:FLOOD_E_SPAWN_X,y=200+(int)((e->at*7)%90);
            if(!spawn(c,a,x,y,e->kind==EV_FISH?2:0,e->kind==EV_FISH?60:50))return;
        }
        c->npc_ticks++;
    }
    (*cur)++;
}
void campaign_flood_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(FLOOD_MAP);
    if(c->phase==WAR_COMPLETE)return;
    c->timer++;
    run_timeline(c,a);
    uint16_t k2[2]={keys[0],keys[1]};
    int boss_hp[6],open[6];for(int i=0;i<6;i++){boss_hp[i]=c->enemies[i].hp;open[i]=c->enemies[i].cooldown>0||c->enemies[i].hitstun>0;}
    campaign_combat_tick(c,a,k2);
    /* Source Catfish: damage lands in full only while it is stunned; otherwise it shrugs off three quarters. */
    for(int i=0;i<6;i++){Fighter *e=&c->enemies[i];
        if(e->ready&&e->knight_id==1&&e->hp>0&&e->hp<boss_hp[i]&&!open[i])e->hp+=(int16_t)((boss_hp[i]-e->hp)*3/4);}
    hazards(c,a);
    march(c,a);
    if(c->group==1&&c->npc_ticks>=TIMELINE_COUNT&&!living(c)){c->phase=WAR_COMPLETE;c->completed|=128;}
}
const char *campaign_flood_objective(const Campaign *c){
    if(c->phase==WAR_COMPLETE)return "Flooded Castle complete";
    if(c->group==1)return "Defeat the Catfish";
    return "Swim for the castle";
}
