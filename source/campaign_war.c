#pragma GCC optimize("Os") /* Once-per-tick encounter logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_war.h"
#include "stage.h"
#include "campaign_scene.h"
#include <stdlib.h>
#include <string.h>

static int progress(const Arena *a){int x=0;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&a->p[h].x>x)x=a->p[h].x;return x;}
static int living(const Campaign *c){int n=0;for(int i=0;i<6;i++)n+=!(c->war.runner_mask&(1u<<i))&&c->enemies[i].ready&&c->enemies[i].hp>0&&c->enemies[i].knight_id!=3;return n;}
static int clear(const Arena *a,int x,int y){for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(x-a->p[h].x)<56&&abs(y-a->p[h].y)<28)return 0;return 1;}
static int place(Fighter *p,const Arena *a,int x,int y,int safe){
    for(int dx=0;dx<=224;dx+=16)for(int side=1;side>=-1;side-=2){
        int xx=x+side*dx;
        if(xx<0||xx>=stage_world_width(a->map_id))continue;
        for(int dy=0;dy<160;dy++)for(int sy=-1;sy<=1;sy+=2){
            int yy=y+sy*dy;
            if(yy<0||yy>=stage_world_height(a->map_id))continue;
            if(stage_position_open_map(a->map_id,xx,yy)&&(!safe||clear(a,xx,yy))){p->x=xx;p->y=yy;return 1;}
        }
    }
    return 0;
}
static int spawn(Campaign *c,Arena *a,WarSpawn s){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    Fighter p={.hp=a->p[1].ready?60:50,.mana=ARENA_MANA,.ready=1,.facing=-1,.invulnerable=8,.knight_id=s.kind};
    if(!place(&p,a,s.x,s.y,1))return 0;
    c->enemies[i]=p;c->targets[i]=0;c->combat.death_age[i]=0;
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1;
}
/* Original scene376 prop placements; all positions are source /2 +origin. */
static const WarPoint grass_points[7]={{1911,238},{2059,241},{1914,287},{2051,312},{2244,319},{2339,280},{2125,345}};
static const uint8_t grass_items[7]={4,3,4,5,2,3,0};
static unsigned random_value(Campaign *c,unsigned n){c->rng=c->rng*1664525u+1013904223u;return (c->rng>>8)%n;}
static int prop_hit(Campaign *c,Arena *a,unsigned bit,int x,int y,int arrows){
 Fighter object={.x=x,.y=y,.hp=1,.ready=1};
 for(int h=0;h<2;h++){
  Fighter *p=&a->p[h];
  if(p->ready&&campaign_attack_contact(p)&&p->attack_kind!=HIT_NONE&&c->break_hit_seq[bit][h]!=(uint8_t)p->attack_seq&&arena_melee_overlap(p,&object)){
   c->break_hit_seq[bit][h]=(uint8_t)p->attack_seq;return 1;
  }
 }
 if(arrows)for(int s=0;s<2;s++){Projectile *p=&a->shots[s];if(p->life&&p->owner>=0&&p->owner<2&&p->kind==SHOT_ARROW&&abs(p->x-x)<20+abs(p->vx)&&abs(p->y-y)<12&&p->z<60){p->life=0;return 1;}}
 return 0;
}
static void food_drop(Campaign *c,Arena *a,int x,int y,unsigned value){Fighter p={0};if(place(&p,a,x,y,0))campaign_queue_drop(c,p.x,p.y,DROP_FOOD,value);}
static void props(Campaign *c,Arena *a){
 if(a->map_id!=WAR_MAP)return;
 if(!(c->broken&1)&&prop_hit(c,a,1,676,307,1)){c->broken|=1;c->war.wrestle_pending=3;}
 if(c->war.wrestle_pending&1){int slot=spawn(c,a,(WarSpawn){676,307,0});if(slot){Fighter *p=&c->enemies[slot-1];p->z=8;p->vz=5;p->pose=POSE_AIRHURT;p->knockback=5;c->war.wrestle_pending&=~1u;}}
 if(c->war.wrestle_pending&2){
  Fighter *p=!c->ally.ready?&c->ally:0;
  if(!p)for(int i=0;i<6;i++)if(!c->enemies[i].ready){p=&c->enemies[i];c->combat.death_age[i]=0;break;}
  if(p){*p=(Fighter){.hp=60,.mana=40,.ready=1,.facing=1,.knight_id=3,.pose=POSE_KNOCKDOWN,.cooldown=20};place(p,a,676,307,0);c->war.wrestle_pending&=~2u;}
 }
 if(!(c->broken&2)&&prop_hit(c,a,0,3058,239,1)){
  if(c->break_hp[0])c->break_hp[0]--;
  if(!c->break_hp[0]){c->broken|=2;food_drop(c,a,3058,239,10|(random_value(c,5)<<5));}
 }
 for(int i=0;i<7;i++)if(!(c->broken&(4u<<i))&&prop_hit(c,a,i+2,grass_points[i].x,grass_points[i].y,0)){
  c->broken|=4u<<i;unsigned food=grass_items[i];
  if(!food&&random_value(c,3)==1)food=2+random_value(c,5);
  if(food)food_drop(c,a,grass_points[i].x,grass_points[i].y,10|((food-2)<<5));
 }
}
static void sleep_after_hit(Campaign *c,const uint16_t before[6]){
 if(c->group!=7||c->phase!=WAR_FIGHT)return;
 for(int i=0;i<6;i++)if(c->enemies[i].hit_seq!=before[i]){
  c->war.flags|=WAR_FLAG_SLEEP_RELEASED;
  if(c->enemies[i].knight_id==WAR_SLEEPING_BARBARIAN)c->enemies[i].knight_id=WAR_BARBARIAN;
 }
 if((c->war.flags&WAR_FLAG_SLEEP_RELEASED)&&c->timer%30==0)for(int i=0;i<6;i++){
  Fighter *p=&c->enemies[i];if(p->ready&&p->hp>0&&p->knight_id==WAR_SLEEPING_BARBARIAN&&random_value(c,14)==1){p->knight_id=WAR_BARBARIAN;p->pose=POSE_IDLE;p->animation=0;}
 }
}

