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
    {769,EV_BAT_L,0},{777,EV_FISH,0},{780,EV_WAIT,0},{852,EV_BAT_R,0},{862,EV_BAT_R,0},{873,EV_BAT_L,0},{884,EV_BAT_R,0},{915,EV_FOOD,5},
    {945,EV_BAT_R,0},{995,EV_BAT_L,0},{1002,EV_BAT_R,0},{1009,EV_BAT_R,0},{1018,EV_BAT_R,0},{1079,EV_BAT_R,0},{1084,EV_FOOD,6},
    {1094,EV_FISH,0},{1097,EV_WAIT,0},{1103,EV_FOOD,7},{1109,EV_BAT_R,0},{1123,EV_BAT_R,0},{1212,EV_BAT_R,0},{1219,EV_BAT_R,0},
    {1226,EV_BAT_R,0},{1232,EV_BAT_R,0},{1239,EV_FOOD,8},{1342,EV_BAT_R,0},{1349,EV_BAT_R,0},{1356,EV_BAT_R,0},{1370,EV_BAT_L,0},
    {1374,EV_FOOD,9},{1384,EV_FISH,0},{1389,EV_FISH,1},{1393,EV_WAIT,0},{1407,EV_FOOD,10},{1482,EV_FOOD,11},{1751,EV_BOSS,0}};
#define TIMELINE_COUNT (sizeof timeline/sizeof *timeline)

/* Shark fins (f_SpawnSharkFin calls in sprite 540): spawn frame, x offset and y offset from the rock spawner in source units. A fin spawned left
   of the screen swims right. It slides in at 40 source units per tick losing two each tick, waits ten ticks, then accelerates by one unit per tick. */
typedef struct {uint16_t at;int16_t dx,dy;} FinSpawn;
static const FinSpawn fin_spawn[]={
    {366,0,75},{571,0,15},{571,25,140},{659,-1400,5},{659,-1425,150},{659,0,75},{1145,0,5},{1160,20,35},{1175,40,65},{1190,60,95},{1205,80,125},
    {1246,-1500,175},{1260,-1480,145},{1275,-1460,115},{1290,-1440,75},{1305,-1420,45}};
