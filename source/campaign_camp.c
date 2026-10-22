#pragma GCC optimize("Os") /* Once-per-tick encounter logic; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_camp.h"
#include "stage.h"
#include "campaign_scene.h"
#include "species.h"
#include "pets.h"
#include "campaign_weapons.h"
#include <stdlib.h>
#include <string.h>

/* LEVEL20 f_Main: the waypoint of each encounter. Order of play: WP4 huts, WP6 bears, WP7 boss, WP9 propeller bears, WP14 bears from the left,
   WP20 bats, then the exit door at WP21. */
static const uint8_t event_wp[CAMP_EVENTS]={4,6,7,9,14,19,20,21};
int campaign_camp_event_x(unsigned event){return camp_waypoints[event_wp[event<CAMP_EVENTS?event:CAMP_EVENTS-1]][0];}
enum {K_BEAR=0,K_BOSS,K_BAT,K_PROP,K_MAGE2,K_MAGE3};
typedef struct {int16_t dx,y;uint8_t kind,coop;uint16_t delay;} CampSpawn;
/* Spawn offsets are from the waypoint (dx) and rows (y; 0 means the waypoint row). Source positions are camera relative (right edge plus 100 to 350). */
static const CampSpawn huts[]={{306,263,K_BEAR,0,0},{383,248,K_BEAR,0,20},{384,311,K_BEAR,0,40},{452,287,K_BEAR,0,60}}; /* spawn1..4, the four huts at x 1472..1618 */
/* ROW, PROPELLER and LEFT offsets are beyond the screen edge (positive: right edge, negative: left edge), as the source places them relative to _root.main. */
static const CampSpawn row[]={{50,0,K_MAGE2,0,0},{125,0,K_MAGE2,0,0},{125,0,K_MAGE2,0,0},{175,0,K_MAGE2,1,30}}; /* level 2: magicians */
/* WP9: two beefy bears fly in on propellers (80 and 60 ticks), then two bears arrive. */
static const CampSpawn propeller[]={{40,0,K_PROP,0,0},{100,25,K_PROP,0,0},{-125,0,K_MAGE3,0,60},{-50,0,K_MAGE3,0,60}};
static const CampSpawn leftside[]={{-50,0,K_MAGE3,0,0},{-100,0,K_MAGE3,0,0},{-125,0,K_MAGE3,1,40}}; /* level 3: wind balls and tornadoes */
/* WP20: seven bats from the right, y WP20 -10, +20, -20, +30, 0, -15, +15 in source units. */
static const CampSpawn bats[]={{250,-5,K_BAT,0,0},{275,10,K_BAT,0,0},{300,-10,K_BAT,0,0},{325,15,K_BAT,0,0},{338,0,K_BAT,0,0},{360,-8,K_BAT,0,0},{385,8,K_BAT,0,0}};
#define BOSS_INTRO_TICKS 120
#define BAT_Z 25
enum {M_NONE=0,M_PROP,M_BOSS,M_MAGE2,M_MAGE3}; /* Campaign.flood.mode for camp enemies */
static int living(const Campaign *c){int n=0;for(int i=0;i<6;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static int progress(const Arena *a){int x=0;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&a->p[h].x>x)x=a->p[h].x;return x;}
static int clear_of_heroes(const Arena *a,int x,int y){for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(x-a->p[h].x)<56&&abs(y-a->p[h].y)<28)return 0;return 1;}
static int place(Fighter *p,const Arena *a,int x,int y,int safe){
    for(int dx=0;dx<=224;dx+=16)for(int side=1;side>=-1;side-=2){
        int xx=x+side*dx;if(xx<0||xx>=CAMP_WIDTH)continue;
        for(int dy=0;dy<160;dy++)for(int sy=-1;sy<=1;sy+=2){
            int yy=y+sy*dy;if(yy<0||yy>=448)continue;
            if(stage_position_open_map(a->map_id,xx,yy)&&(!safe||clear_of_heroes(a,xx,yy))){p->x=xx;p->y=yy;return 1;}
        }
    }
    return 0;
}
static int spawn(Campaign *c,Arena *a,int x,int y,unsigned kind){
    int i=0;while(i<6&&c->enemies[i].ready)i++;if(i==6)return 0;
    int solo=!a->p[1].ready;
    Fighter p={.hp=solo?50:60,.mana=40,.ready=1,.facing=-1,.invulnerable=8,.knight_id=1};
    unsigned species=SPECIES_BEAR;
    if(kind==K_BOSS)p.hp=solo?190:275; /* source 100 base + 100 for f_BadDude + 150 per player, at the 0.55 scale of the other bears */
    if(kind==K_PROP)p.hp=solo?100:120; /* beefy */
    if(kind==K_BAT){p.hp=40;p.knight_id=2;species=SPECIES_BAT;} /* knight_id 2 keeps the shared AI off: the bat flies its own line */
    if(!place(&p,a,x,y,kind!=K_BAT))return 0;
    if(kind==K_BAT){p.x=x;p.y=y;p.z=BAT_Z;}
    c->enemies[i]=p;c->species[i]=(uint8_t)species;c->targets[i]=0;c->combat.death_age[i]=0;
    c->flood.mode[i]=M_NONE;c->flood.timer[i]=0;c->flood.hits[i]=0;
    if(kind==K_MAGE2||kind==K_MAGE3){c->flood.mode[i]=kind==K_MAGE2?M_MAGE2:M_MAGE3;c->flood.aux[i]=0;c->flood.timer[i]=(uint8_t)(60+c->rng%60);if(kind==K_MAGE3)p.hp+=p.hp/4;c->enemies[i].hp=p.hp;}
    if(kind==K_BOSS){c->flood.mode[i]=M_BOSS;c->flood.aux[i]=0;c->flood.timer[i]=(uint8_t)(60+c->rng%60);c->flood.tx[i]=(int16_t)p.x;c->flood.ty[i]=(int16_t)p.y;}
    if(kind==K_PROP){c->flood.mode[i]=M_PROP;c->flood.timer[i]=(uint8_t)(i&1?60:80);c->flood.aux[i]=c->flood.timer[i];p.z=80;c->enemies[i].z=80;c->enemies[i].invulnerable=c->flood.timer[i]+2;}
    c->combat.windup[i]=c->combat.recovery[i]=c->combat.guard[i]=0;
    c->combat.shot_wait[i]=c->combat.ranged[i]=c->combat.target_clock[i]=0;c->combat.observed_attack[i]=0;
    return i+1;
}
static const CampSpawn *table(unsigned event,unsigned *count){
    switch(event){
    case CAMP_EV_HUTS:*count=sizeof huts/sizeof *huts;return huts;
    case CAMP_EV_ROW:*count=sizeof row/sizeof *row;return row;
    case CAMP_EV_PROPELLER:*count=sizeof propeller/sizeof *propeller;return propeller;
    case CAMP_EV_LEFT:*count=sizeof leftside/sizeof *leftside;return leftside;
    case CAMP_EV_BATS:*count=sizeof bats/sizeof *bats;return bats;
    default:*count=0;return 0;
    }
}
/* c->spawned counts entries already placed; entries for a second player advance it when there is none. */
static void feed(Campaign *c,Arena *a){
    unsigned count;const CampSpawn *t=table(c->group,&count);
    int wx=campaign_camp_event_x(c->group);
    while(c->spawned<count){
        const CampSpawn *s=&t[c->spawned];
        if(s->coop&&!a->p[1].ready){c->spawned++;continue;}
        int y=s->y>240?s->y:camp_waypoints[event_wp[c->group]][1]+s->y;
        int x=c->group==CAMP_EV_HUTS?s->dx+1166:wx+s->dx;
        if(c->group==CAMP_EV_ROW||c->group==CAMP_EV_PROPELLER||c->group==CAMP_EV_LEFT){
            int right=progress(a),left=right;for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&a->p[h].x<left)left=a->p[h].x;
            x=s->dx>=0?right+144+s->dx:left-144+s->dx; /* just beyond the screen edge */
            if(x<20)x=20;
        }
        if(c->timer<s->delay||!spawn(c,a,x,y,s->kind))break;
        c->spawned++;
    }
}
static int wave_done(const Campaign *c,const Arena *a){
    unsigned count;const CampSpawn *t=table(c->group,&count);
    for(unsigned i=c->spawned;i<count;i++)if(!t[i].coop||a->p[1].ready)return 0;
    return !living(c);
}
static void begin(Campaign *c,unsigned event){c->group=event;c->spawned=0;c->timer=0;c->phase=WAR_FIGHT;}
void campaign_camp_start(Campaign *c,Arena *a){
    a->map_id=CAMP_MAP;a->round=CAMP_MAP;stage_set_map(CAMP_MAP);a->winner=0;a->running=1;campaign_scene_arena(a);
    memset(&c->war,0,sizeof(c->war));memset(c->enemies,0,sizeof(c->enemies));memset(&c->ally,0,sizeof(c->ally));memset(c->species,0,sizeof(c->species));memset(&c->flood,0,sizeof(c->flood));
    memset(c->drops,0,sizeof(c->drops));memset(c->drop_age,0,sizeof(c->drop_age));c->pending_count=0;
    memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    c->group=0;c->spawned=0;c->kills=0;c->timer=0;c->phase=WAR_TRAVEL;
    c->broken=0;c->break_tick=0;c->npc_ticks=0;c->story_flags&=6;
    memset(c->break_hp,0,sizeof(c->break_hp));memset(c->break_hit_seq,0,sizeof(c->break_hit_seq));
    memset(c->dig_timer,0,sizeof(c->dig_timer));memset(c->item_held,0,sizeof(c->item_held));memset(c->revive,0,sizeof(c->revive));
    memset(c->pickup_timer,0,sizeof(c->pickup_timer));memset(c->targets,0,sizeof(c->targets));c->mount_mask=0;
    memset(c->mount_arc,0,sizeof(c->mount_arc));
    static const int16_t start[2][2]={{CAMP_P1_1_X,CAMP_P1_1_Y},{CAMP_P2_1_X,CAMP_P2_1_Y}};
    for(int h=0;h<2;h++)if(a->p[h].ready){
        campaign_scene_fighter(&a->p[h]);place(&a->p[h],a,start[h][0],start[h][1],0);
        c->pet_followers[h]=(PetFollower){.x=a->p[h].x-25,.y=a->p[h].y,.facing=1};
    }
}
static void keep_in_leash(Campaign *c,Arena *a){
    int left,right;campaign_camp_leash(c,&left,&right);
    for(int h=0;h<2;h++)if(a->p[h].ready){
        if(a->p[h].x>right){int ox=a->p[h].x,oy=a->p[h].y;a->p[h].x=right;stage_move(&a->p[h],ox,oy);}
        if(a->p[h].x<left){int ox=a->p[h].x,oy=a->p[h].y;a->p[h].x=left;stage_move(&a->p[h],ox,oy);}
    }
}
static const Fighter *nearest(const Arena *a,const Fighter *e){
    const Fighter *t=0;int best=100000;
    for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){int d=abs(a->p[h].x-e->x)+abs(a->p[h].y-e->y);if(d<best){best=d;t=&a->p[h];}}
    return t;
}
/* Bear magic (f_SetPlayerMagic helmet 20): a wind ball for level 2, and for level 3 and the boss (magic_splash 2) a coin flip between the ball and the
   tornado (PLAYER sprite 1769), a spin that rushes the target. Wait 60 to 120 ticks between casts (magic_wait, magic_delay 90), 14 ticks of casting.
   One ball flies at a time, in the machine slot of the snapshot. aux: 0 waiting, 1 casting, 2 tornado. */