static void fill_wave(Campaign *c,Arena *a){
    if(a->map_id==BRIDGE_MAP){while(c->spawned<3){WarPoint p=bridge_spawns[c->spawned];if(!spawn(c,a,(WarSpawn){p.x,p.y,0}))break;c->spawned++;}return;}
    if(c->group>=WAR_WAVE_COUNT)return;
    const WarWave *w=&war_waves[c->group];
    while(c->spawned<w->count&&spawn(c,a,war_spawns[w->first+c->spawned]))c->spawned++;
}
static void begin_wave(Campaign *c,Arena *a){c->phase=WAR_FIGHT;c->timer=0;c->spawned=0;if(c->group==7)c->war.flags&=~WAR_FLAG_SLEEP_RELEASED;fill_wave(c,a);}
static void depart(Fighter *p,const Arena *a){
    if(!p->ready||p->hp<=0)return;
    int edge=progress(a)+260;if(edge>WAR_WIDTH-40)edge=WAR_WIDTH-40;
    p->pose=POSE_WALK;p->facing=1;p->animation=(p->animation+1)%16;place(p,a,p->x+3,p->y,0);
    if(p->x>=edge)p->ready=0;
}
static void allies(Campaign *c,Arena *a){
    if(a->map_id!=WAR_MAP||!c->group||living(c))return;
    if(!(c->war.runner_mask&64))depart(&c->ally,a);
    for(int i=0;i<6;i++)if(c->enemies[i].knight_id==3)depart(&c->enemies[i],a);
}
void campaign_war_start(Campaign *c,Arena *a,unsigned map){
    if(map!=WAR_MAP&&map!=BRIDGE_MAP)return;
    a->map_id=map;a->round=map;stage_set_map(map);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=0;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_TRAVEL;
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));c->break_hp[0]=3;memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));
    c->war.gate_hp=WAR_GATE_HP(a->p[1].ready?2:1)+1; /* Source opens only below zero HP. */
    for(int h=0;h<2;h++)if(a->p[h].ready){WarPoint p=map==WAR_MAP?war_start[h]:bridge_start[h];campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,p.x,p.y,0);c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};}
    if(map==WAR_MAP){
        begin_wave(c,a);
        Fighter gray={.hp=60,.mana=40,.ready=1,.facing=1,.knight_id=3};
        place(&gray,a,war_allies[0].x,war_allies[0].y,0);c->ally=gray;
        place(&gray,a,war_allies[1].x,war_allies[1].y,0);c->enemies[5]=gray;
    }
}
static void machine_spawn(Campaign *c,Arena *a){
    c->war.machine=(Fighter){.x=WAR_MACHINE_SPAWN_X,.y=WAR_MACHINE_SPAWN_Y,.hp=WAR_MACHINE_HP(a->p[1].ready?2:1),.ready=1,.facing=-1,.pose=POSE_WALK};
    c->war.mode=WAR_MACHINE_LEFT;c->war.summon_timer=100;
    /* Source guns start with no timer and then reload every 90 ticks. The DS has two shot slots, so the three barrels are staggered. */
    c->war.barrel_timer[0]=20;c->war.barrel_timer[1]=50;c->war.barrel_timer[2]=80;c->war.speed4=c->war.frac=0;
}
static unsigned pick_hero(Campaign *c,const Arena *a){
    unsigned alive[2],n=0;for(unsigned h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0)alive[n++]=h;
    return n?alive[random_value(c,n)]:0;
}
/* Camera left edge when the fight camera follows the rightmost hero inside the source leash. */
static int machine_camera_left(const Arena *a){
    int left=progress(a)-WAR_CAMERA_WIDTH/2,lo=WAR_MACHINE_LEASH_LEFT-32,hi=WAR_MACHINE_LEASH_RIGHT+32-WAR_CAMERA_WIDTH;
    return left>hi?hi:left<lo?lo:left;
}
/* Source f_WarMachine: ram left, ram right, track a player for 400 ticks, shake, repeat.
   Source f_WarMachineAim reloads each of the three cannons every 90 ticks in every mode. */
