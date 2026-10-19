#pragma GCC optimize("Os") /* Once-per-tick scene logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_deer.h"
#include "stage.h"
#include "campaign_scene.h"
#include <stdlib.h>
#include <string.h>

/* LEVEL22 (level22.pak): sprite 188 (monster and mill clip), 141 (log sequence), 132 (log), 88 (mill scroll), f_MonsterMash.
   Source frame = one tick. Distances: source/2. Speeds: 1/16 pixel per tick. */
#define GUIDE_FULL 109          /* the guide tops out at 20 source units per tick, scaled to the DS view (see SPEED_SCALE) */
#define GUIDE_RAMP_TICKS 190    /* 1 + 0.1n source units reaches 20 after 190 ticks */
/* The DS view shows 288 world pixels where the original showed 424, so source speeds feel 1.47 times faster on the small screen; every
   chase speed is scaled by 17/25 to keep the share of the screen crossed per tick the same. */
#define LOG_SPEED16 98          /* logs move left at 18 source units per tick in the world */
#define FIRST_WAVE_TICK 30
#define EXIT_REL (-128)         /* a log is recycled when it has passed 200 source units left of the screen */
#define MONSTER_X_BEHIND 48     /* the monster's origin trails the guide by 97 source units */
#define FINALE_SPEED 7
#define DOCK_RUN_BEFORE 130
#define DOCK_TICKS 70

typedef struct {int8_t kind,off;} WaveLog; /* off: log x ahead of the first log, in 4 DS pixel steps */
typedef struct {WaveLog log[3];} Wave;
/* LEVEL22 sprite 141 frames 2..25: each step sets the next log kinds and their spacing (source +300..+800 = 150..400 DS). Frames without
   an action (8, 10, 16) keep the previous logs and are not waves. Kinds 2..9 are log piles (jump them), 10..12 are walls with a gap. */
static const Wave waves[DEER_WAVE_COUNT]={
    {{{6,0},{0,0},{0,0}}},{{{7,0},{0,0},{0,0}}},{{{4,0},{0,0},{0,0}}},{{{8,0},{0,0},{0,0}}},{{{10,0},{0,0},{0,0}}},
    {{{2,0},{5,75},{0,0}}},{{{3,0},{10,50},{0,0}}},{{{12,0},{6,62},{0,0}}},{{{3,0},{0,0},{0,0}}},{{{11,0},{0,0},{0,0}}},
    {{{10,0},{0,0},{0,0}}},{{{11,0},{7,100},{0,0}}},{{{10,0},{0,0},{0,0}}},{{{11,0},{0,0},{0,0}}},{{{12,0},{0,0},{0,0}}},
    {{{11,0},{0,0},{0,0}}},{{{10,0},{0,0},{0,0}}},{{{6,0},{0,0},{0,0}}},{{{7,0},{8,75},{0,0}}},{{{4,0},{3,75},{0,0}}},{{{5,0},{2,75},{0,0}}}};