#define CAST_TICKS 14
#define TORNADO_TICKS 50
static void wind_ball(Campaign *c,const Fighter *p){
    Fighter *m=&c->war.machine;if(m->ready)return;
    *m=(Fighter){.ready=1,.x=p->x+p->facing*14,.y=p->y,.z=20,.hp=1,    .facing=p->facing,.attack_kind=0};c->flood.ball=70;
}
static int magic(Campaign *c,Arena *a,int i){
    Fighter *p=&c->enemies[i];FloodState *f=&c->flood;
    unsigned mode=f->mode[i];
    if(mode!=M_MAGE2&&mode!=M_MAGE3&&mode!=M_BOSS)return 0;
    if(mode==M_BOSS&&c->timer<BOSS_INTRO_TICKS)return 0;
    if(p->hitstun||p->pose==POSE_KNOCKDOWN||p->pose==POSE_AIRHURT||p->z){f->aux[i]=0;if(f->timer[i]<30)f->timer[i]=30;return 0;}
    const Fighter *t=nearest(a,p);if(!t)return 0;
    int can_tornado=mode!=M_MAGE2;Fighter hazard={.facing=p->facing};
    if(f->aux[i]==1){
        p->x=f->tx[i];p->y=f->ty[i];p->pose=POSE_CAST;p->animation=(int16_t)(CAST_TICKS-f->timer[i]);p->cooldown=0;
        if(!--f->timer[i]){
            if(can_tornado&&(c->rng>>9)%2){f->aux[i]=2;f->timer[i]=TORNADO_TICKS;}
            else{wind_ball(c,p);f->aux[i]=0;f->timer[i]=(uint8_t)(90+c->rng%40);}
        }
        return 1;
    }
    if(f->aux[i]==2){
        int ox=p->x,oy=p->y,dir=t->x>p->x?1:-1;
        p->x+=dir*3;if(t->y>p->y+2)p->y+=2;else if(t->y<p->y-2)p->y-=2;stage_move(p,ox,oy);
        p->facing=dir;p->pose=POSE_THROW;p->animation=f->timer[i];p->invulnerable=2;p->cooldown=0;
        for(int h=0;h<2;h++){Fighter *q=&a->p[h];if(q->ready&&q->hp>0&&!q->invulnerable&&abs(q->x-p->x)<24&&abs(q->y-p->y)<10&&q->z<30)arena_campaign_strike(&hazard,q,10,8,dir,0,0);}
        if(!--f->timer[i]){f->aux[i]=0;f->timer[i]=(uint8_t)(90+c->rng%40);p->pose=POSE_IDLE;p->animation=0;p->invulnerable=8;}
        return 1;
    }
    if(f->timer[i])f->timer[i]--;
    if(!f->timer[i]){
        int dx=abs(t->x-p->x),dy=abs(t->y-p->y);
        if(dx>=60&&dx<=220&&dy<14&&!c->war.machine.ready){f->aux[i]=1;f->timer[i]=CAST_TICKS;f->tx[i]=(int16_t)p->x;f->ty[i]=(int16_t)p->y;p->facing=t->x>p->x?1:-1;}
        else f->timer[i]=12;
    }
    return 0;
}
/* The two wooden barricades (LEVEL20 sprite 333 at source (3196,-306) and (3262,-268)) shoot arrows left at heroes approaching from the west
   (the level's f_EnemyAttack fires while the camera is within 600 source units). One arrow flies at a time, in the machine slot (attack_kind 1). */