static void machine_tick(Campaign *c,Arena *a){
    WarState *w=&c->war;Fighter *m=&w->machine;int old_x=m->x,old_y=m->y;
    if(m->hp<=0){c->phase=WAR_MACHINE_DEATH;c->timer=0;w->mode=WAR_MACHINE_DYING;m->pose=POSE_DEAD;m->animation=0;return;}
    int camera_left=machine_camera_left(a),camera_right=camera_left+WAR_CAMERA_WIDTH;
    unsigned players=a->p[1].ready?2:1;
    unsigned target=w->target;if(target>1||!a->p[target].ready||a->p[target].hp<=0)target=pick_hero(c,a);w->target=target;
    Fighter *hero=&a->p[target];m->attack_kind=w->mode;
    m->animation=(m->animation+1)%30;m->pose=POSE_WALK;
    if(w->mode==WAR_MACHINE_LEFT||w->mode==WAR_MACHINE_RIGHT){
        int dir=w->mode==WAR_MACHINE_LEFT?-1:1;m->facing=dir;m->x+=dir*((a->tick&1)?7:8);w->events|=WAR_EVENT_RAM;
        if(dir<0&&m->x<camera_left-90){w->target=pick_hero(c,a);m->y=a->p[w->target].y-5;w->mode=WAR_MACHINE_RIGHT;}
        else if(dir>0&&m->x>camera_right+90){m->facing=-1;w->target=pick_hero(c,a);w->mode=WAR_MACHINE_TRACK;w->mode_timer=400;w->speed4=w->frac=0;}
    }else if(w->mode==WAR_MACHINE_TRACK){
        int moving=1;m->facing=-1;
        /* Hold 75..125 world units right of the prey while keeping the body near the screen. */
        if(m->x<hero->x+75&&m->x<camera_right-50){if(w->speed4<8)w->speed4++;}
        else if(m->x>hero->x+125){if(w->speed4>-8)w->speed4--;}
        else if(w->speed4>0)w->speed4--;
        else if(w->speed4<0)w->speed4++;
        else moving=0;
        int step=w->frac+w->speed4;m->x+=step/4;w->frac=step%4;
        if(w->summon_timer)w->summon_timer--;
        if(!w->summon_timer&&living(c)<3){w->mode=WAR_MACHINE_SUMMON;w->mode_timer=0;w->summon_timer=400;}
        else if(w->mode_timer&&!--w->mode_timer){w->mode=WAR_MACHINE_WINDUP;w->mode_timer=46;}
        if(!moving)m->pose=POSE_IDLE;
    }else if(w->mode==WAR_MACHINE_WINDUP){m->pose=POSE_IDLE;if(w->mode_timer&&!--w->mode_timer)w->mode=WAR_MACHINE_LEFT;}
    else if(w->mode==WAR_MACHINE_SUMMON){
        m->pose=POSE_CAST;w->mode_timer++;m->animation=w->mode_timer;
        if((w->mode_timer==42||w->mode_timer==75)&&living(c)<3)spawn(c,a,(WarSpawn){m->x+m->facing*40,m->y+15,0});
        if(w->mode_timer>=75){w->mode=WAR_MACHINE_TRACK;w->mode_timer=400;}
    }
    for(unsigned b=0;b<3;b++){
        if(w->barrel_timer[b])w->barrel_timer[b]--;
        if(!w->barrel_timer[b]){
            /* Source gun n aims at player n, or a random player when it has none. */
            unsigned t=b<players&&a->p[b].hp>0?b:pick_hero(c,a);
            w->barrel=b;w->target=t;w->events|=WAR_EVENT_CANNON;w->barrel_timer[b]=90;
        }
    }
    /* Heroes are held by the source leash; the machine may ram beyond it. */
    if(m->x<WAR_MACHINE_LEASH_LEFT-400)m->x=WAR_MACHINE_LEASH_LEFT-400;
    if(m->x>WAR_MACHINE_SPAWN_X+60)m->x=WAR_MACHINE_SPAWN_X+60;
    if(m->pose==POSE_WALK&&m->x==old_x&&m->y==old_y){m->pose=POSE_IDLE;m->animation=0;}
}
static void runners_tick(Campaign *c,const Arena *a){
    /* Source movie363 creates fleeing extras, not a new encounter. Their
       last spawn at75 can still be on screen when the movie stops at245. */
    for(int i=0;i<7;i++)if(c->war.runner_mask&(1u<<i)){
        Fighter *p=i==6?&c->ally:&c->enemies[i];p->x-=2;p->pose=POSE_WALK;p->facing=-1;p->animation=(p->animation+1)%16;
        if(p->x<progress(a)-240){p->ready=0;c->war.runner_mask&=~(1u<<i);}
    }
}
static void horse_tick(Campaign *c,Arena *a){
    unsigned frame=c->timer;
    campaign_cinematic_tick(c,a);
    if(frame>=2&&frame<128){
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){Fighter *p=&a->p[h];int x=p->x,y=p->y;p->x+=10;stage_move(p,x,y);p->pose=POSE_WALK;p->facing=1;p->animation=(p->animation+1)%16;}
    }
    /* These are fleeing movie extras, not another combat wave. */
    runners_tick(c,a);
    unsigned expected=frame<15?0:1+(frame-15)/6;if(expected>11)expected=11;
    if(c->war.runner_spawned<expected){
        for(int i=0;i<7;i++){Fighter *p=i==6?&c->ally:&c->enemies[i];if(p->ready)continue;
            *p=(Fighter){.x=progress(a)+242,.y=280+(int)(c->war.runner_spawned%3)*14,.hp=50,.ready=1,.facing=-1,.pose=POSE_WALK};
            c->war.runner_mask|=1u<<i;c->war.runner_spawned++;break;
        }
    }
    if(frame==113)c->war.events|=WAR_EVENT_BOMB;
    if(frame==128)for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){a->p[h].vz=16;a->p[h].z=1;a->p[h].pose=POSE_AIRHURT;a->p[h].animation=0;a->p[h].burn=ARENA_BURN_TICKS;}
    if(frame>=128)for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){
        Fighter *p=&a->p[h];if(p->z){p->z+=p->vz;p->vz-=2;if(p->z<=0){p->z=p->vz=0;p->pose=POSE_LAND;p->animation=0;}}
        else {p->pose=POSE_IDLE;p->animation=0;}if(p->burn&&(frame&1))p->burn--; /* Source119-frame effect, six-bit snapshot clock. */
    }
}