#define FIN_COUNT (sizeof fin_spawn/sizeof *fin_spawn)
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
/* Source distance (units) covered by a fin of the given age; DS pixels are source/2 scaled by 17/25 like every chase speed. */
static int fin_distance(unsigned age){
    int s=0;
    for(unsigned j=0;j<age&&j<20;j++)s+=40-2*(int)j;
    if(age>30){unsigned n=age-30;s+=(int)(n*(n-1)/2);}
    return s*34/100;
}
unsigned campaign_flood_fins(unsigned clock,FloodObject out[FLOOD_FIN_LIMIT]){
    unsigned n=0;
    for(unsigned k=0;k<FIN_COUNT&&n<FLOOD_FIN_LIMIT;k++){
        if(clock<fin_spawn[k].at)continue;
        int x0=FLOOD_ROCK_SPAWN_X+fin_spawn[k].dx/2,right=fin_spawn[k].dx<-100;
        int x=right?x0+fin_distance(clock-fin_spawn[k].at):x0-fin_distance(clock-fin_spawn[k].at);
        if(x<24||x>FLOOD_WIDTH-24)continue;
        out[n].x=(int16_t)x;out[n].y=(int16_t)(FLOOD_ROCK_SPAWN_Y+fin_spawn[k].dy/2);n++;
    }
    return n;
}
/* The king boat (sprite 433) drifts right until it is 70 source units inside the left edge of the screen and stays there. */
int campaign_flood_boat_x(const Arena *a){
    int cx=(a->p[0].x+(a->p[1].ready?a->p[1].x:a->p[0].x))/2,x=cx-144+35;
    return x<FLOOD_KINGBOAT_X?FLOOD_KINGBOAT_X:x;
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
static int spawn(Campaign *c,Arena *a,int x,int y,unsigned species,int hp){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    Fighter p={.hp=hp,.mana=40,.ready=1,.facing=-1,.invulnerable=8,.knight_id=2}; /* knight_id 2 keeps the shared enemy AI off: the fish, bats and Catfish below think for themselves */
    if(!place(&p,a,x,y))return 0;
    if(species==FLOOD_BAT)p.z=30;
    c->enemies[i]=p;c->species[i]=(uint8_t)species;c->targets[i]=0;c->combat.death_age[i]=0;
    c->flood.mode[i]=c->flood.timer[i]=c->flood.hits[i]=c->flood.aux[i]=0;c->flood.seen[i]=0;c->flood.tx[i]=c->flood.ty[i]=0;
    if(species==FLOOD_CAT){c->flood.mode[i]=CAT_WAIT;c->flood.timer[i]=60;}
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1;
}
static void logs_start(Campaign *c);
void campaign_flood_start(Campaign *c,Arena *a){
    a->map_id=FLOOD_MAP;a->round=FLOOD_MAP;stage_set_map(FLOOD_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));memset(&c->flood,0,sizeof(c->flood));memset(c->species,0,sizeof(c->species));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=0;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_FIGHT;
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));
    memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));c->mount_mask=0;memset(c->mount_cool,0,sizeof(c->mount_cool));memset(c->mount_arc,0,sizeof(c->mount_arc));
    static const int16_t start[2][2]={{FLOOD_P1_START_X,FLOOD_P1_START_Y},{FLOOD_P2_START_X,FLOOD_P2_START_Y}};
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,start[h][0],start[h][1]);
        /* Source f_ActivatePlayers: the knights drop in from above (body_y -500) and land swimming. */
        a->p[h].z=80;a->p[h].vz=0;a->p[h].pose=POSE_JUMP;
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=-1};
    }
    logs_start(c);
}
/* ---- logs (f_LogDrift, f_LogWaitCheck): four logs in the four lanes drift left; a jump near one (campaign_mount_step) puts a swimmer on it. They ride as
   crocodile drops so their positions travel with the snapshot. */