#define CAMP_ARROW 1
static void barricades(Campaign *c,Arena *a){
    static const int16_t slits[2][2]={{3948,247},{3981,266}};
    Fighter *m=&c->war.machine;if(m->ready||c->phase!=WAR_TRAVEL)return;
    int x=progress(a);if(x>slits[0][0]||x<slits[0][0]-300||a->tick%45!=0)return;
    unsigned i=(c->rng>>10)&1;
    *m=(Fighter){.ready=1,.x=slits[i][0]-10,.y=slits[i][1],.z=14,.hp=1,.facing=-1,.attack_kind=CAMP_ARROW};c->flood.ball=70;
}
static void balls(Campaign *c,Arena *a){
    Fighter *m=&c->war.machine;if(!m->ready)return;
    Fighter hazard={.facing=m->facing};
    m->x+=m->facing*(m->attack_kind==CAMP_ARROW?6:4);m->animation=(m->animation+1)%5;
    if(!stage_position_open_map(a->map_id,m->x,m->y)||!c->flood.ball||!--c->flood.ball){m->ready=0;return;}
    for(int h=0;h<2;h++){Fighter *q=&a->p[h];
        if(q->ready&&q->hp>0&&!q->invulnerable&&abs(q->x-m->x)<10&&abs(q->y-m->y)<8&&q->z<35){arena_campaign_strike(&hazard,q,m->attack_kind==CAMP_ARROW?6:8,m->attack_kind==CAMP_ARROW?0:5,m->facing,0,0);m->ready=0;return;}}
}
/* Enemies that start beyond the screen walk in on their own: the shared fighting AI only engages near its target. Propeller bears fly in
   (source speed_toss_x 13 and 12, propeller_timer 80 and 60) and drop to the ground; bats fly a straight line left at 7 to 9 source units a tick
   and die when they leave the screen (f_BatFlyLeft). */