void campaign_war_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(a->map_id);
    c->war.events=0;
    if(c->phase==WAR_COMPLETE)return;
    if(c->phase==WAR_HORSE_CINEMA||c->phase==WAR_MACHINE_INTRO||c->phase==WAR_BRIDGE_CINEMA){
        a->tick++;c->timer++;
        if(c->phase==WAR_HORSE_CINEMA)horse_tick(c,a);else {runners_tick(c,a);campaign_cinematic_tick(c,a);}
        if(c->phase==WAR_MACHINE_INTRO&&c->timer==45)machine_spawn(c,a);
        /* Original movie364 frame45 sets speed_x=2 source pixels. The
           machine must approach while the camera introduces it, not sit
           offscreen until control returns. */
        if(c->phase==WAR_MACHINE_INTRO&&c->timer>45&&c->war.machine.ready){c->war.machine.x--;c->war.machine.animation=(c->timer-45)%30;}
        /* DS timing adaptation: retain ambush128 and the full landing, then
           release control instead of holding the static source camera tail. */
        unsigned duration=c->phase==WAR_HORSE_CINEMA?WAR_HORSE_RELEASE_TICKS:c->phase==WAR_MACHINE_INTRO?126:46;
        if(c->timer>=duration){
            if(c->phase==WAR_MACHINE_INTRO)c->phase=WAR_MACHINE_FIGHT;
            else {c->war.flags|=c->phase==WAR_HORSE_CINEMA?WAR_FLAG_HORSE_DONE:WAR_FLAG_BRIDGE_STORY_DONE;c->phase=WAR_TRAVEL;}
            c->timer=0;
        }
        return;
    }
    uint16_t before[6];for(int i=0;i<6;i++)before[i]=c->enemies[i].hit_seq;
    if(c->phase==WAR_FIGHT)c->timer++;
    /* Keep cinematic extras visible and exiting, but outside shared combat,
       collision, XP and wave targeting. Restore before rendering/snapshot. */
    unsigned runners=c->war.runner_mask;
    for(int i=0;i<7;i++)if(runners&(1u<<i))(i==6?&c->ally:&c->enemies[i])->ready=0;
    campaign_combat_tick(c,a,keys);
    for(int i=0;i<7;i++)if(runners&(1u<<i))(i==6?&c->ally:&c->enemies[i])->ready=1;
    runners_tick(c,a);sleep_after_hit(c,before);props(c,a);allies(c,a);
    if(c->phase==WAR_MACHINE_FIGHT){
        for(int h=0;h<2;h++)if(a->p[h].ready){if(a->p[h].x<WAR_MACHINE_LEASH_LEFT)place(&a->p[h],a,WAR_MACHINE_LEASH_LEFT,a->p[h].y,0);if(a->p[h].x>WAR_MACHINE_LEASH_RIGHT)place(&a->p[h],a,WAR_MACHINE_LEASH_RIGHT,a->p[h].y,0);}
        machine_tick(c,a);return;
    }
    if(c->phase==WAR_MACHINE_DEATH){
        c->timer++;c->war.machine.animation=c->timer;
        if(c->timer>=188){c->war.machine.ready=0;c->war.flags|=WAR_FLAG_MACHINE_DONE;c->phase=WAR_TRAVEL;c->timer=0;}
        return;
    }
    if(c->phase==WAR_FIGHT){
        fill_wave(c,a);unsigned count=a->map_id==BRIDGE_MAP?3:war_waves[c->group].count;
        if(c->spawned==count&&!living(c)&&!c->war.wrestle_pending){c->group++;c->phase=WAR_TRAVEL;c->timer=0;}
        else if(a->map_id==WAR_MAP){const WarWave *w=&war_waves[c->group];for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].x>w->right)place(&a->p[h],a,w->right,a->p[h].y,0);}
        return;
    }
    int x=progress(a);
    if(living(c)||c->war.wrestle_pending)return;
    if(a->map_id==BRIDGE_MAP){
        if(!c->group&&x>=bridge_waypoints[1].x){begin_wave(c,a);return;}
        if(c->group&&!(c->war.flags&WAR_FLAG_BRIDGE_STORY_DONE)&&x>=bridge_waypoints[2].x){c->phase=WAR_BRIDGE_CINEMA;c->timer=0;}
        else if((c->war.flags&WAR_FLAG_BRIDGE_STORY_DONE)&&x>=804){c->phase=WAR_COMPLETE;c->completed|=2;}
        return;
    }
    /* The original game never takes control to auto-run the heroes here; the ambush movie is not played. */
    if(!(c->war.flags&WAR_FLAG_HORSE_DONE)&&c->group>=5&&x>=war_waypoints[10].x)c->war.flags|=WAR_FLAG_HORSE_DONE;
    if(!(c->war.flags&WAR_FLAG_MACHINE_DONE)&&c->group>=5&&x>=war_waypoints[12].x){c->phase=WAR_MACHINE_INTRO;c->timer=0;return;}
    if(c->group<WAR_WAVE_COUNT&&x>=war_waypoints[war_waves[c->group].waypoint].x){begin_wave(c,a);return;}
    if(!c->war.gate_hp)c->war.flags|=WAR_FLAG_GATE_OPEN;
    if(!(c->war.flags&WAR_FLAG_GATE_OPEN)){for(int h=0;h<2;h++)if(a->p[h].x>WAR_GATE_X-25)place(&a->p[h],a,WAR_GATE_X-25,a->p[h].y,0);}
    else if(c->group==WAR_WAVE_COUNT&&x>=war_waypoints[21].x+50)campaign_war_start(c,a,BRIDGE_MAP);
}
const char *campaign_war_objective(const Campaign *c,const Arena *a){
    switch(c->phase){case WAR_FIGHT:return "Defeat the attackers";case WAR_HORSE_CINEMA:return "Barbarian ambush";case WAR_MACHINE_INTRO:return "A war machine approaches";case WAR_MACHINE_FIGHT:return "Destroy the war machine";case WAR_MACHINE_DEATH:return "The war machine is destroyed";case WAR_BRIDGE_CINEMA:return "The bridge";case WAR_COMPLETE:return "Barbarian War complete";default:return a->map_id==BRIDGE_MAP?"Continue across the bridge":"Continue through Barbarian War";}
}