static const int16_t log_lane[4][2]={{660,224},{700,247},{640,272},{720,297}};
static void logs_start(Campaign *c){for(int k=0;k<4;k++)campaign_croc_place(c,log_lane[k][0],log_lane[k][1],-1);}
static void logs(Campaign *c,const Arena *a){
    int cx=(a->p[0].x+(a->p[1].ready?a->p[1].x:a->p[0].x))/2,left=cx-144-60;
    if(left<86)left=86;
    for(int d=0;d<CAMPAIGN_DROPS;d++){
        CampaignDrop *item=&c->drops[d];if(item->kind!=DROP_CROC)continue;
        item->x=(int16_t)(item->x-(c->timer%5<2?2:1)); /* 4 source units per tick, scaled */
        if(item->x<left){int x=cx+144+(int)(mix(c->timer+(unsigned)d)%40);item->x=(int16_t)(x>FLOOD_WIDTH-100?FLOOD_WIDTH-100:x);}
    }
}
/* f_KnockOff: a rock or fin throws a rider into the water and the log stays behind. */
static void knock_off(Campaign *c,const Arena *a,int h){
    uint8_t bit=(uint8_t)(1u<<(6+h));
    if(c->mount_mask&bit){c->mount_mask&=(uint8_t)~bit;campaign_croc_place(c,a->p[h].x,a->p[h].y,-1);c->mount_cool[h]=60;}
}
/* ---- hazards ------------------------------------------------------------------------------------------------------ */
static void hazards(Campaign *c,Arena *a){
    FloodObject rock[FLOOD_ROCK_LIMIT],fin[FLOOD_FIN_LIMIT];
    unsigned nr=campaign_flood_rocks(c->timer,rock),nf=campaign_flood_fins(c->break_tick,fin);
    Fighter hazard={.facing=-1};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0||c->flood.grab[h])continue;
        /* f_RockScroll: a swimmer on the surface (not jumping) within 100 source units in front of a rock and 10 source units off its lane is knocked back. */
        if(p->z<10)for(unsigned i=0;i<nr;i++)if(p->x<rock[i].x&&rock[i].x-p->x<50&&abs(p->y-rock[i].y)<5&&!p->invulnerable){
            p->y=(int16_t)(p->y>rock[i].y?rock[i].y+3:rock[i].y-3);
            arena_campaign_strike(&hazard,p,5,6,-1,0,0);knock_off(c,a,h);
        }
        for(unsigned i=0;i<nf;i++)if(abs(p->x-fin[i].x)<14&&abs(p->y-fin[i].y)<5&&p->z<10&&!p->invulnerable){arena_campaign_strike(&hazard,p,10,6,fin[i].x>p->x?-1:1,0,0);knock_off(c,a,h);}
    }
}
/* ---- the three enemies -------------------------------------------------------------------------------------------- */
static const Fighter *prey_of(const Campaign *c,const Arena *a,const Fighter *e,int random_pick){
    const Fighter *t=0;int best=100000;
    if(random_pick){
        int live=0;for(int h=0;h<2;h++)live+=a->p[h].ready&&a->p[h].hp>0;
        if(live==2)return &a->p[mix(c->timer+(unsigned)e->x)&1];
    }
    for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-e->x)+2*abs(a->p[h].y-e->y);if(d<best){best=d;t=&a->p[h];}}
    return t;
}
static int stunned(const Fighter *e){return e->hitstun||e->freeze||e->pose==POSE_KNOCKDOWN||e->pose==POSE_AIRHURT||e->pose==POSE_HURT;}
static void step_toward(Fighter *e,int tx,int ty,int speed){
    int ox=e->x,oy=e->y,dx=tx-e->x,dy=ty-e->y,ax=abs(dx),ay=abs(dy);
    int sx=ax>ay?speed:ay?speed*ax/ay:0,sy=ay>=ax?speed:ax?speed*ay/ax:0;
    if(sx>ax)sx=ax;
    if(sy>ay)sy=ay;
    e->x+=dx<0?-sx:sx;e->y+=dy<0?-sy:sy;stage_move(e,ox,oy);
}
static void strike_heroes(Arena *a,const Fighter *src,int rx,int ry,int damage,int launch,int dir){
    for(int h=0;h<2;h++){Fighter *p=&a->p[h];if(p->ready&&p->hp>0&&abs(p->x-src->x)<rx&&abs(p->y-src->y)<ry&&p->z<20)arena_campaign_strike(src,p,damage,launch,dir,0,0);}
}
/* Source fat fish (sprite 486): speed 4, punch 12. It walks to its prey and leaps when it touches a player (jump clip, 30 ticks); the fifth hit
   it takes sends it charging in a straight line. */