/* ---- the deterministic part: guide speed and distance --------------------------------------------------------- */
static unsigned speed16(unsigned n){unsigned v=(8+(n*4)/5)*17/25;return v>GUIDE_FULL?GUIDE_FULL:v;}
static uint16_t ramp[GUIDE_RAMP_TICKS+1]; /* distance in 1/16 DS pixel after n chase ticks, n <= GUIDE_RAMP_TICKS */
static uint16_t spawn_tick[DEER_WAVE_COUNT];
static uint16_t chase_end,table_ready; /* chase ticks before the finale */
static uint32_t dist16(unsigned n){return n<=GUIDE_RAMP_TICKS?ramp[n]:(uint32_t)ramp[GUIDE_RAMP_TICKS]+(uint32_t)GUIDE_FULL*(n-GUIDE_RAMP_TICKS);}
static uint32_t travelled16(unsigned n){return dist16(n)+(uint32_t)LOG_SPEED16*n;} /* what a log has moved left relative to the guide */
static void tables(void){
    if(table_ready)return;
    unsigned sum=0;for(unsigned i=0;i<GUIDE_RAMP_TICKS;i++){ramp[i]=(uint16_t)sum;sum+=speed16(i);}ramp[GUIDE_RAMP_TICKS]=(uint16_t)sum;
    unsigned at=FIRST_WAVE_TICK;
    for(unsigned w=0;w<DEER_WAVE_COUNT;w++){
        spawn_tick[w]=(uint16_t)at;
        /* The next wave appears when this wave's first log leaves the screen; the last wave also waits for its trailing logs. */
        int extra=0;if(w==DEER_WAVE_COUNT-1)for(int j=0;j<3;j++)if(waves[w].log[j].kind&&waves[w].log[j].off*4>extra)extra=waves[w].log[j].off*4;
        unsigned n=at;
        while(((int32_t)(DEER_LOG_SPAWN_AHEAD+extra)*16-(int32_t)(travelled16(n)-travelled16(at)))>=EXIT_REL*16)n++;
        at=n;
    }
    chase_end=(uint16_t)at;table_ready=1;
}
static int wrap_guide(uint32_t d16){return DEER_GUIDE_START+(int)((d16/16)%DEER_LOOP_PERIOD);}
static unsigned finale_start(void){tables();return DEER_INTRO_TICKS+chase_end;}
static int finale_origin(void){tables();return wrap_guide(dist16(chase_end));}
static unsigned dock_start(void){
    int g0=finale_origin(),target=DEER_DOCK_X-DOCK_RUN_BEFORE;
    return finale_start()+(unsigned)((target-g0+FINALE_SPEED-1)/FINALE_SPEED);
}
unsigned campaign_deer_stage(const Campaign *c){
    unsigned t=c->timer;tables();
    return t<DEER_INTRO_TICKS?DEER_STAGE_INTRO:t<finale_start()?DEER_STAGE_CHASE:t<dock_start()?DEER_STAGE_FINALE:DEER_STAGE_DOCK;
}
static int guide_at(unsigned t){
    tables();
    if(t<DEER_INTRO_TICKS)return DEER_GUIDE_START;
    if(t<finale_start())return wrap_guide(dist16(t-DEER_INTRO_TICKS));
    unsigned ds=dock_start();
    return finale_origin()+(int)((t<ds?t:ds)-finale_start())*FINALE_SPEED;
}
int campaign_deer_guide(const Campaign *c){return guide_at(c->timer);}
int campaign_deer_gap(unsigned kind,int *top,int *bottom){
    /* Source top/bottom markers of sprite 132 frames 10..12, relative to the log row y 220. */
    static const int8_t gap[3][2]={{-28,8},{5,44},{-52,-12}};
    if(kind<10||kind>12)return 0;
    *top=220+gap[kind-10][0];*bottom=220+gap[kind-10][1];return 1;
}
/* Visible logs. rel = x relative to the guide. */
static unsigned logs_at(unsigned t,int guide,DeerLog out[DEER_LOG_LIMIT],int rel_out[DEER_LOG_LIMIT]){
    tables();unsigned count=0;
    if(t<DEER_INTRO_TICKS)return 0;
    unsigned n=t-DEER_INTRO_TICKS;
    if(n>(unsigned)chase_end+60)return 0;
    for(unsigned w=0;w<DEER_WAVE_COUNT&&spawn_tick[w]<=n;w++){
        for(int j=0;j<3;j++){
            const WaveLog *l=&waves[w].log[j];if(!l->kind)continue;
            int32_t rel16=(int32_t)(DEER_LOG_SPAWN_AHEAD+l->off*4)*16-(int32_t)(travelled16(n)-travelled16(spawn_tick[w]));
            int rel=rel16>>4;
            if(rel<-130||rel>300||count>=DEER_LOG_LIMIT)continue;
            out[count]=(DeerLog){.x=(int16_t)(guide+rel),.y=(int16_t)(220+(j==2?3:0)),.kind=(uint8_t)l->kind,.debris=0};
            if(rel<-MONSTER_X_BEHIND){int k=(-MONSTER_X_BEHIND-rel)/6;out[count].debris=(uint8_t)(k>3?3:k)+1;}
            rel_out[count]=rel;count++;
        }
    }
    return count;
}
unsigned campaign_deer_logs(const Campaign *c,DeerLog out[DEER_LOG_LIMIT]){int rel[DEER_LOG_LIMIT];return logs_at(c->timer,guide_at(c->timer),out,rel);}
/* Entrance clip (sprite 188): the key frame shown at tick t, matching tools/deer_actors.py MON. */
int campaign_deer_monster_key(const Campaign *c,int *x){
    static const uint16_t frames[12]={196,200,210,225,242,260,280,285,289,296,305,316};
    unsigned t=c->timer;tables();
    *x=guide_at(t)-MONSTER_X_BEHIND;
    if(t>=finale_start()+64)return -1; /* the monster gives up when the flume ends (sprite 88 frame 404) */
    if(t+1<196)return -1;
    int k=0;for(int i=0;i<12;i++)if(t+1>=frames[i])k=i;
    return k;
}
unsigned campaign_deer_shake(const Campaign *c){
    /* f_ScreenShake(seconds, magnitude) calls of sprite 188: frame, duration in ticks, amplitude. */
    static const struct {uint16_t at,len;uint8_t amp;} cues[]={{6,6,2},{36,9,3},{73,6,2},{103,9,3},{133,6,2},{175,6,3},{187,6,3},{196,6,3},{285,15,4},{316,9,3}};
    unsigned t=c->timer;
    for(unsigned i=0;i<sizeof cues/sizeof *cues;i++)if(t>=cues[i].at&&t<cues[i].at+cues[i].len)return cues[i].amp;
    tables();
    if(t>=finale_start()&&t<finale_start()+60)return 2+(unsigned)((t-finale_start())/20); /* the flume breaks up (sprite 88 frames 390..437) */
    return 0;
}
unsigned campaign_deer_cue(unsigned after,unsigned now){
    /* 1 Explosion3, 2 Explosion4, 3 MillMonster, 4 Ground2, 5 Ground3, 6 MillMonster2 (sprite 188 and 88 frame actions). */
    static const struct {uint16_t at;uint8_t id;} cues[]={{6,1},{36,2},{73,1},{103,2},{133,1},{155,3},{175,4},{187,5},{196,5},{285,2},{316,4}};
    unsigned best=0;
    for(unsigned i=0;i<sizeof cues/sizeof *cues;i++)if(cues[i].at>after&&cues[i].at<=now&&(!best||cues[i].at<(best>>8)))best=((unsigned)cues[i].at<<8)|cues[i].id;
    tables();
    const struct {unsigned at;uint8_t id;} fin[]={{finale_start()+4,2},{finale_start()+14,1},{finale_start()+47,6}};
    for(unsigned i=0;i<3;i++)if(fin[i].at>after&&fin[i].at<=now&&(!best||fin[i].at<(best>>8)))best=(fin[i].at<<8)|fin[i].id;
    return best;
}
int campaign_deer_camera_x(const Campaign *c){
    int t=(int)c->timer,off=DEER_CAMERA_AHEAD;
    if(t<DEER_INTRO_TICKS){
        const int near_party=8,far_left=-70;
        if(t<180)off=near_party;
        else if(t<230)off=near_party+(far_left-near_party)*(t-180)/50;
        else if(t<285)off=far_left;
        else off=far_left+(DEER_CAMERA_AHEAD-far_left)*(t-285)/(DEER_INTRO_TICKS-285);
    }
    return campaign_deer_guide(c)+off;
}
const char *campaign_deer_objective(const Campaign *c){
    if(c->phase==WAR_COMPLETE)return "Mill Run complete";
    switch(campaign_deer_stage(c)){
    case DEER_STAGE_INTRO:return "Something is in the mill";
    case DEER_STAGE_CHASE:return "Jump the logs, find the gaps";
    case DEER_STAGE_FINALE:return "Run for the dock";
    default:return "Jump!";
    }
}