static void drive(Campaign *c,Arena *a){
    Fighter hazard={.facing=-1};
    for(int i=0;i<6;i++){
        Fighter *p=&c->enemies[i];
        if(!p->ready||p->hp<=0)continue;
        if(c->species[i]==SPECIES_BAT){
            if(p->hitstun||p->pose==POSE_KNOCKDOWN||p->pose==POSE_AIRHURT)continue;
            p->z=BAT_Z;p->vz=0; /* the shared physics would drop it */
            p->x-=2+(int)((c->timer+(unsigned)i)%3==0);p->facing=-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
            if(!stage_position_open_map(a->map_id,p->x,p->y)){ /* keep over the walkable band; where it ends the bat has left the level */
                int found=0;for(int dy=1;dy<=14&&!found;dy++)for(int sy=-1;sy<=1&&!found;sy+=2)if(stage_position_open_map(a->map_id,p->x,p->y+sy*dy)){p->y=(int16_t)(p->y+sy*dy);found=1;}
                if(!found){p->hp=0;p->ready=0;continue;}
            }
            for(int h=0;h<2;h++){Fighter *q=&a->p[h];if(q->ready&&q->hp>0&&!q->invulnerable&&abs(q->x-p->x)<14&&abs(q->y-p->y)<8&&q->z<BAT_Z+10)arena_campaign_strike(&hazard,q,6,5,-1,0,0);}
            if(p->x<progress(a)-230||p->x<4){p->hp=0;p->ready=0;} /* flown off the screen */
            continue;
        }
        if(c->flood.mode[i]==M_PROP){
            const Fighter *t=nearest(a,p);if(!t)continue;
            unsigned total=c->flood.aux[i],left=c->flood.timer[i];
            int dir=t->x>p->x?1:-1;
            if(left>total/3){int ox=p->x;p->x+=dir*3;if(!stage_position_open_map(a->map_id,p->x,p->y))p->x=ox;} /* toss across */
            p->z=(int16_t)(80*(int)left/(int)total);p->vz=0;p->pose=POSE_DIG;p->facing=dir;p->invulnerable=2;p->hitstun=0;
            if(!--c->flood.timer[i]){c->flood.mode[i]=M_NONE;p->z=0;p->pose=POSE_IDLE;p->animation=0;p->invulnerable=8;}
            continue;
        }
        if(magic(c,a,i))continue;
        if(p->z||p->hitstun||p->cooldown||p->pose==POSE_KNOCKDOWN)continue;
        const Fighter *t=nearest(a,p);if(!t)continue;
        int dist=abs(t->x-p->x)+abs(t->y-p->y);if(dist<170)continue;
        int ox=p->x,oy=p->y;p->x+=t->x>p->x?2:-2;if(t->y>p->y+3)p->y+=1;else if(t->y<p->y-3)p->y-=1;
        stage_move(p,ox,oy);p->facing=t->x>ox?1:-1;p->pose=POSE_WALK;p->animation=(p->animation+1)%16;
    }
}
/* Boss fight (sprite 374 and the `boss` spawner, sprite 368). The party first walks to its marks p*_4 while the boss dances (dance3, frames 2 to 121 of the
   clip); then it is released. While the boss lives and fewer than four enemies are alive another bear arrives from the right edge (level 2). */