static void fish(Campaign *c,Arena *a,int i){
    Fighter *e=&c->enemies[i];FloodState *f=&c->flood;
    if(e->hit_seq!=f->seen[i]){f->seen[i]=(uint8_t)e->hit_seq;if(++f->hits[i]>4){f->hits[i]=0;f->mode[i]=FISH_CHARGE;f->timer[i]=40;f->aux[i]=0;}else if(f->mode[i]==FISH_JUMP){f->mode[i]=FISH_WALK;}}
    if(e->hp<=0||stunned(e))return;
    const Fighter *t=prey_of(c,a,e,0);if(!t)return;
    int dir=t->x>e->x?1:-1;
    switch(f->mode[i]){
    case FISH_JUMP:
        e->pose=POSE_ATTACK;e->animation=(30-f->timer[i])/3;e->facing=dir;
        if(f->timer[i]==15)strike_heroes(a,e,26,10,12,4,dir);
        if(!--f->timer[i]){f->mode[i]=FISH_REST;f->timer[i]=40;}
        break;
    case FISH_CHARGE:{
        if(!f->aux[i])f->aux[i]=(uint8_t)(dir>0?1:2);
        int d=f->aux[i]==1?1:-1,ox=e->x,oy=e->y;
        e->x+=d*5;if(t->y>e->y+2)e->y+=2;else if(t->y<e->y-2)e->y-=2;stage_move(e,ox,oy);e->facing=d;e->pose=POSE_HEAVY;e->animation=f->timer[i]/4;
        strike_heroes(a,e,16,10,12,6,d);
        if(!--f->timer[i]||e->x==ox){f->mode[i]=FISH_REST;f->timer[i]=40;}
        break;}
    case FISH_REST:
        e->pose=POSE_IDLE;e->facing=dir;if(!--f->timer[i])f->mode[i]=FISH_WALK;
        break;
    default:{
        int dx=t->x-e->x,dy=t->y-e->y;
        if(abs(dx)<22&&abs(dy)<8){f->mode[i]=FISH_JUMP;f->timer[i]=30;break;}
        int ox=e->x,oy=e->y;
        if(abs(dx)>6)e->x+=(dx>0?1:-1)*(1+(c->timer%5<2));
        if(abs(dy)>3&&(c->timer&1))e->y+=dy>0?1:-1;
        stage_move(e,ox,oy);e->facing=dir;e->pose=POSE_WALK;e->animation=(e->animation+1)%16;}
    }
}
/* Source bats (EBAT, f_BatFlyLeft): speed 9, flies to a point near its prey at a random height, and carries off a swimmer that is standing still. */
static void bat(Campaign *c,Arena *a,int i){
    Fighter *e=&c->enemies[i];FloodState *f=&c->flood;
    if(e->hp<=0)return;
    if(stunned(e)){f->mode[i]=0;return;}
    if(e->z<20&&!e->vz)e->z=20;
    const Fighter *t=prey_of(c,a,e,1);if(!t)return;
    if(!f->mode[i]){
        unsigned r=mix(c->timer*7+(unsigned)i);
        f->tx[i]=(int16_t)(t->x+(e->x>t->x?1:-1)*(int)(r%37));f->ty[i]=t->y;f->aux[i]=(uint8_t)(20+(r>>8)%40);f->mode[i]=1;
    }
    step_toward(e,f->tx[i],f->ty[i],3);
    if(e->z<f->aux[i])e->z++;else if(e->z>f->aux[i])e->z--;
    e->facing=f->tx[i]<e->x?-1:1;e->pose=POSE_WALK;e->animation=(e->animation+1)%16;
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];
        if(p->ready&&p->hp>0&&!f->grab[h]&&p->pose==POSE_IDLE&&!p->z&&!((c->mount_mask>>(6+h))&1)&&abs(e->x-p->x)<15&&abs(e->y-p->y)<5){
            f->grab[h]=1;f->chomp[h]=0;e->hp=0;e->ready=0;return; /* the bat sits on the swimmer's head: sprite 1737 "bathead" */
        }
    }
    if(abs(e->x-f->tx[i])<=3&&abs(e->y-f->ty[i])<=3)f->mode[i]=0;
}
/* Held swimmers (sprite 1737): three ticks of intro, then six button presses shake the bat off. It bites for five every 30 ticks. */
static void held(Campaign *c,Arena *a,uint16_t keys[2]){
    FloodState *f=&c->flood;
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];
        if(!f->grab[h])continue;
        if(!p->ready||p->hp<=0){f->grab[h]=0;continue;}
        unsigned frame=f->grab[h];
        if(frame<4)frame++;else if(keys[h]&(ATTACK_EDGE|JUMP_EDGE|HEAVY_EDGE|BOW_EDGE|EQUIP_EDGE))frame++;
        keys[h]=0;p->invulnerable=2;
        if(++f->chomp[h]%30==0){p->hp=(int16_t)(p->hp>5?p->hp-5:0);p->flash=5;p->hit_seq++;}
        if(!p->hp){f->grab[h]=0;p->pose=POSE_DEAD;p->animation=0;continue;}
        if(frame>=10){ /* thrown off */
            f->grab[h]=0;p->z=1;p->vz=6;p->knockback=(mix(c->timer+(unsigned)h)&1?3:-3);p->pose=POSE_AIRHURT;p->invulnerable=20;continue;
        }
        f->grab[h]=(uint8_t)frame;
    }
}
static void held_pose(Campaign *c,Arena *a,const int16_t at[2][2]){
    for(int h=0;h<2;h++)if(c->flood.grab[h]){
        Fighter *p=&a->p[h];
        p->x=at[h][0];p->y=at[h][1];p->z=0;p->vz=0;p->cooldown=0;p->hitstun=0;p->pose=POSE_DIG;p->animation=(c->flood.grab[h]-1)/2;
    }
}
/* Source f_Boss (sprite 298): waits, picks a swimmer, chases to 100 pixels behind it and punches. Below half health it moves out of the lane and
   charges across the corridor. Between times it spits a hairball at the king boat, whose cannon stuns the Catfish for 120 ticks if it
   stands in the boat's lane (f_HitCatFish: only a stunned Catfish takes full damage, otherwise a quarter). */