/* ---- the simulated part: the two heroes ------------------------------------------------------------------------------- */
static const int16_t start_pos[2][2]={{DEER_GUIDE_START+DEER_HERO_OFFSET,216},{DEER_GUIDE_START+DEER_HERO_OFFSET-32,195}};
void campaign_deer_start(Campaign *c,Arena *a){
    a->map_id=DEER_MAP;a->round=DEER_MAP;stage_set_map(DEER_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(c->species,0,sizeof(c->species));memset(&c->ally,0,sizeof(c->ally));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=DEER_STAGE_INTRO;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_TRAVEL;c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->targets,0,sizeof(c->targets));c->mount_mask=0;
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);
        a->p[h].x=start_pos[h][0];a->p[h].y=start_pos[h][1];a->p[h].z=0;a->p[h].vz=0;
        a->p[h].pose=POSE_IDLE;a->p[h].animation=0;a->p[h].guard=a->p[h].hitstun=0;
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};
    }
    if(a->p[1].ready){c->rng=c->rng*1664525u+1013904223u;if(((c->rng>>8)%3)==0)c->story_flags|=DEER_STORY_POOP;} /* one deer in three, as the source picks deer 2..4 */
    tables();
}
static int clamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static void hero_tick(Arena *a,unsigned h,uint16_t keys,int guide_prev,int guide_now,unsigned t,unsigned stage){
    Fighter *p=&a->p[h];
    if(!p->ready)return;
    int rel=p->x-guide_prev;
    if(p->hp<=0){p->pose=POSE_DEAD;p->z=0;p->animation=0;p->x=guide_now+rel;p->x=clamp(p->x,70,DEER_WIDTH-70);return;}
    p->facing=1;
    unsigned dockstart=dock_start();
    if(stage==DEER_STAGE_DOCK){
        unsigned td=t-dockstart;
        /* Both heroes run to the dock edge and jump (PLAYER dockjump, 55 ticks). */
        int target=DEER_DOCK_X-(int)h*20;
        if(p->x<target){p->x+=6;if(p->x>target)p->x=target;}
        else p->x=target;
        p->y=clamp(p->y,180,287);
        p->pose=DEER_POSE_DOCK;p->animation=(int16_t)(td<55?td*10/55:9);
        p->z=td<55?(int16_t)((td*(55-td))/20):0;
        return;
    }
    if(p->hitstun>0){ /* tossed by the monster: 24 ticks of deertoss, carried along at the mash line */
        rel=DEER_MASH_LINE;p->hitstun--;
        p->pose=DEER_POSE_TOSS;p->animation=(int16_t)((24-p->hitstun-1)*8/24);p->z=0;
        p->x=guide_now+rel;p->x=clamp(p->x,70,DEER_WIDTH-70);return;
    }
    int dx=0,dy=0;
    dx=((keys&RIGHT)?2:0)-((keys&LEFT)?2:0);dy=((keys&DOWN)?2:0)-((keys&UP)?2:0);
    rel=clamp(rel+dx,-60,190);p->y=(int16_t)clamp(p->y+dy,180,287);
    if(p->z>0||p->vz){p->z+=p->vz;p->vz--;if(p->z<=0){p->z=0;p->vz=0;}}
    else if((keys&JUMP_EDGE)&&!p->guard){p->vz=10;p->z=1;} /* a slammed rider cannot jump out of the slam */
    /* Logs: a pile slams a rider who is not in the air, a wall slams one outside its gap. The log pins the hero at its face. */
    DeerLog logs[DEER_LOG_LIMIT];int lrel[DEER_LOG_LIMIT];unsigned n=logs_at(t,guide_now,logs,lrel);
    int pinned=0;
    for(unsigned i=0;i<n;i++){
        if(logs[i].debris)continue;
        int face=lrel[i]-31,top,bottom;
        if(rel<face||rel>face+25)continue;
        int wall=campaign_deer_gap(logs[i].kind,&top,&bottom);
        int hit=wall?(p->y<top||p->y>bottom):p->z<=0;
        if(hit){rel=face;pinned=1;p->z=0;p->vz=0;}
    }
    /* A log that reaches the monster explodes and sets the heroes in front of it back. */
    unsigned back=t>0?t-1:0;DeerLog before[DEER_LOG_LIMIT];int brel[DEER_LOG_LIMIT];unsigned nb=logs_at(back,guide_prev,before,brel);
    for(unsigned i=0;i<nb;i++)if(brel[i]>=-MONSTER_X_BEHIND){
        for(unsigned j=0;j<n;j++)if(logs[j].kind==before[i].kind&&abs(lrel[j]-brel[i])<=30&&lrel[j]<-MONSTER_X_BEHIND&&rel<lrel[j]+37)rel=lrel[j]-50;
    }
    if(rel<DEER_MASH_LINE){ /* f_MonsterMash */
        rel=DEER_MASH_LINE;p->hp=(int16_t)(p->hp>10?p->hp-10:0);p->hitstun=24;p->guard=0;p->z=0;p->vz=0;
        p->pose=DEER_POSE_TOSS;p->animation=0;
    }else if(pinned){
        p->guard++;p->pose=DEER_POSE_SLAM;p->animation=(int16_t)(p->guard>43?9:p->guard*10/44);
    }else{
        p->guard=0;
        if(p->z>0){p->pose=DEER_POSE_JUMP;int k=(10-p->vz)*8/22;p->animation=(int16_t)clamp(k,0,7);}
        else {p->pose=DEER_POSE_RUN;p->animation=(int16_t)((t/2)%11);}
    }
    p->x=clamp(guide_now+rel,70,DEER_WIDTH-70);
    p->invulnerable=0;
}
void campaign_deer_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(DEER_MAP);
    if(c->phase==WAR_COMPLETE)return;
    unsigned t=c->timer;tables();
    unsigned stage=campaign_deer_stage(c);
    c->group=(uint8_t)stage;
    a->tick++;
    if(stage==DEER_STAGE_INTRO){
        /* The mill's entrance clip: the party stands frozen; the deer take fright from frame 280 and the heroes jump on at frame 305. */
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){
            Fighter *p=&a->p[h];p->x=start_pos[h][0];p->y=start_pos[h][1];p->z=0;p->facing=1;
            if(t>=305){p->pose=DEER_POSE_MOUNT;p->animation=(int16_t)((t-305)*7/11);{int tt=(int)(t-305);p->z=(int16_t)(tt*(11-tt)/2);}}
            else if(t>=250){p->pose=DEER_POSE_SCARED;p->animation=(int16_t)(((t-250)/6)%5);}
            else {p->pose=POSE_IDLE;p->animation=0;}
        }
    }else{
        int gp=guide_at(t-1),gn=guide_at(t);
        for(unsigned h=0;h<2;h++)hero_tick(a,h,keys[h],gp,gn,t,stage);
        if(stage==DEER_STAGE_DOCK&&t-dock_start()>=DOCK_TICKS){c->phase=WAR_COMPLETE;c->completed|=64;}
    }
    for(int h=0;h<2;h++)if(a->p[h].ready){c->pet_followers[h].x=(int16_t)(a->p[h].x-34);c->pet_followers[h].y=a->p[h].y;c->pet_followers[h].facing=1;} /* The pets run behind their deer. */
    if(c->phase!=WAR_COMPLETE)c->timer++;
}