static void boss_intro(Arena *a,uint16_t keys[2]){
    static const int16_t marks[2][2]={{CAMP_P1_4_X,CAMP_P1_4_Y},{CAMP_P2_4_X,CAMP_P2_4_Y}};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        keys[h]=0;p->invulnerable=2;
        int dx=marks[h][0]-p->x,dy=marks[h][1]-p->y,x=p->x,y=p->y;
        if(abs(dx)>1||abs(dy)>1){p->x+=dx>3?3:dx<-3?-3:dx;p->y+=dy>2?2:dy<-2?-2:dy;stage_move(p,x,y);p->pose=POSE_WALK;p->facing=dx<0?-1:1;p->animation=(p->animation+1)%16;}
        else{p->pose=POSE_IDLE;p->animation=0;p->facing=1;}
    }
}
static void story_walk(Arena *a,uint16_t keys[2]){
    static const int16_t marks[2][2]={{CAMP_P1_5_X,CAMP_P1_5_Y},{CAMP_P2_5_X,CAMP_P2_5_Y}};
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!p->ready||p->hp<=0)continue;
        keys[h]=0;p->invulnerable=2;
        int dx=marks[h][0]-p->x,dy=marks[h][1]-p->y,x=p->x,y=p->y;
        if(abs(dx)>1||abs(dy)>1){p->x+=dx>3?3:dx<-3?-3:dx;p->y+=dy>2?2:dy<-2?-2:dy;stage_move(p,x,y);p->pose=POSE_WALK;p->facing=dx<0?-1:1;p->animation=(p->animation+1)%16;}
        else{p->pose=POSE_IDLE;p->animation=0;p->facing=1;}
    }
}
static void boss_feed(Campaign *c,Arena *a){
    if(c->spawned==0){
        if(spawn(c,a,CAMP_BOSS_X,CAMP_BOSS_Y,K_BOSS)){c->spawned=1;c->war.gate_hp=1;c->war.hit_seq[0]=0;}
        return;
    }
    for(int i=0;i<6;i++)if(c->enemies[i].ready&&c->enemies[i].hp>0&&c->flood.mode[i]==M_BOSS&&c->timer<BOSS_INTRO_TICKS){
        Fighter *p=&c->enemies[i];p->x=c->flood.tx[i];p->y=c->flood.ty[i];p->pose=POSE_CAST;p->animation=(int16_t)(c->timer/12);p->invulnerable=2;p->facing=-1;p->hitstun=0;p->cooldown=0;
    }
    if(c->timer<BOSS_INTRO_TICKS)return;
    int boss_alive=0;for(int i=0;i<6;i++)if(c->flood.mode[i]==M_BOSS&&c->enemies[i].ready&&c->enemies[i].hp>0)boss_alive=1;
    if(!boss_alive){ /* f_CloseBossHealth: the music returns and f_KillEnemies clears the stream */
        for(int i=0;i<6;i++)if(c->enemies[i].ready&&c->enemies[i].hp>0){c->enemies[i].hp=0;c->enemies[i].ready=0;}
        c->war.gate_hp=0;c->war.machine.ready=0;return;
    }
    if(c->war.gate_hp&&living(c)<4&&c->timer%45==0)spawn(c,a,progress(a)+170,CAMP_BOSS_Y+(int)(c->rng>>8)%90,K_MAGE2);
}
/* Animal orbs (LEVEL20 f_Init and f_SetAnimalTimer): Hawkster (id 16) beside the huts once the first bears are down, Rammy (id 3) at animal_pt after the boss,
   Burly Bear (id 6) behind the bombable wall near the end. They appear when the heroes come near and are withdrawn once passed or taken. */