static void cat(Campaign *c,Arena *a,int i){
    Fighter *e=&c->enemies[i];FloodState *f=&c->flood;
    int max=a->p[1].ready?900:600;
    if(e->hp<=0)return; /* death is driven by cat_death */
    if(f->stun){
        e->hitstun=2;e->pose=POSE_HURT;
        if(!--f->stun){e->hitstun=0;f->mode[i]=CAT_PICK;f->timer[i]=60;}
        return;
    }
    if(stunned(e))return;
    const Fighter *t=prey_of(c,a,e,0);if(!t)return;
    switch(f->mode[i]){
    case CAT_WAIT:
        e->pose=POSE_IDLE;if(!--f->timer[i]){f->mode[i]=CAT_PICK;f->timer[i]=60;}
        break;
    case CAT_PICK:
        e->pose=POSE_IDLE;
        if(!--f->timer[i]){f->mode[i]=CAT_CHASE;f->timer[i]=210;f->aux[i]=(uint8_t)(mix(c->timer)&1);}
        break;
    case CAT_CHASE:{
        const Fighter *p=&a->p[f->aux[i]&&a->p[1].ready&&a->p[1].hp>0?1:0];if(p->hp<=0)p=t;
        int tx=p->x+100;if(tx>FLOOD_WIDTH-120)tx=FLOOD_WIDTH-120;
        e->facing=p->x<e->x?-1:1;e->pose=POSE_WALK;e->animation=(e->animation+1)%16;
        if(abs(e->x-tx)<=2&&abs(e->y-p->y)<=2){f->mode[i]=CAT_PUNCH;f->tx[i]=f->timer[i];f->aux[i]=0;e->pose=POSE_ATTACK;break;}
        step_toward(e,tx,p->y,2);
        if(!--f->timer[i]){f->mode[i]=e->hp*2<max?CAT_LANE:CAT_SPIT;f->timer[i]=0;}
        break;}
    case CAT_PUNCH: /* punch1_3: 31 ticks, the fist lands at the middle; the chase clock resumes 30 ticks shorter */
        e->pose=POSE_ATTACK;e->animation=f->aux[i]*4/31;
        if(f->aux[i]==16)strike_heroes(a,e,60,12,10,6,e->facing);
        if(++f->aux[i]>=31){f->mode[i]=CAT_CHASE;f->aux[i]=(uint8_t)(mix(c->timer)&1);f->timer[i]=f->tx[i]>30?(uint8_t)(f->tx[i]-30):1;}
        break;
    case CAT_SPIT: /* punch2_1: 155 ticks; the hairball leaves at tick 70 */
        e->pose=POSE_LIGHT3;e->animation=f->timer[i]*8/155;
        if(f->timer[i]==70){f->spit_x=(int16_t)(e->x-125);f->ball=0;}
        if(++f->timer[i]>=155){f->mode[i]=CAT_WAIT;f->timer[i]=30;}
        break;
    case CAT_LANE: /* modes 5 and 6: out of the swimmer's lane by 30 pixels, then charge */
        e->pose=POSE_WALK;e->animation=(e->animation+1)%16;
        if(!f->timer[i]){f->ty[i]=(int16_t)(t->y<FLOOD_BOSS_Y?FLOOD_BOSS_Y+30:FLOOD_BOSS_Y-30);f->timer[i]=1;}
        step_toward(e,e->x,f->ty[i],2);
        if(abs(e->y-f->ty[i])<=2){f->mode[i]=CAT_CHARGE;f->timer[i]=0;f->aux[i]=0;e->facing=t->x>e->x?1:-1;}
        break;
    case CAT_CHARGE:{ /* punch2_2: 20 source units per tick across the corridor, then back with the next prey, up to five passes */
        int ox=e->x,oy=e->y;
        e->pose=POSE_LIGHT2;e->animation=(f->timer[i]++/6)%8;
        e->x+=e->facing*7;stage_move(e,ox,oy);
        for(int h=0;h<2;h++){Fighter *p=&a->p[h];if(p->ready&&p->hp>0&&abs(p->x-e->x)<50&&abs(p->y-e->y)<7&&p->z<20&&!p->invulnerable)arena_campaign_strike(e,p,10,6,e->facing,0,0);}
        if(e->facing<0&&abs(e->y-FLOOD_KINGBOAT_Y)<7&&abs(e->x-campaign_flood_boat_x(a))<175){ /* rams the boat: it explodes and the Catfish takes ten */
            e->hp=(int16_t)(e->hp>10?e->hp-10:1);f->mode[i]=CAT_WAIT;f->timer[i]=30;break;
        }
        if(e->x==ox||e->x<=FLOOD_BOSS_X-300||e->x>=FLOOD_BOSS_X+300){
            if(++f->aux[i]>=5){f->mode[i]=CAT_WAIT;f->timer[i]=30;e->x=FLOOD_BOSS_X;e->y=FLOOD_BOSS_Y;}
            else{e->facing=-e->facing;e->y=t->y;}
        }
        (void)oy;
        break;}
    }
}
/* Sprite 297: the Catfish's death clip runs 149 ticks before the score sign comes (frame 149 calls f_ScoreTally). The shared corpse code would remove it
   after 44, so the body is held in a knocked-down pose whose animation is the clip key. */