static int owns_pet(const Campaign *c,unsigned id){for(int h=0;h<2;h++)if(pets_owned(&c->pets[h],id))return 1;return 0;}
static int has_drop(const Campaign *c,unsigned kind,unsigned id){for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind==kind&&(kind==DROP_WEAPON?c->drops[d].value==id:(c->drops[d].value&31)==id))return d+1;return 0;}
static int owns_weapon(const Campaign *c,unsigned slot){return weapon_set_has(&c->owned_weapons[0],slot)||weapon_set_has(&c->owned_weapons[1],slot);}
/* Weapon pickups: weapon_type 54 (Lobster, at the start) and 55 (Umbrella) are clips of the scene (sprite 140), weapon 62 (Wrench) sits by Rammy (f_LevelItems). */
static void pickups(Campaign *c,Arena *a){
    static const struct {int16_t x,y;uint8_t kind,id,after;} item[6]={
        {391,346,DROP_WEAPON,WEAPON_LOBSTER,0},{2765,267,DROP_WEAPON,WEAPON_UMBRELLA,0},{2610,288,DROP_WEAPON,WEAPON_WRENCH,CAMP_EV_PROPELLER},
        {1610,288,DROP_PET,PET_HAWKSTER,CAMP_EV_ROW},{2562,288,DROP_PET,PET_RAMMY,CAMP_EV_PROPELLER},{4823,286,DROP_PET,PET_BURLY,CAMP_EV_STORY}};
    int x=progress(a);
    for(int i=0;i<6;i++){
        int slot=has_drop(c,item[i].kind,item[i].id),near=abs(x-item[i].x)<260;
        int owned=item[i].kind==DROP_PET?owns_pet(c,item[i].id):owns_weapon(c,item[i].id);
        if(owned){if(slot){c->drops[slot-1].kind=0;c->drop_age[slot-1]=0;}continue;}
        if(near&&!slot&&c->group>=item[i].after){Fighter spot={0};if(place(&spot,a,item[i].x,item[i].y,0)){if(item[i].kind==DROP_PET)campaign_pet_place(c,item[i].id,spot.x,spot.y,1);else campaign_weapon_place(c,item[i].id,spot.x,spot.y);}}
        if(slot&&abs(x-item[i].x)>600){c->drops[slot-1].kind=0;c->drop_age[slot-1]=0;}
    }
}
void campaign_camp_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    stage_set_map(CAMP_MAP);
    if(c->phase==WAR_COMPLETE)return;
    if(c->phase==WAR_FIGHT)c->timer++;
    c->rng=c->rng*1664525u+1013904223u;
    uint16_t k2[2]={keys[0],keys[1]};
    int cinema=c->phase==WAR_FIGHT&&c->group==CAMP_EV_BOSS&&c->timer<BOSS_INTRO_TICKS;
    int story=c->phase==WAR_FIGHT&&c->group==CAMP_EV_STORY;
    if(cinema)boss_intro(a,k2);
    if(story)story_walk(a,k2);
    campaign_combat_tick(c,a,k2);
    keep_in_leash(c,a);
    pickups(c,a);
    if(!cinema&&!story){drive(c,a);barricades(c,a);balls(c,a);}
    int x=progress(a);
    if(c->phase==WAR_FIGHT){
        if(c->group==CAMP_EV_BOSS){
            boss_feed(c,a);
            if(c->spawned&&!living(c)){c->phase=WAR_TRAVEL;c->group=CAMP_EV_PROPELLER;c->spawned=0;c->timer=0;c->war.gate_hp=0;}
            return;
        }
        if(story){if(c->timer>=CAMP_STORY_TICKS){c->phase=WAR_TRAVEL;c->group=CAMP_EV_BATS;c->spawned=0;c->timer=0;}return;}
        feed(c,a);
        if(wave_done(c,a)){c->phase=WAR_TRAVEL;c->group++;c->spawned=0;c->timer=0;}
        return;
    }
    if(living(c)&&c->group!=CAMP_EV_EXIT)return;
    if(c->group<CAMP_EV_EXIT&&x>=campaign_camp_event_x(c->group))begin(c,c->group);
    else if(c->group==CAMP_EV_EXIT&&x>=CAMP_DOOR3_X-30){c->phase=WAR_COMPLETE;c->completed|=256;}
}
unsigned campaign_camp_boss_music(const Campaign *c){return c->group==CAMP_EV_BOSS&&c->phase==WAR_FIGHT&&c->spawned;}
const char *campaign_camp_objective(const Campaign *c){
    if(c->phase==WAR_COMPLETE)return "Tall Grass Field complete";
    if(c->phase==WAR_FIGHT)return c->group==CAMP_EV_BOSS?"Defeat the bear boss":c->group==CAMP_EV_BATS?"Beat back the bats":"Defeat the bears";
    return c->group>=CAMP_EV_EXIT?"Reach the exit door":"Press on";
}