static int cat_death(Campaign *c){
    int dying=0;
    for(int i=0;i<6;i++){
        Fighter *e=&c->enemies[i];
        if(c->species[i]!=FLOOD_CAT||!e->ready||e->hp>0)continue;
        e->pose=POSE_KNOCKDOWN;e->z=0;e->vz=0;e->animation=c->flood.timer[i]/30;
        if(c->flood.timer[i]<149){c->flood.timer[i]++;dying=1;}
    }
    return dying;
}
/* The hairball and the cannon ball ride in the machine slot of the snapshot. */
static void projectiles(Campaign *c,const Arena *a){
    int boat=campaign_flood_boat_x(a);
    FloodState *f=&c->flood;Fighter *m=&c->war.machine;
    int boss=-1;for(int i=0;i<6;i++)if(c->species[i]==FLOOD_CAT&&c->enemies[i].ready&&c->enemies[i].hp>0)boss=i;
    m->ready=0;
    if(boss<0){f->spit_x=0;f->ball=0;return;}
    const Fighter *e=&c->enemies[boss];
    if(f->spit_x){
        f->spit_x=(int16_t)(f->spit_x-2);
        *m=(Fighter){.ready=1,.x=f->spit_x,.y=e->y,.z=20,.hp=1,.pose=POSE_IDLE,.attack_kind=0,.animation=(c->timer/3)%5};
        if(f->spit_x<boat+100){f->spit_x=0;f->ball=1;}
    }else if(f->ball){
        int x=boat+10*f->ball;
        *m=(Fighter){.ready=1,.x=x,.y=FLOOD_KINGBOAT_Y,.z=20,.hp=1,.pose=POSE_IDLE,.attack_kind=1};
        if(abs(x-e->x)<50&&abs(e->y-FLOOD_KINGBOAT_Y)<12&&!f->stun){f->stun=120;f->ball=0;m->ready=0;}
        else if(++f->ball>90)f->ball=0;
    }
}
static void enemies(Campaign *c,Arena *a){
    for(int i=0;i<6;i++){
        Fighter *e=&c->enemies[i];
        if(!e->ready)continue;
        if(c->species[i]==FLOOD_BAT)bat(c,a,i);else if(c->species[i]==FLOOD_CAT)cat(c,a,i);else fish(c,a,i);
    }
}
static void run_timeline(Campaign *c,Arena *a){
    uint16_t *cur=&c->break_tick;
    int cx=(a->p[0].x+(a->p[1].ready?a->p[1].x:a->p[0].x))/2;
    while(c->npc_ticks<TIMELINE_COUNT){
        const FloodEvent *e=&timeline[c->npc_ticks];
        if(*cur<e->at)break;
        if(e->kind==EV_WAIT){if(living(c))return;}
        else if(e->kind==EV_FOOD){
            /* f_ItemSpawn at the screen's right edge, y e_spawn + random(80) */
            Fighter f={0};if(place(&f,a,cx+144+50,FLOOD_E_SPAWN_Y+(int)(mix(e->arg+11u)%80)))campaign_queue_drop(c,f.x,f.y,DROP_FOOD,10|((e->arg%5)<<5));
        }else if(e->kind==EV_BOSS){
            if(living(c))return;
            if(spawn(c,a,FLOOD_BOSS_X,FLOOD_BOSS_Y,FLOOD_CAT,a->p[1].ready?900:600)){c->group=1;}else return;
        }else if(e->kind==EV_FISH){
            /* f_SpawnFatFish at the rock spawner, 50 down; the second one of the last pair comes in from the left edge */
            int x=e->arg?cx-144-50:FLOOD_ROCK_SPAWN_X,y=FLOOD_ROCK_SPAWN_Y+(e->arg?75:50);
            if(!spawn(c,a,x,y,FLOOD_FISH,60))return;
        }else{
            int x=e->kind==EV_BAT_L?cx-144-50:cx+144+50+(int)(mix(e->at)%50),y=FLOOD_E_SPAWN_Y+(int)(mix(e->at+3u)%80);
            if(x<90)x=90;
            if(x>FLOOD_WIDTH-100)x=FLOOD_WIDTH-100;
            if(!spawn(c,a,x,y,FLOOD_BAT,50))return;
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
    held(c,a,k2);
    campaign_mount_step(c,a,k2);
    int16_t at[2][2]={{a->p[0].x,a->p[0].y},{a->p[1].x,a->p[1].y}};
    int hp[6];for(int i=0;i<6;i++)hp[i]=c->enemies[i].hp;
    int stun=c->flood.stun>0;
    campaign_combat_tick(c,a,k2);
    campaign_mount_arc(c,a);
    held_pose(c,a,at);
    /* Catfish damage rule: a hit that lands while it is not stunned only counts for a quarter. */
    for(int i=0;i<6;i++){Fighter *e=&c->enemies[i];
        if(e->ready&&c->species[i]==FLOOD_CAT&&e->hp>0&&e->hp<hp[i]&&!stun)e->hp+=(int16_t)((hp[i]-e->hp)*3/4);}
    logs(c,a);
    hazards(c,a);
    enemies(c,a);
    projectiles(c,a);
    if(c->group==1&&c->npc_ticks>=TIMELINE_COUNT&&!living(c)&&!cat_death(c)){c->phase=WAR_COMPLETE;c->completed|=128;c->war.machine.ready=0;}
}
int campaign_flood_boss_hp(const Campaign *c){for(int i=0;i<6;i++)if(c->species[i]==FLOOD_CAT&&c->enemies[i].ready&&c->enemies[i].hp>0)return c->enemies[i].hp;return 0;}
const char *campaign_flood_objective(const Campaign *c){
    if(c->phase==WAR_COMPLETE)return "Flooded Castle complete";
    if(c->group==1)return "Defeat the Catfish";
    return "Swim for the castle";
}
