#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign.h"
#include "campaign_forest.h"
#include "campaign_deer.h"
#include "campaign_flood.h"
#include "campaign_camp.h"
#include "campaign_thieves.h"
#include "species.h"
#include "item_selector.h"
#include "campaign_combat.h"
#include "cinematic.h"
#include "stage.h"
#include <string.h>
#include <stdlib.h>
int campaign_max_health(unsigned level){return 97+3*(level>=1&&level<=99?level:1);}
/* MAIN ReadStorage restores four base attributes and derives unspent points:
   total=level*2+2 through20, then42+(level-20), capped100. Base starts1. */
unsigned campaign_stat_budget(unsigned level){unsigned total=level<=20?level*2+2:42+level-20;return (total>100?100:total)-4;}
unsigned campaign_points(const Campaign *c,unsigned h){
    if(h>1)return 0;
    unsigned used=0;for(int i=0;i<4;i++)used+=c->stats[h][i];
    unsigned budget=campaign_stat_budget(c->profiles[h].level?c->profiles[h].level:1);
    return used<budget?budget-used:0;
}
ArenaCampaignAttributes campaign_attributes(const Campaign *c,unsigned h){
    const CampaignWeapon *w=campaign_weapon_info(c->weapons[h]);if(!w)w=campaign_weapon_info(0);
    const PetInfo *pet=pets_info(c->pets[h].equipped);
    return (ArenaCampaignAttributes){campaign_weapon_stat(1+c->stats[h][0]+(pet?pet->strength:0),w->strength),campaign_weapon_stat(1+c->stats[h][1],w->defense),campaign_weapon_stat(1+c->stats[h][2],w->magic),campaign_weapon_stat(1+c->stats[h][3],w->agility),w->critical_divisor};
}
int campaign_health(const Campaign *c,unsigned h){ArenaCampaignAttributes v=campaign_attributes(c,h);return 69+28*v.defense+3*(c->profiles[h].level?c->profiles[h].level:1);}
int campaign_mana(const Campaign *c,unsigned h){return 35+5*campaign_attributes(c,h).magic;}
int campaign_equip(Campaign *c,Arena *a,unsigned h,unsigned slot){
    if(h>1||!a->p[h].ready||!a->p[h].hp||!campaign_weapon_can_equip(&c->owned_weapons[h],slot,c->profiles[h].level))return 0;
    c->weapons[h]=slot;a->p[h].equip_timer=30;
    if(a->p[h].hp>campaign_health(c,h))a->p[h].hp=campaign_health(c,h);
    if(a->p[h].mana>campaign_mana(c,h))a->p[h].mana=campaign_mana(c,h);
    return 1;
}
int campaign_spend(Campaign *c,Arena *a,unsigned h,unsigned stat){
    if(h>1||stat>3||!a->p[h].ready||!campaign_points(c,h)||c->stats[h][stat]>=24)return 0;
    c->stats[h][stat]++;return 1; /* Raising maximum health never heals for free. */
}
/* Original LEVEL13 waypoint x coordinates, translated to half-scale world. */
static const int encounter_x[4]={759,868,1542,1954};
static int living_heroes(const Arena *a){return !!a->p[0].hp+(a->p[1].ready&&a->p[1].hp>0);}
static int living_enemies(const Campaign *c){int n=0;for(int i=0;i<CAMPAIGN_ENEMIES;i++)n+=c->enemies[i].ready&&c->enemies[i].hp>0;return n;}
static void place(Fighter *p,int x,int y,unsigned map){
    /* Choose a legal foot point even on the original stair profiles. */
    if(!stage_position_open_map(map,x,y)){
        for(int radius=0;radius<300;radius++)for(int side=-1;side<=1;side+=2){
            int yy=y+radius*side;
            if(yy>=0&&yy<stage_world_height(map)&&stage_position_open_map(map,x,yy)){p->x=x;p->y=yy;return;}
        }
        for(int xx=ARENA_MIN_X;xx<stage_world_width(map)-33;xx++)for(int yy=0;yy<stage_world_height(map);yy++)
            if(stage_position_open_map(map,xx,yy)){p->x=xx;p->y=yy;return;}
    }
    p->x=x;p->y=y;
}
/* A delayed co-op partner may already stand on the original hidden spawn.
 * Keep its anchor normally, otherwise choose a nearby legal entrance point
 * beyond both heroes' immediate melee/body area. Never relocate either hero. */
static int spawn_clear(const Arena *a,int x,int y){
    for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0&&abs(a->p[h].x-x)<56&&abs(a->p[h].y-y)<28)return 0;
    return 1;
}
static int safe_princess_spawn(Fighter *p,const Arena *a){
    if(spawn_clear(a,p->x,p->y))return 1;
    for(int distance=16;distance<=224;distance+=16)for(int side=1;side>=-1;side-=2){
        int x=p->x+distance*side;
        if(stage_position_open_map(a->map_id,x,p->y)&&spawn_clear(a,x,p->y)){p->x=x;return 1;}
    }
    return 0; /* Retry next tick if both entrances are occupied. */
}
static int spawn_enemy(Campaign *c,Arena *a,int ordinal,int ladder){
    int slot=0;while(slot<CAMPAIGN_ENEMIES&&c->enemies[slot].ready)slot++;
    if(slot==CAMPAIGN_ENEMIES)return 0;
    Fighter *p=&c->enemies[slot];
    c->combat.death_age[slot]=0;c->combat.windup[slot]=c->combat.recovery[slot]=c->combat.guard[slot]=0;
    c->combat.shot_wait[slot]=c->combat.ranged[slot]=c->combat.target_clock[slot]=0;c->combat.observed_attack[slot]=0;c->targets[slot]=0;
    *p=(Fighter){.hp=a->p[1].ready?KEEP_BARBARIAN_HP_COOP:KEEP_BARBARIAN_HP_SOLO,.mana=ARENA_MANA,.facing=-1,.ready=1,.invulnerable=8};
    /* LEVEL13 explicit spawn calls at WP4,WP6,WP9,WP12; half source scale.
       Ladder movie32/33 placements include their source +50/+100 Y offsets. */
    static const int16_t spawn[4][4][2]={
        {{869,364},{919,364},{969,401},{0,0}},
        {{1198,373},{1248,373},{1298,410},{0,0}},
        {{1543,368},{1533,365},{1553,365},{0,0}},
        {{2292,237},{2117,237},{2023,176},{2146,177}}
    };
    place(p,spawn[c->group][ordinal][0],spawn[c->group][ordinal][1],a->map_id);
    if(c->group==2&&!safe_princess_spawn(p,a)){p->ready=0;return 0;}
    if(ladder){p->z=ordinal==2?KEEP_LADDER_ONE_HEIGHT:KEEP_LADDER_TWO_HEIGHT;p->pose=POSE_JUMP;}
    c->spawned++;return 1;
}
static void begin_group(Campaign *c,Arena *a){
    /* Previous bodies retain their own blink clock while a new group enters. */
    c->spawned=0;c->timer=0;c->phase=KEEP_FIGHT;
    int n=c->group==3?2:3;
    while(c->spawned<n&&spawn_enemy(c,a,c->spawned,0)){}
}
static void enter_store(Campaign *c,Arena *a){
    /* LEVEL19 origin(400,-600), half scale; source waypoint6 fight actors. */
    static const int16_t spawn[6][2]={{639,274},{589,294},{664,309},{689,279},{739,319},{759,274}};
    a->map_id=4;a->round=4;stage_set_map(4);c->phase=KEEP_STORE_FIGHT;c->timer=0;c->spawned=6;
    memset(c->enemies,0,sizeof(c->enemies));memset(c->drops,0,sizeof(c->drops));c->pending_count=0;memset(a->shots,0,sizeof(a->shots));
    campaign_combat_reset(&c->combat);memset(c->targets,0,sizeof(c->targets));
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];p->z=p->vz=p->cooldown=p->hitstun=p->knockback=0;p->pose=POSE_IDLE;p->animation=0;p->attack_kind=HIT_NONE;p->queued=0;p->guard=0;
        place(p,h?158:178,h?159:144,4);
    }
    for(int i=0;i<6;i++){
        c->enemies[i]=(Fighter){.hp=a->p[1].ready?60:50,.mana=40,.ready=1,.facing=-1,.invulnerable=8};
        place(&c->enemies[i],spawn[i][0],spawn[i][1],4);
    }
    c->ally=(Fighter){.hp=60,.mana=40,.ready=1,.facing=1};place(&c->ally,514,294,4);
}
void campaign_start(Campaign *c,Arena *a,int coop){
    unsigned seed=a->tick^c->rng^0x735ac917u;unsigned ids[2]={a->p[0].knight_id,a->p[1].knight_id};CampaignProfile saved[2];memcpy(saved,c->profiles,sizeof(saved));
    PetProfile pets[2]={c->pets[0],c->pets[1]};uint8_t tools[2]={c->tools[0],c->tools[1]};uint64_t completed=c->completed;
    uint8_t stats[2][4],potions[2],ack[2];memcpy(stats,c->stats,sizeof(stats));memcpy(potions,c->potions,2);memcpy(ack,c->menu_ack,2);
    WeaponSet owned[2]={c->owned_weapons[0],c->owned_weapons[1]};uint8_t weapons[2]={c->weapons[0],c->weapons[1]};
    arena_init(a);a->map_id=CAMPAIGN_BARRACKS_MAP;a->running=1;a->round=CAMPAIGN_BARRACKS_MAP;stage_set_map(a->map_id);
    memset(c,0,sizeof(*c));c->completed=completed;c->phase=KEEP_BARRACKS_INTRO;c->rng=seed?seed:1;c->break_hp[0]=4;for(int b=1;b<=5;b++)c->break_hp[b]=3;
    for(int i=0;i<2;i++){
        a->p[i].knight_id=ids[i];a->p[i].ready=i==0||coop;place(&a->p[i],i?318:336,i?261:285,a->map_id);
        c->profiles[i]=saved[i].level>=1&&saved[i].level<=99&&saved[i].knight==ids[i]?saved[i]:(CampaignProfile){.level=1,.knight=ids[i]};
        if(saved[i].level&&saved[i].knight==ids[i]){memcpy(c->stats[i],stats[i],4);c->potions[i]=potions[i];c->pets[i]=pets[i];c->tools[i]=tools[i];}c->menu_ack[i]=ack[i];
        c->owned_weapons[i]=saved[i].level&&saved[i].knight==ids[i]?owned[i]:(WeaponSet){0};weapon_set_add(&c->owned_weapons[i],0);
        c->weapons[i]=campaign_weapon_can_equip(&c->owned_weapons[i],weapons[i],c->profiles[i].level)?weapons[i]:0;
        a->p[i].hp=campaign_health(c,i);a->p[i].mana=campaign_mana(c,i);
        c->pet_followers[i]=(PetFollower){.x=a->p[i].x-25,.y=a->p[i].y,.facing=1};
    }
}
static void enter_keep(Campaign *c,Arena *a){
    a->map_id=CAMPAIGN_MAP;a->round=CAMPAIGN_MAP;stage_set_map(a->map_id);
    c->phase=KEEP_INTRO;c->timer=0;c->npc_ticks=0;
    memset(c->drops,0,sizeof(c->drops));c->pending_count=0;memset(a->shots,0,sizeof(a->shots));campaign_combat_reset(&c->combat);
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];p->z=p->vz=p->cooldown=p->hitstun=p->knockback=0;p->pose=POSE_IDLE;p->animation=0;p->attack_kind=HIT_NONE;p->queued=0;p->guard=0;
        place(p,110+h*35,239+h*20,a->map_id);
    }
}
static void give_xp(Campaign *c,Arena *a,int hero,unsigned hits){
    CampaignProfile *p=&c->profiles[hero];p->xp+=hits*KEEP_XP_PER_HIT;if(p->xp>131071)p->xp=131071;
    while(p->level<KEEP_MAX_LEVEL&&p->xp>keep_xp_threshold(p->level)){
        p->level++;a->p[hero].hp=campaign_health(c,hero);a->p[hero].mana=campaign_mana(c,hero);
    }
}
void campaign_award_hit(Campaign *c,Arena *a,unsigned hero){if(hero<2)give_xp(c,a,hero,1);}
static int drop(Campaign *c,const Fighter *enemy,unsigned kind,unsigned value){
    int slot=-1;for(int i=0;i<CAMPAIGN_DROPS;i++)if(!c->drops[i].kind){slot=i;break;}
    if(slot<0)return 0;
    c->drop_age[slot]=0;
    c->drops[slot]=(CampaignDrop){.x=enemy->x,.y=enemy->y,.kind=kind,.value=value};
    return 1;
}
int campaign_pet_place(Campaign *c,unsigned id,int x,int y,int unlock){
    if(!pets_info(id))return 0;
    Fighter origin={.x=x,.y=y};return drop(c,&origin,DROP_PET,id|(unlock?32:0));
}
int campaign_bag_place(Campaign *c,int x,int y){Fighter origin={.x=x,.y=y};return drop(c,&origin,DROP_BAG,5);}
int campaign_croc_place(Campaign *c,int x,int y,int facing){Fighter origin={.x=x,.y=y};return drop(c,&origin,DROP_CROC,facing<0);}
int campaign_weapon_place(Campaign *c,unsigned slot,int x,int y){
    if(slot>=WEAPON_COUNT)return 0;
    Fighter origin={.x=x,.y=y};return drop(c,&origin,DROP_WEAPON,slot);
}
int campaign_dig_buddy(Campaign *c,const Arena *a,unsigned hero,int completed){
    if(hero>=2||a->map_id!=5||!completed||!(c->tools[hero]&1)||!a->p[hero].ready||!a->p[hero].hp||
       abs(a->p[hero].x-155)>=25||abs(a->p[hero].y-286)>=10||(c->story_flags&128))return 0;
    if(!campaign_pet_place(c,PET_MR_BUDDY,155,286,1))return 0;
    c->story_flags|=128;return 1;
}
static unsigned random_below(Campaign *c,unsigned limit);
/* MAIN f_OwletTimer/f_AnimalGrabFruit: every 30 frames an Owlet looks for an unpicked registered tree on screen,
   flies to it at speed 6 (3 DS px), takes a fruit (random(5) food) and brings it to its owner. */
static void owlet_fly(PetFollower *f,int x,int y){
    int dx=x-f->x,dy=y-f->y,d=abs(dx)>abs(dy)?abs(dx):abs(dy);
    if(d<=3){f->x=(int16_t)x;f->y=(int16_t)y;return;}
    f->x=(int16_t)(f->x+dx*3/d);f->y=(int16_t)(f->y+dy*3/d);f->facing=dx<0?-1:1;
}
static void owlet_step(Campaign *c,Arena *a){
    for(unsigned h=0;h<2;h++){
        const Fighter *p=&a->p[h];PetFollower *f=&c->pet_followers[h];
        if(a->map_id!=FOREST_MAP||!p->ready||!p->hp||c->pets[h].equipped!=PET_OWLET){c->owlet_state[h]=0;c->owlet_timer[h]=0;continue;}
        if(c->owlet_state[h]==0){
            if(++c->owlet_timer[h]>30){
                c->owlet_timer[h]=0;
                if(!c->fruit_taken&&abs(FOREST_TREE_X-p->x)<212&&abs(FOREST_TREE_Y-p->y)<120)c->owlet_state[h]=1;
            }
        }else if(c->owlet_state[h]==1){
            if(c->fruit_taken){c->owlet_state[h]=0;continue;}
            owlet_fly(f,FOREST_TREE_X,FOREST_TREE_Y-20);
            if(abs(f->x-FOREST_TREE_X)<=3&&abs(f->y-(FOREST_TREE_Y-20))<=3){c->fruit_taken=1;c->owlet_state[h]=2;}
        }else{
            int gx=p->x-p->facing*25,gy=p->y;owlet_fly(f,gx,gy);
            if(abs(f->x-gx)<=3&&abs(f->y-gy)<=3){
                Fighter origin={.x=p->x+p->facing*20,.y=p->y};
                drop(c,&origin,DROP_FOOD,10|(random_below(c,5)<<5));
                c->owlet_state[h]=0;
            }
        }
    }
}
/* MAIN f_TrollPetTimer: every 300 ticks the Troll heals its owner by one percent of maximum health. */
static void troll_step(Campaign *c,Arena *a){
    for(unsigned h=0;h<2;h++){
        Fighter *p=&a->p[h];
        if(c->pets[h].equipped!=PET_TROLL||!p->ready||p->hp<=0){c->troll_timer[h]=0;continue;}
        if(++c->troll_timer[h]>300){
            c->troll_timer[h]=0;int maximum=campaign_health(c,h),gain=maximum/100;if(gain<1)gain=1;
            if(p->hp<maximum-1){
                int before=p->hp;p->hp+=gain;if(p->hp>maximum)p->hp=maximum;
                /* The same +N pop-up and eating sound as food, so the pet's healing is noticed. */
                c->pickup_kind[h]=DROP_FOOD;c->pickup_value[h]=(uint8_t)(p->hp-before);c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;
            }
        }
    }
}
/* Animal orbs with a strike (LEVEL20 f_SetAnimalTimer): Rammy rams the enemy in front of it every 60 ticks, Hawkster pecks every 75, Burly Bear mauls every 90. */
static void pet_attack_step(Campaign *c,Arena *a){
    for(unsigned h=0;h<2;h++){
        unsigned id=c->pets[h].equipped,period=id==PET_RAMMY?60:id==PET_HAWKSTER?75:id==PET_BURLY?90:0;
        const Fighter *owner=&a->p[h];
        if(!period||!owner->ready||owner->hp<=0){c->pet_timer[h]=0;continue;}
        if(++c->pet_timer[h]<period)continue;
        const PetFollower *f=&c->pet_followers[h];int reach=id==PET_RAMMY?70:id==PET_HAWKSTER?55:45,damage=id==PET_RAMMY?12:id==PET_HAWKSTER?8:16,best=-1,near=1000;
        for(int i=0;i<CAMPAIGN_ENEMIES;i++){
            const Fighter *e=&c->enemies[i];if(!e->ready||e->hp<=0||e->z>20||e->pose==POSE_KNOCKDOWN)continue;
            int dx=(e->x-f->x)*(owner->facing<0?-1:1);
            if(id==PET_RAMMY?(dx<4||dx>reach):abs(dx)>reach)continue;
            if(abs(e->y-f->y)>(id==PET_RAMMY?20:28))continue;
            if(abs(dx)<near){near=abs(dx);best=i;}
        }
        if(best<0){c->pet_timer[h]=(uint8_t)(period-1);continue;} /* look again next tick */
        c->pet_timer[h]=0;
        Fighter attacker=*owner;attacker.attack_kind=HIT_NONE;
        arena_campaign_strike(&attacker,&c->enemies[best],damage,id==PET_RAMMY?4:0,c->enemies[best].x>=f->x?1:-1,0,0);
    }
}
static void collect_pets(Campaign *c,Arena *a){
    unsigned alive=(a->p[0].ready&&a->p[0].hp?1:0)|(a->p[1].ready&&a->p[1].hp?2:0);
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind==DROP_PET){
        CampaignDrop *v=&c->drops[d];PetDrop pet={.x=v->x,.y=v->y,.source_id=v->value&31,.ignore_timer=c->drop_age[d],.unlock=!!(v->value&32)};
        for(unsigned h=0;h<2;h++)if(!a->p[h].z&&pets_pickup(c->pets,alive,h,&c->pet_followers[h],&pet,a->p[h].x,a->p[h].y)){
            /* The old pet replaces the consumed pickup atomically. Its brief
               ignore clock prevents the collector immediately swapping back. */
            *v=(CampaignDrop){pet.x,pet.y,pet.source_id?DROP_PET:0,pet.source_id|(pet.unlock?32:0)};
            c->drop_age[d]=pet.ignore_timer;break;
        }
    }
}
static void queue_drop(Campaign *c,const Fighter *enemy,unsigned kind,unsigned value){
    if(!kind||drop(c,enemy,kind,value))return;
    /* Chapter encounters share bounded overflow without replacing visible loot. */
    if(c->pending_count<CAMPAIGN_PENDING_DROPS)c->pending_drops[c->pending_count++]=(CampaignDrop){enemy->x,enemy->y,kind,value};
}
void campaign_queue_drop(Campaign *c,int x,int y,unsigned kind,unsigned value){Fighter p={.x=x,.y=y};queue_drop(c,&p,kind,value);}
static void drain_drops(Campaign *c){
    while(c->pending_count){
        CampaignDrop *d=&c->pending_drops[0];Fighter origin={.x=d->x,.y=d->y};
        if(!drop(c,&origin,d->kind,d->value))break;
        c->pending_count--;
        memmove(c->pending_drops,c->pending_drops+1,c->pending_count*sizeof(CampaignDrop));
    }
}
static unsigned random_below(Campaign *c,unsigned limit){
    unsigned x=c->rng?c->rng:1;x^=x<<13;x^=x>>17;x^=x<<5;c->rng=x;return x%limit;
}
unsigned campaign_roll_drop(Campaign *c,unsigned *value){
    *value=0;
    /* MAIN EnemyDie: eligible weapon1/120; else random item1/6; else coin9/10.
       Weapon19 is the barbarian weapon. */
    if(!random_below(c,120)){*value=WEAPON_BARBARIAN;return DROP_WEAPON;}
    if(!random_below(c,6)){
        unsigned item=2+random_below(c,8);
        if(item<=6){*value=10|((item-2)<<5);return DROP_FOOD;}
        if(item==7){*value=50;return DROP_CHICKEN;}
        if(item==8){*value=5;return DROP_BAG;}
        unsigned gem=1+random_below(c,12);*value=gem==12?1:5+gem;return gem==12?DROP_COIN:DROP_GEM;
    }
    if(random_below(c,10)){*value=1;return DROP_COIN;}
    return DROP_NONE;
}
static void age_drops(Campaign *c){
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind&&c->drops[d].kind!=DROP_WEAPON&&c->drops[d].kind!=DROP_CROC){
        if(c->drops[d].kind==DROP_PET){if(c->drop_age[d])c->drop_age[d]--;continue;}
        if(++c->drop_age[d]>=303){c->drops[d].kind=0;c->drop_age[d]=0;}
    }
}
static void collect(Campaign *c,Arena *a){
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind&&c->drops[d].kind!=DROP_WEAPON&&c->drops[d].kind!=DROP_PET&&c->drops[d].kind!=DROP_CROC&&c->drop_age[d]>=19)for(int i=0;i<2;i++){
        Fighter *p=&a->p[i];CampaignDrop *item=&c->drops[d];
        if(!p->ready||!p->hp||p->z>=KEEP_PICKUP_MAX_Z||abs(p->x-item->x)>KEEP_PICKUP_RANGE_X||abs(p->y-item->y)>KEEP_PICKUP_RANGE_Y)continue;
        if(item->kind==DROP_BOW){
            /* Source item 11: f_UnlockItem(bow) and equip it for the collecting hero. */
            if(c->tools[i]&2)continue;
            c->tools[i]|=2;campaign_select_item(c,i,3);p->equip_timer=30;p->bow_equipped=1;
            c->pickup_kind[i]=0;c->pickup_timer[i]=0;item->kind=0;break;
        }
        unsigned before=p->hp;
        if(item->kind<=DROP_BAG||item->kind==DROP_GEM){unsigned gold=c->profiles[i].gold+item->value;c->profiles[i].gold=gold>65535?65535:gold;}
        else {int maximum=campaign_health(c,i);if(p->hp>=maximum)continue;p->hp+=maximum*(item->kind==DROP_FOOD?(item->value&31):item->value)/100;if(p->hp>maximum)p->hp=maximum;}
        c->pickup_kind[i]=item->kind;c->pickup_value[i]=(item->kind<=DROP_BAG||item->kind==DROP_GEM)?item->value:p->hp-before;
        c->pickup_seq[i]=(c->pickup_seq[i]+1)&31;c->pickup_timer[i]=30;
        item->kind=0;break;
    }
    /* Hold shield beside a fallen partner for two seconds to revive. */
    for(int i=0;i<2;i++){
        Fighter *dead=&a->p[i],*helper=&a->p[1-i];
        if(dead->ready&&!dead->hp&&helper->ready&&helper->hp&&(helper->previous&BLOCK)&&abs(dead->x-helper->x)<26&&abs(dead->y-helper->y)<14&&!helper->hitstun){
            if(++c->revive[i]>=60){dead->hp=campaign_health(c,i)/4;dead->pose=POSE_IDLE;dead->invulnerable=30;dead->z=dead->vz=0;c->revive[i]=0;}
        }else c->revive[i]=0;
    }
}
int campaign_attack_contact(const Fighter *p){
    if(p->pose==POSE_DASHPUNCH||p->pose==POSE_DASHSLAM)
        return p->animation>=1&&p->animation<(p->pose==POSE_DASHPUNCH?6:11)&&p->cooldown&&!p->hitstun&&!p->freeze&&p->hp;
    int onset;
    switch(p->pose){
    case POSE_ATTACK:case POSE_UPPERCUT:case POSE_AIRATTACK:case POSE_AIRSPIN:onset=2;break;
    case POSE_LIGHT2:case POSE_AIRATTACK2:case POSE_AIRHEAVY:onset=4;break;
    case POSE_HEAVY3:case POSE_LIGHT3:onset=3;break;case POSE_FINISHER:onset=9;break;
    case POSE_STOMP:onset=8;break;
    case POSE_HEAVY:case POSE_SPLASH:case POSE_THROW:onset=6;break;case POSE_HEAVY2:onset=1;break;
    default:return 0;
    }
    return p->animation>=onset&&p->animation<onset+3&&p->cooldown&&!p->hitstun&&!p->freeze&&p->hp;
}
static void hit_object(Campaign *c,Arena *a,int id,const Fighter *object){
    if(!c->break_hp[id]||(c->broken&(1u<<id)))return;
    if(!--c->break_hp[id]){
        c->break_tick=(uint16_t)a->tick;
        c->broken=(c->broken&63u)|(1u<<id)|((unsigned)id<<8);
        /* MAIN f_HitBarrel random(5)+2 yields original10% healing foods. */
        if(id){Fighter spot=*object;place(&spot,spot.x,spot.y,a->map_id);queue_drop(c,&spot,DROP_FOOD,10|(random_below(c,5)<<5));}
    }
}
static void breakables(Campaign *c,Arena *a,const Projectile before[2]){
    if(a->map_id!=3&&a->map_id!=5)return;
    int first=a->map_id==5?1:0,last=a->map_id==5?5:0;
    for(int id=first;id<=last;id++){
        if(c->broken&(1u<<id))continue;
        Fighter object={.x=id?campaign_barrel_positions[id-1][0]:917,.y=id?campaign_barrel_positions[id-1][1]:333,.hp=1};
        for(int h=0;h<2;h++){
            Fighter *p=&a->p[h];
            if(p->ready&&p->attack_kind!=HIT_NONE&&campaign_attack_contact(p)&&c->break_hit_seq[id][h]!=(uint8_t)p->attack_seq&&arena_melee_overlap(p,&object)){
                c->break_hit_seq[id][h]=(uint8_t)p->attack_seq;hit_object(c,a,id,&object);
            }
        }
        for(int i=0;i<2;i++){
            Projectile *p=&a->shots[i];if(!p->life||p->owner>1||p->owner<0)continue;
            int from=before[i].life?before[i].x:a->p[p->owner].x;
            int lo=from<p->x?from:p->x,hi=from>p->x?from:p->x,radius=id?12:25;
            if(hi>=object.x-radius&&lo<=object.x+radius&&abs(p->y-object.y)<=8&&p->z<=46){hit_object(c,a,id,&object);p->life=0;}
        }
    }
}
int campaign_select_item(Campaign *c,unsigned h,unsigned item){
    if(h>=2||item>3||(item==1&&!c->potions[h])||(item==2&&!(c->tools[h]&1))||(item==3&&!(c->tools[h]&2)))return 0;
    int old=item==1?campaign_potion_equipped(c,h):item==2?c->shovel_equipped[h]:item==3?c->bow_on[h]:0;
    c->story_flags&=~(2u<<h);c->shovel_equipped[h]=0;c->bow_on[h]=0;
    if(!old&&item==1)c->story_flags|=2u<<h;
    if(!old&&item==2)c->shovel_equipped[h]=1;
    if(!old&&item==3)c->bow_on[h]=1;
    return 1;
}
static void campaign_items(Campaign *c,Arena *a,const uint16_t keys[2],uint16_t filtered[2]){
    for(int h=0;h<2;h++){
        Fighter *p=&a->p[h];unsigned held=c->item_held[h];int pressed=((keys[h]&BOW)&&!(held&1))||(keys[h]&BOW_EDGE);
        c->item_held[h]=(keys[h]&BOW?1:0)|(keys[h]&ATTACK?2:0)|(keys[h]&EQUIP?4:0);filtered[h]=keys[h]&~(EQUIP|EQUIP_EDGE);
        p->bow_equipped=(c->tools[h]&2)&&c->bow_on[h]; /* Only the picked-up bow is ever drawn. */
        if(!c->potions[h])c->story_flags&=~(2u<<h);
        if(!p->ready||!p->hp)continue;
        if(c->dig_timer[h]&&(p->hitstun||p->freeze||p->z))c->dig_timer[h]=0;
        if(c->shovel_equipped[h]){
            filtered[h]&=~(BOW|BOW_EDGE);
            if(pressed&&!c->dig_timer[h]&&!p->z&&!p->cooldown&&!p->hitstun&&!p->freeze)c->dig_timer[h]=1;
        }
        if(c->dig_timer[h]){filtered[h]=0;continue;}

        if(campaign_item_mask(c,h)&&(((keys[h]&EQUIP)&&!(held&4))||(keys[h]&EQUIP_EDGE))){
            /* EQUIP walks through the owned items in selector order, then back to empty hands. */
            unsigned mask=campaign_item_mask(c,h),current=campaign_equipped_item(c,h),next=0;
            for(unsigned i=current+1;i<ITEM_COUNT;i++)if(mask&(1u<<i)){next=i;break;}
            campaign_select_item(c,h,next?next:current);p->equip_timer=30;
            p->bow_equipped=(c->tools[h]&2)&&c->bow_on[h];
        }
        if(c->phase==KEEP_BARRACKS_TRAVEL&&abs(p->x-515)<=35&&abs(p->y-231)<=40&&!(c->story_flags&(8u<<h))){
            int claim=(keys[h]&ATTACK_EDGE)||((keys[h]&ATTACK)&&!(held&2));
            if(claim&&!c->potions[h]){c->potions[h]=1;c->story_flags|=8u<<h;}
            filtered[h]&=~(ATTACK|ATTACK_EDGE);
        }
        int action=(keys[h]&ATTACK_EDGE)||((keys[h]&ATTACK)&&!(held&2)),handled=0;
        for(int d=0;d<CAMPAIGN_DROPS;d++){
            CampaignDrop *item=&c->drops[d];
            if(item->kind!=DROP_WEAPON||abs(p->x-item->x)>28||abs(p->y-item->y)>22||p->z)continue;
            handled=1;filtered[h]&=~(ATTACK|ATTACK_EDGE);
            if(action){
                unsigned next=item->value;WeaponSet candidate=c->owned_weapons[h];campaign_weapon_unlock(&candidate,next);
                if(campaign_weapon_can_equip(&candidate,next,c->profiles[h].level)&&next!=c->weapons[h]){
                    item->value=c->weapons[h];c->owned_weapons[h]=candidate;campaign_equip(c,a,h,next);
                    int maximum=campaign_health(c,h);if(p->hp>maximum)p->hp=maximum;
                    c->pickup_kind[h]=DROP_WEAPON;c->pickup_value[h]=next;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;
                }
            }
            break;
        }
        /* LEVEL19 normal shop: potion12 gold, Skinny Sword32 gold. */
        if(!handled&&c->phase==KEEP_STORE&&p->y>=236&&p->y<=276&&p->x>=609&&p->x<=725){
            if(action&&p->x<669&&c->potions[h]<5&&c->profiles[h].gold>=12){c->profiles[h].gold-=12;c->potions[h]++;c->pickup_kind[h]=DROP_POTION;c->pickup_value[h]=c->potions[h];c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;}
            if(action&&p->x>=669&&c->weapons[h]!=WEAPON_SKINNY){
                if(c->profiles[h].gold>=32&&drop(c,p,DROP_WEAPON,c->weapons[h])){
                    c->profiles[h].gold-=32;
                    weapon_set_add(&c->owned_weapons[h],0);weapon_set_add(&c->owned_weapons[h],WEAPON_SKINNY);campaign_equip(c,a,h,WEAPON_SKINNY);
                    c->pickup_kind[h]=DROP_WEAPON;c->pickup_value[h]=WEAPON_SKINNY;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;
                }
            }
            if(keys[h]&ATTACK)p->previous|=ATTACK;else p->previous&=~ATTACK;
            filtered[h]&=~(ATTACK|ATTACK_EDGE);
        }
        if(campaign_potion_equipped(c,h))filtered[h]&=~(BOW|BOW_EDGE);
        if(pressed&&campaign_potion_equipped(c,h)&&p->hp<campaign_health(c,h)){
            unsigned old=p->hp;c->potions[h]--;if(!c->potions[h]){c->story_flags&=~(2u<<h);p->equip_timer=0;}p->hp=campaign_health(c,h);
            c->pickup_kind[h]=DROP_FOOD;c->pickup_value[h]=p->hp-old>255?255:p->hp-old;c->pickup_seq[h]=(c->pickup_seq[h]+1)&31;c->pickup_timer[h]=30;
            filtered[h]&=~(BOW|BOW_EDGE);
        }
    }
}
/* Boss reinforcements can carry authored food. The first six otherwise
   unused boss breakable bytes retain their spawn-time item until removal. */
/* Source f_EnemyDie: a thief that dies on screen drops the bow while a hero lacks it and none lies on the ground. */
static unsigned thief_bow(const Campaign *c,const Arena *a,unsigned slot){
    const Fighter *e=&c->enemies[slot];unsigned need=0;
    if(e->knight_id!=1)return 0;
    for(unsigned h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp&&!(c->tools[h]&2)&&abs(a->p[h].x-e->x)<260)need=1;
    if(!need)return 0;
    for(int d=0;d<CAMPAIGN_DROPS;d++)if(c->drops[d].kind==DROP_BOW)return 0;
    for(unsigned d=0;d<c->pending_count;d++)if(c->pending_drops[d].kind==DROP_BOW)return 0;
    return 1;
}
static unsigned campaign_enemy_drop(Campaign *c,const Arena *a,unsigned slot,unsigned *value){
    if(thief_bow(c,a,slot)){*value=0;return DROP_BOW;}
    if(a->map_id==BOSS_MAP&&slot<CAMPAIGN_ENEMIES){
        unsigned item=c->break_hp[slot];c->break_hp[slot]=0;
        if(item>=2&&item<=6){*value=10|((item-2)<<5);return DROP_FOOD;}
    }
    return campaign_roll_drop(c,value);
}
/* Shared by each campaign chapter: identical corpse/loot/XP/item behavior. */
void campaign_cinematic_tick(Campaign *c,Arena *a){
    for(int i=0;i<7;i++){
        Fighter *p=i<6?&c->enemies[i]:&c->ally;
        if(!p->ready||p->hp||p->z||p->pose!=POSE_DEAD)continue;
        if(c->combat.death_age[i]<44)c->combat.death_age[i]++;
        p->animation=c->combat.death_age[i];
        if(p->animation>=44){p->ready=0;if(i<6&&p->knight_id!=3){unsigned value=0,kind=campaign_enemy_drop(c,a,i,&value);queue_drop(c,p,kind,value);}}
    }
    collect(c,a);collect_pets(c,a);drain_drops(c);
}
void campaign_combat_tick(Campaign *c,Arena *a,const uint16_t keys[2]){
    pet_attack_step(c,a);
    int old_hp[CAMPAIGN_ENEMIES];uint8_t old_ready[CAMPAIGN_ENEMIES];
    for(int i=0;i<CAMPAIGN_ENEMIES;i++){old_hp[i]=c->enemies[i].hp;old_ready[i]=c->enemies[i].ready;}
    uint16_t filtered[2];campaign_items(c,a,keys,filtered);
    Projectile before[2]={a->shots[0],a->shots[1]};
    uint8_t levels[2]={c->profiles[0].level,c->profiles[1].level},hits[2]={0};
    ArenaCampaignAttributes attributes[2]={campaign_attributes(c,0),campaign_attributes(c,1)};
    campaign_combat_step_attributes(&c->combat,a,c->enemies,&c->ally,c->targets,filtered,levels,attributes,hits);
    for(unsigned h=0;h<2;h++)if(c->dig_timer[h]){
        Fighter *p=&a->p[h];
        if(!p->hp||p->hitstun||p->freeze){c->dig_timer[h]=0;continue;}
        unsigned tick=c->dig_timer[h]++;
        if(tick==(c->pets[h].equipped==PET_MR_BUDDY?34u:39u))campaign_dig_buddy(c,a,h,1);
        if(tick==(c->pets[h].equipped==PET_MR_BUDDY?34u:39u))campaign_forest_dig(c,a,h);
        p->pose=POSE_DIG;p->animation=tick-1;
        if(tick>=76)c->dig_timer[h]=0;
    }
    if(a->map_id<=CAMPAIGN_BARRACKS_MAP)breakables(c,a,before);
    for(int i=0;i<2;i++){give_xp(c,a,i,hits[i]);if(a->p[i].mana>campaign_mana(c,i))a->p[i].mana=campaign_mana(c,i);}
    for(int i=0;i<CAMPAIGN_ENEMIES;i++){
        if(c->enemies[i].knight_id==3)continue;
        if(old_hp[i]>0&&!c->enemies[i].hp){c->kills++;unsigned by=hits[1]>hits[0]?1u:hits[0]?0u:(a->p[1].ready&&abs(a->p[1].x-c->enemies[i].x)<abs(a->p[0].x-c->enemies[i].x)?1u:0u);if(c->level_kills[by]<65535)c->level_kills[by]++;}
        /* MAIN EnemyDie creates loot only after PLAYER death timeline ends.
           Process before the same slot can be reused by the next encounter. */
        if(old_ready[i]&&!c->enemies[i].ready&&!c->enemies[i].hp){
            unsigned value=0,kind=campaign_enemy_drop(c,a,i,&value);
            queue_drop(c,&c->enemies[i],kind,value);
        }
    }
    collect(c,a);collect_pets(c,a);drain_drops(c);
    owlet_step(c,a);troll_step(c,a);
    for(unsigned h=0;h<2;h++)if(c->pets[h].equipped&&!c->owlet_state[h])pets_follow_step(&c->pet_followers[h],a->p[h].x,a->p[h].y,a->p[h].facing);
}
static void war_physics(Campaign *c,Arena *a){
    WarState *w=&c->war;Fighter *m=&w->machine;
    if(c->phase==WAR_MACHINE_FIGHT&&m->ready&&m->hp>0){
        for(unsigned h=0;h<2;h++){
            Fighter *p=&a->p[h];ArenaCampaignAttributes stats=campaign_attributes(c,h);
            /* The machine occupies a wider body than humanoid targets. Test
               the near edge with the same authored melee rectangles. */
            Fighter edge=*m;edge.x=m->x+m->facing*7;edge.y=m->y+9;
            if(p->ready&&p->hp&&campaign_attack_contact(p)&&p->attack_kind!=HIT_NONE&&w->hit_seq[h]!=p->attack_seq&&arena_melee_overlap_body(p,&edge,130,95)){
                unsigned damage=(30+c->profiles[h].level+10*stats.strength)/10;
                w->hit_seq[h]=p->attack_seq;m->hp=m->hp>(int)damage?m->hp-damage:0;m->hit_seq++;give_xp(c,a,h,1);
            }
            if((w->events&WAR_EVENT_RAM)&&p->ready&&abs(p->x-m->x)<90&&abs(p->y-m->y)<22&&p->z<45){
                int damage=(2*(120-stats.defense)+50)/100;
                if(arena_campaign_strike(m,p,damage,6,m->facing,0,0))p->invulnerable=24;
            }
        }
        for(int i=0;i<2;i++){
            Projectile *p=&a->shots[i];if(!p->life||p->owner<0||p->owner>1)continue;
            if(abs(p->x-m->x)<=90+abs(p->vx)&&abs(p->y-m->y)<25&&p->z<100){
                ArenaCampaignAttributes stats=campaign_attributes(c,p->owner);
                unsigned damage=p->kind==SHOT_ARROW?2+stats.agility:(30+c->profiles[p->owner].level+4*stats.magic)/10;
                m->hp=m->hp>(int)damage?m->hp-damage:0;m->hit_seq++;give_xp(c,a,p->owner,1);p->life=0;
            }
        }
        if(w->events&WAR_EVENT_CANNON)for(int i=0;i<2;i++)if(!a->shots[i].life){
            unsigned target=w->target<2?w->target:0,barrel=w->barrel<3?w->barrel:0;int x=m->x+m->facing*(70+15*(int)barrel);
            int tx=a->p[target].x,ty=a->p[target].y,vx=(tx-x)/39;if(vx>25)vx=25;if(vx<-25)vx=-25;if(!vx)vx=tx>=x?1:-1;
            /* Lobbed at the spot where the target stands now; the red mark on the ground gives time to leave it. */
            a->shots[i]=(Projectile){.x=x,.y=ty,.z=ARENA_CANNON_Z0,.vx=vx,.life=48,.owner=8,.kind=SHOT_ARROW,.vz=tx};break;

        }
    }
    if(a->map_id==WAR_MAP&&w->gate_hp&&c->group>=WAR_WAVE_COUNT){
        for(unsigned h=0;h<2;h++){
            Fighter *p=&a->p[h];Fighter gate={.x=WAR_GATE_X,.y=p->y};
            if(p->ready&&p->hp&&campaign_attack_contact(p)&&p->attack_kind!=HIT_NONE&&p->attack_seq!=w->gate_hit_seq[h]&&abs(p->y-WAR_GATE_Y)<100&&arena_melee_overlap(p,&gate)){
                unsigned damage=7; /* Source wood door has fixed damage per swing. */
                w->gate_hit_seq[h]=p->attack_seq;w->gate_hp=w->gate_hp>damage?w->gate_hp-damage:0;
            }
        }
        for(unsigned i=0;i<2;i++){
            Projectile *p=&a->shots[i];
            if(p->life&&p->owner<2&&p->kind==SHOT_ARROW&&abs(p->x-WAR_GATE_X)<25+abs(p->vx)&&abs(p->y-WAR_GATE_Y)<100){if(w->gate_hp>1)w->gate_hp--;p->life=0;}
        }
    }
}
void campaign_tally_values(const Campaign *c,unsigned hero,unsigned out[3]){
    if(hero>=2){out[0]=out[1]=out[2]=0;return;}
    int gold=(int)c->profiles[hero].gold-(int)c->tally_gold0[hero],xp=(int)(c->profiles[hero].xp&131071u)-(int)(c->tally_xp0[hero]&131071u);
    out[0]=c->level_kills[hero];out[1]=gold>0?(unsigned)gold:0;out[2]=xp>0?(unsigned)xp:0;
}
int campaign_tally_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    a->tick++;if(c->tally_ticks<65535)c->tally_ticks++;
    return c->tally_ticks>=CAMPAIGN_TALLY_TICKS||(c->tally_ticks>=150&&((keys[0]|keys[1])&ATTACK_EDGE));
}
static void tally_begin(Campaign *c){for(int h=0;h<2;h++){c->tally_xp0[h]=c->profiles[h].xp;c->tally_gold0[h]=c->profiles[h].gold;c->level_kills[h]=0;}c->tally_ticks=0;c->tally_valid=1;}
void campaign_world_map(Campaign *c,Arena *a){
    c->tally_ticks=0;a->winner=0;a->countdown=0;c->paused=0;c->phase=CAMPAIGN_WORLD_MAP;c->group=a->map_id==CAMP_MAP?9:a->map_id==FLOOD_MAP?8:a->map_id==DEER_MAP?7:a->map_id==THIEVES_MAP?6:a->map_id==FOREST_MAP?5:a->map_id==BOSS_MAP?4:a->map_id>=HUB_BLACKSMITH?0:a->map_id==4?2:(a->map_id==6||a->map_id==7)?3:1;c->spawned=c->group;c->timer=0;
}
/* The departure node and elapsed walk fit existing phase-specific wire fields.
   Gameplay actors stay on their valid scene coordinates throughout map travel. */
int campaign_world_choose(Campaign *c,unsigned node){
    if(c->phase!=CAMPAIGN_WORLD_MAP||node>=CAMPAIGN_WORLD_NODES||campaign_world_travelling(c)||node==c->group)return 0;
    c->spawned=c->group;c->group=node;c->timer=0;return 1;
}
void campaign_world_tick(Campaign *c,Arena *a){
    if(c->phase!=CAMPAIGN_WORLD_MAP||c->paused)return;
    a->tick++;
    if(campaign_world_travelling(c)&&++c->timer>=world_walk_duration(c->spawned,c->group)){c->spawned=c->group;c->timer=0;}
}
int campaign_world_enter(Campaign *c,Arena *a,unsigned node){
    if(campaign_world_travelling(c))return 0;
    if(!campaign_world_unlocked(c,node))return 0;
    for(unsigned h=0;h<2;h++)if(a->p[h].ready){a->p[h].hp=campaign_health(c,h);a->p[h].mana=campaign_mana(c,h);}
    tally_begin(c);
    if(node==0)campaign_hubs_start(c,a,HUB_BLACKSMITH,0);else if(node<=2){campaign_start(c,a,a->p[1].ready);if(node==2)enter_store(c,a);}else if(node==9)campaign_camp_start(c,a);else if(node==8)campaign_flood_start(c,a);else if(node==7)campaign_deer_start(c,a);else if(node==6)campaign_thieves_start(c,a);else if(node==5)campaign_forest_start(c,a);else if(node==4)campaign_boss_start(c,a);else campaign_war_start(c,a,WAR_MAP);
    return 1;
}
void campaign_step(Campaign *c,Arena *a,const uint16_t keys[2]){
    if(!a->running||a->winner||c->paused||c->phase==CAMPAIGN_WORLD_MAP)return;
    age_drops(c);drain_drops(c);
    for(int i=0;i<2;i++)if(c->pickup_timer[i])c->pickup_timer[i]--;
    if(c->phase!=BOSS_DUEL&&!living_heroes(a)){a->winner=2;return;}
    if(a->map_id==BOSS_MAP){campaign_boss_step(c,a,keys);return;}
    if(a->map_id==CAMP_MAP){campaign_camp_step(c,a,keys);if(c->phase==WAR_COMPLETE&&campaign_tally_step(c,a,keys))campaign_world_map(c,a);return;}
    if(a->map_id==FLOOD_MAP){campaign_flood_step(c,a,keys);if(c->phase==WAR_COMPLETE&&campaign_tally_step(c,a,keys))campaign_world_map(c,a);return;}
    if(a->map_id==DEER_MAP){campaign_deer_step(c,a,keys);if(c->phase==WAR_COMPLETE&&campaign_tally_step(c,a,keys))campaign_world_map(c,a);return;}
    if(a->map_id==THIEVES_MAP){campaign_thieves_step(c,a,keys);if(c->phase==WAR_COMPLETE&&campaign_tally_step(c,a,keys))campaign_world_map(c,a);return;}
    if(a->map_id==FOREST_MAP){campaign_forest_step(c,a,keys);if(c->phase==WAR_COMPLETE&&campaign_tally_step(c,a,keys))campaign_world_map(c,a);return;}
    if(a->map_id>=HUB_BLACKSMITH){if(campaign_hubs_step(c,a,keys)==HUB_WORLD_MAP)campaign_world_map(c,a);return;}
    if(a->map_id==WAR_MAP||a->map_id==BRIDGE_MAP){campaign_war_step(c,a,keys);war_physics(c,a);if(c->phase==WAR_COMPLETE)campaign_boss_start(c,a);return;}
    if(c->phase==KEEP_BARRACKS_INTRO){
        c->timer++;a->tick++;
        if(c->timer>=CAMPAIGN_BARRACKS_INTRO_TICKS||(c->timer>30&&((keys[0]|keys[1])&ATTACK_EDGE))){if(c->timer>=CAMPAIGN_BARRACKS_INTRO_TICKS)c->story_flags|=64;c->phase=KEEP_BARRACKS_TRAVEL;c->timer=0;}
        return;
    }
    if(c->phase==KEEP_DOOR){if(++c->timer>=18){c->completed|=1;campaign_war_start(c,a,WAR_MAP);}a->tick++;return;}
    if(c->phase==KEEP_PAN_IN||c->phase==KEEP_PAN_OUT){
        c->timer++;a->tick++;
        if(c->timer>=18){
            if(c->phase==KEEP_PAN_OUT&&c->group==2){
                /* LEVEL13 WP7 creates these actors hidden before the movie;
                   reveal from their original anchors when the camera returns,
                   while the party is still at WP7 rather than ON the WP9 spawn. */
                begin_group(c,a);
            }else{c->phase=c->phase==KEEP_PAN_IN?KEEP_ABDUCTION:KEEP_TRAVEL;c->timer=0;}
        }
        return;
    }
    if(c->phase==KEEP_INTRO||c->phase==KEEP_ABDUCTION||c->phase==KEEP_ENDING){
        c->timer++;a->tick++;
        int length=c->phase==KEEP_INTRO?(int)cinematic_duration(CINEMATIC_CRYSTAL):c->phase==KEEP_ABDUCTION?(int)cinematic_duration(CINEMATIC_PRINCESSES):90;
        if(c->timer>=length||(c->timer>30&&((keys[0]|keys[1])&ATTACK_EDGE))){
            if(c->phase==KEEP_ENDING){c->completed|=1;campaign_war_start(c,a,WAR_MAP);}
            else if(c->phase==KEEP_ABDUCTION){c->phase=KEEP_PAN_OUT;c->timer=0;}
            else {c->phase=KEEP_TRAVEL;c->timer=0;}
        }
        return;
    }
    if(a->map_id==3&&(c->npc_ticks||a->p[0].x>=376||(a->p[1].ready&&a->p[1].x>=376))&&c->npc_ticks<255)c->npc_ticks++;
    if(a->map_id==3&&!(c->story_flags&32)&&(a->p[0].x>=1250||(a->p[1].ready&&a->p[1].x>=1250))){
        Fighter fork={0};place(&fork,1378,335,3);if(drop(c,&fork,DROP_WEAPON,WEAPON_CARROT))c->story_flags|=32;
    }
    campaign_combat_tick(c,a,keys);
    if(c->phase==KEEP_STORE_FIGHT&&!living_enemies(c)){c->phase=KEEP_STORE;c->timer=0;}
    if(c->phase==KEEP_FIGHT){
        c->timer++;
        int initial=c->group==3?2:3;
        while(c->spawned<initial&&spawn_enemy(c,a,c->spawned,0)){}
        if(c->group==3){
            if(c->spawned==2&&c->timer>=28)spawn_enemy(c,a,2,1);
            if(c->spawned==3&&c->timer>=41)spawn_enemy(c,a,3,1);
        }
        if(!living_enemies(c)&&c->spawned==keep_encounter_enemies[c->group]){
            c->group++;c->phase=KEEP_TRAVEL;c->timer=0;

        }
    }
    /* Keep partners within one scrolling view without repositioning either at
       section boundaries. The trailing hero remains free to catch up. */
    if(a->p[1].ready&&a->p[0].hp&&a->p[1].hp){
        int lead=a->p[0].x>a->p[1].x?0:1,trail=1-lead;
        if(a->p[lead].x-a->p[trail].x>300){
            int x=a->p[lead].x,y=a->p[lead].y;
            a->p[lead].x=a->p[trail].x+300;stage_move(&a->p[lead],x,y);
        }
    }
    if(c->phase==KEEP_BARRACKS_TRAVEL){
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp&&a->p[h].x>1173&&a->p[h].y>=131&&a->p[h].y<=174)a->p[h].x=1173;
        int enter=1,alive=0;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp){alive++;if(a->p[h].x<1136||a->p[h].x>1173||a->p[h].y<131||a->p[h].y>174)enter=0;}
        if(enter&&alive)enter_keep(c,a);
    }
    if(c->phase==KEEP_TRAVEL){
        int progress=a->p[0].hp?a->p[0].x:a->p[1].x;
        if(a->p[1].ready&&a->p[1].hp&&a->p[1].x<progress)progress=a->p[1].x;
        if(c->group==2&&!(c->story_flags&1)&&progress>=1421){
            c->story_flags|=1;c->phase=KEEP_PAN_IN;c->timer=0;
        }else if(c->group<4&&progress>=encounter_x[c->group])begin_group(c,a);else if(c->group==4&&progress>=2380){c->completed|=8;enter_store(c,a);}
    }
    if(c->phase==KEEP_STORE){
        int progress=a->p[0].hp?a->p[0].x:a->p[1].x;
        if(a->p[1].ready&&a->p[1].hp&&a->p[1].x<progress)progress=a->p[1].x;
        if(progress>=965){c->phase=KEEP_DOOR;c->timer=0;}
    }
}
const char *campaign_objective(const Campaign *c){
    if(c->phase==KEEP_BARRACKS_INTRO)return "A warning interrupts the music";
    if(c->phase==KEEP_BARRACKS_TRAVEL)return "Free potion / exit upstairs";
    if(c->phase==KEEP_INTRO)return "The crystal has been stolen!";
    if(c->phase==KEEP_ABDUCTION)return "They have the princesses!";
    if(c->phase==KEEP_STORE_FIGHT)return "Help the Gray Knight";
    if(c->phase==KEEP_STORE)return "A: potion12 / sword32 gold";
    if(c->phase==KEEP_DOOR)return "Leaving the castle";
    if(c->phase==KEEP_FIGHT)return "Defeat the barbarians";
    if(c->phase==KEEP_ENDING||c->phase==KEEP_COMPLETE)return "Castle Keep complete";
    return "Move right - stay together";
}
static const unsigned char wire_pose_count[]={
13,16,13,12,3,6,13,14,18,16,15,15,12,9,6,6,2,1,17,8,13,5,12,20,
12,14,13,12,24,76,13,14,
};
_Static_assert(sizeof(wire_pose_count)==POSE_COUNT,"Every pose needs a snapshot frame count");
static void actor_pack(CampaignActor *v,const Fighter *p){
    unsigned pose=p->pose>=0&&p->pose<POSE_COUNT?p->pose:0,count=wire_pose_count[pose],animation=p->animation>0?(unsigned)p->animation:0;
    unsigned frame=(pose==POSE_IDLE||pose==POSE_WALK||pose==POSE_CLASH)?animation%count:pose==POSE_DEAD?(animation<44?animation:44):(animation<count?animation:count-1); /* Death clips of the species enemies run all 45 ticks. */
    *v=(CampaignActor){.x=p->x,.y=p->y,.hp=p->hp,.mana=p->mana,.animation=frame,.z=p->z,.pose=pose,
        .flags=p->ready|(p->facing<0?2:0)|(p->bow_equipped?4:0)|((p->previous&MAGIC)?8:0)|(p->knight_id<<4),
        .freeze=p->freeze,.burn=p->burn,.cooldown=p->cooldown,.attack_kind=p->attack_kind,
        .event_attack_hit=(p->attack_seq&15)|((p->hit_seq&15)<<4),.event_block_magic=(p->block_seq&15)|((p->magic_seq&15)<<4),
        .event_arrow_equip=(p->arrow_seq&15)|((p->equip_timer/2)<<4)};
}
static void actor_unpack(Fighter *p,const CampaignActor *v){
    *p=(Fighter){.x=v->x,.y=v->y,.hp=v->hp,.mana=v->mana,.animation=v->animation,.z=v->z,.pose=v->pose,
        .ready=v->flags&1,.facing=v->flags&2?-1:1,.bow_equipped=!!(v->flags&4),.previous=v->flags&8?MAGIC:0,.knight_id=(v->flags>>4)&3,
        .freeze=v->freeze,.burn=v->burn,.equip_timer=(v->event_arrow_equip>>4)*2,.cooldown=v->cooldown&31,.attack_kind=v->attack_kind&15,
        .attack_seq=v->event_attack_hit&15,.hit_seq=v->event_attack_hit>>4,.block_seq=v->event_block_magic&15,.magic_seq=v->event_block_magic>>4,.arrow_seq=v->event_arrow_equip&15};
    if(p->pose==POSE_WALK&&(v->animation&32)){
        p->animation=32+(v->animation&15);
        if(v->animation&16)p->previous|=p->facing<0?LEFT:RIGHT;
        if(v->animation&64)p->previous|=UP;
        if(v->animation&128)p->previous|=DOWN;
    }
}
/* Rotating extended state: every snapshot carries one byte in the spare high bits of the two heroes' hp.
 * Even sequences alternate the heroes' weapon ids (so an equip shows within four snapshots); odd ones cycle the eight
 * level-clear bytes and the two equipped pet ids. Guests keep the last value of each field between snapshots. */
static unsigned ext_byte(const Campaign *c,uint32_t seq){
    unsigned sel=seq&31,idx=sel>>1;
    if(!(sel&1))return c->weapons[idx&1];
    if(idx<8)return (unsigned)(c->completed>>(8*idx))&255;
    if(idx<10)return c->pets[idx-8].equipped;
    return 0;
}
static int ext_valid(uint32_t seq,unsigned e){
    unsigned sel=seq&31,idx=sel>>1;
    if(!(sel&1))return e<WEAPON_COUNT;
    if(idx>=8&&idx<10)return !e||pets_info(e);
    return 1;
}
static void ext_apply(Campaign *c,uint32_t seq,unsigned e){
    unsigned sel=seq&31,idx=sel>>1;
    if(!ext_valid(seq,e))return;
    if(!(sel&1))c->weapons[idx&1]=(uint8_t)e;
    else if(idx<8)c->completed=(c->completed&~((uint64_t)255<<(8*idx)))|((uint64_t)e<<(8*idx));
    else if(idx<10)c->pets[idx-8].equipped=(uint8_t)e;
}
void campaign_pack(CampaignPacket *p,const Campaign *c,const Arena *a,uint32_t seq){
    memset(p,0,sizeof(*p));p->magic=CAMPAIGN_MAGIC;p->version=CAMPAIGN_VERSION;p->sequence=seq;
    for(int i=0;i<2;i++){
        actor_pack(&p->actors[i],&a->p[i]);p->actors[i].cooldown|=c->shovel_equipped[i]?32:0;if((c->mount_mask>>(6+i))&1)p->actors[i].flags|=64;
        if(arena_fighter_running(&a->p[i]))p->actors[i].animation|=32|((a->p[i].previous&(LEFT|RIGHT))?16:0)|((a->p[i].previous&UP)?64:0)|((a->p[i].previous&DOWN)?128:0);
        if(a->map_id==BOSS_MAP&&campaign_boss_buried_frame(c,i)){p->actors[i].cooldown|=64;p->actors[i].animation=campaign_boss_buried_frame(c,i);}
        if(c->phase!=BOSS_DUEL&&a->clash_timer&&a->p[i].pose==POSE_CLASH)p->actors[i].animation=a->clash_score[0];
    }
    {unsigned e=ext_byte(c,seq);p->actors[0].hp|=(e&15)<<11;p->actors[1].hp|=(e>>4)<<11;}
    for(int i=0;i<CAMPAIGN_ENEMIES;i++){
        actor_pack(&p->actors[i+2],&c->enemies[i]);p->actors[i+2].attack_kind|=(uint8_t)(c->species[i]<<4);if(c->mount_mask&(1u<<i)&&c->enemies[i].hp>0)p->actors[i+2].flags|=64;
        if(c->enemies[i].knight_id==2)p->actors[i+2].animation=c->enemies[i].animation%60;
        if(!c->enemies[i].hp&&c->enemies[i].pose==POSE_DEAD)p->actors[i+2].animation=c->enemies[i].animation;
        if(c->phase!=BOSS_DUEL&&a->clash_timer&&c->enemies[i].pose==POSE_CLASH)p->actors[i+2].animation=a->clash_score[1];
    }
    actor_pack(&p->actors[8],&c->ally);
    if(c->war.machine.ready||a->map_id==BOSS_MAP){actor_pack(&p->actors[8],&c->war.machine);p->actors[8].flags|=128;p->actors[8].animation=c->war.machine.animation;}
    if(!c->ally.hp&&c->ally.pose==POSE_DEAD)p->actors[8].animation=c->ally.animation;
    if(a->map_id==BOSS_MAP&&c->phase>=BOSS_TREASURE){
        uint32_t mask=c->war.hit_seq[0]|((uint32_t)c->war.hit_seq[1]<<16);
        p->actors[8].event_attack_hit=mask;p->actors[8].event_block_magic=mask>>8;
        p->actors[8].event_arrow_equip=mask>>16;p->actors[8].animation=mask>>24;
    }
    memcpy(p->shots,a->shots,sizeof(p->shots));memcpy(p->profiles,c->profiles,sizeof(p->profiles));memcpy(p->drops,c->drops,sizeof(p->drops));
    /* Spare coordinate bits carry each item's independent nine-bit age. */
    for(int d=0;d<CAMPAIGN_DROPS;d++){
        unsigned age=c->drops[d].kind&&c->drops[d].kind!=DROP_WEAPON?c->drop_age[d]:0;
        p->drops[d].x=(c->drops[d].x&16383)|((age>>7)<<14);
        p->drops[d].y=(c->drops[d].y&511)|((age&127)<<9);
    }
    for(int i=0;i<2;i++){
        p->profiles[i].knight=(c->profiles[i].knight&3)|((((c->owned_weapons[i].w[0])|1)&15)<<4);
        p->profiles[i].xp=(c->profiles[i].xp&131071u)|((uint32_t)c->stats[i][0]<<17)|((uint32_t)c->stats[i][1]<<22)|((uint32_t)c->stats[i][2]<<27);
        p->actors[i].freeze|=(c->stats[i][3]&7)<<5;p->actors[i].burn|=(c->stats[i][3]>>3)<<6;
        p->actors[i].event_arrow_equip=(p->actors[i].event_arrow_equip&240)|(c->menu_ack[i]&15);
        p->actors[i].attack_kind=(p->actors[i].attack_kind&15)|((c->tools[i]&3)<<6);
    }
    p->tick=a->tick;p->countdown=c->broken;p->round=c->break_tick;p->timer=c->timer;p->running=a->running|(c->paused?2:0)|(c->completed&28);p->winner=a->winner;p->map_id=a->map_id;
    p->phase=c->phase;p->group=c->group;p->spawned=c->spawned;p->kills=c->kills;p->completed=(c->completed&1)|((c->completed&2)<<6)|(c->potions[0]<<1)|(c->potions[1]<<4);memcpy(p->revive,c->revive,2);p->reserved[0]=c->npc_ticks;p->reserved[7]=c->story_flags;
    if(a->map_id==WAR_MAP||a->map_id==BRIDGE_MAP||a->map_id==BOSS_MAP||a->map_id>=FOREST_MAP){p->countdown=c->war.gate_hp;p->round=(c->war.flags&31)|(c->broken<<5);}
    if(a->map_id>=FOREST_MAP&&c->phase==WAR_COMPLETE){p->kills=(uint8_t)(c->level_kills[0]>255?255:c->level_kills[0]);p->countdown=c->level_kills[1]>301?301:c->level_kills[1];} /* The score sign needs each hero's kills; the gate and goal counters are idle once a level is complete. */
    if(a->map_id==BOSS_MAP&&c->phase==BOSS_DUEL){p->countdown=(a->countdown&127)|((a->clash_score[0]&15)<<7)|((a->clash_score[1]&15)<<11);p->round=(c->war.flags&31)|((a->clash_timer&127)<<5);}
    for(int i=0;i<2;i++){p->reserved[1+i*3]=(c->pickup_seq[i]<<3)|c->pickup_kind[i];p->reserved[2+i*3]=c->pickup_value[i];p->reserved[3+i*3]=c->pickup_timer[i];}
}
int campaign_packet_valid(const CampaignPacket *p){
    if(!p||p->magic!=CAMPAIGN_MAGIC||p->version!=CAMPAIGN_VERSION||p->map_id<3||p->map_id>CAMP_MAP||p->running>31||p->winner>2||((p->completed>>1)&7)>5||((p->completed>>4)&7)>5||p->revive[0]>60||p->revive[1]>60)return 0;
    int war=p->map_id==WAR_MAP||p->map_id==BRIDGE_MAP||p->map_id==FOREST_MAP||p->map_id==THIEVES_MAP||p->map_id==DEER_MAP||p->map_id==FLOOD_MAP||p->map_id==CAMP_MAP,boss=p->map_id==BOSS_MAP;
    if(p->phase==CAMPAIGN_WORLD_MAP){if(p->group>=CAMPAIGN_WORLD_NODES||p->spawned>=CAMPAIGN_WORLD_NODES||p->timer>world_walk_duration(p->spawned,p->group)||(p->spawned==p->group&&p->timer))return 0;}
    else if(boss){if(p->phase<BOSS_INTRO||p->phase>BOSS_COMPLETE||p->group>4||p->spawned>6||(p->phase==BOSS_DUEL?((p->countdown&127)>90||((p->countdown>>7)&15)>10||((p->countdown>>11)&15)>10||p->countdown>32767||p->round>4095||(p->round>>5)>90):p->round>31))return 0;}
    else if(p->map_id>=HUB_BLACKSMITH&&p->map_id<FOREST_MAP){if(p->phase!=HUB_EXPLORE)return 0;}
    else if(war){if(p->phase<WAR_TRAVEL||p->phase>WAR_FOREST_CINEMA||p->group>(p->map_id==THIEVES_MAP?THIEVES_EVENT_COUNT:WAR_WAVE_COUNT)||p->spawned>(p->map_id>=FOREST_MAP?15:7)||p->countdown>301||(p->map_id<FOREST_MAP&&(p->round&~16383u)))return 0;} /* Forest levels publish up to eleven scenery bits above the five war flags. */
    else if((p->countdown&~0x73fu)||((p->countdown>>8)&7)>5||p->phase>KEEP_BARRACKS_TRAVEL||p->group>4||p->spawned>6||p->kills>19)return 0;
    if(boss&&p->phase>=BOSS_TREASURE&&(p->actors[8].animation>31||(p->actors[8].flags&1)||p->actors[8].hp))return 0;
    for(int i=0;i<9;i++){
        const CampaignActor *f=&p->actors[i];
        int machine=(war||boss)&&i==8&&(f->flags&128);
        if((i>=2&&(f->flags&~(machine?191:p->map_id>=FOREST_MAP?127:63)))||f->hp<0||(i<2?f->hp&2047:f->hp)>(i<2?1200:machine?(boss?BOSS_MAX_HEALTH:450):p->map_id==FLOOD_MAP?1000:ARENA_HEALTH)||f->mana>(i<2?170:ARENA_MANA)||f->z>80||f->pose>=POSE_COUNT||(f->freeze&(i<2?31:255))>ARENA_FREEZE_TICKS||(f->burn&(i<2?63:255))>ARENA_BURN_TICKS||(i<2?(f->cooldown&31):f->cooldown)>24||(i<2&&(f->cooldown&128))||(f->attack_kind&15)>HIT_STOMP||(i>=2&&i<8&&(f->attack_kind>>4)>=SPECIES_COUNT))return 0;
        if(i<2&&(f->cooldown&64)&&(!boss||p->phase!=BOSS_FIGHT||!f->animation||f->animation>41||f->pose!=POSE_IDLE||f->z||!f->hp||!(f->flags&1)))return 0;
        if(machine&&(f->x<0||f->x>=stage_world_width(p->map_id)||f->y<0||f->y>=stage_world_height(p->map_id)))return 0;
        if(!machine&&(f->flags&1)&&!stage_position_open_map(p->map_id,f->x,f->y))return 0;
    }
    for(int i=0;i<2;i++)if(((p->reserved[1+i*3]&7)==DROP_POTION&&(p->reserved[2+i*3]<1||p->reserved[2+i*3]>5))||p->reserved[3+i*3]>30)return 0;
    for(int i=0;i<2;i++)if(p->profiles[i].level<1||p->profiles[i].level>99||!(p->profiles[i].knight&16))return 0;
    if(!ext_valid(p->sequence,((p->actors[0].hp>>11)&15)|(((p->actors[1].hp>>11)&15)<<4)))return 0;
    for(int i=0;i<2;i++){
        unsigned st[4]={(p->profiles[i].xp>>17)&31,(p->profiles[i].xp>>22)&31,p->profiles[i].xp>>27,(p->actors[i].freeze>>5)|((p->actors[i].burn>>6)<<3)};
        unsigned used=0;for(int k=0;k<4;k++){if(st[k]>24)return 0;used+=st[k];}
        if(used>campaign_stat_budget(p->profiles[i].level))return 0;
    }
    for(int i=0;i<CAMPAIGN_DROPS;i++){
        const CampaignDrop *d=&p->drops[i];unsigned x=(uint16_t)d->x,y=(uint16_t)d->y;
        unsigned age=((x>>14)<<7)|(y>>9);
        if(age>=303||d->kind>DROP_CROC||d->kind==DROP_POTION||(d->kind==DROP_WEAPON&&(d->value>=WEAPON_COUNT||age))||
           (d->kind==DROP_PET&&(!pets_info(d->value&31)||d->value>63||age>PET_IGNORE_TICKS))||
           (d->kind&&(!stage_position_open_map(p->map_id,x&16383,y&511)||(d->kind==DROP_FOOD?((d->value&31)!=10||(d->value>>5)>4):d->kind!=DROP_PET&&d->value>50))))return 0;
    }
    StatePacket test={.magic=PROTOCOL_MAGIC,.version=PROTOCOL_VERSION};
    arena_init(&test.arena);test.arena.map_id=p->map_id;memcpy(test.arena.shots,p->shots,sizeof(p->shots));
    for(int i=0;i<2;i++){CampaignActor hero=p->actors[i];hero.hp&=2047;actor_unpack(&test.arena.p[i],&hero);test.arena.p[i].freeze&=31;test.arena.p[i].burn&=63;if(!test.arena.p[i].ready)place(&test.arena.p[i],110,239,p->map_id);}
    return protocol_state_valid(&test);
}
void campaign_unpack(const CampaignPacket *p,Campaign *c,Arena *a){
    struct {uint8_t weapons[2],pet[2];uint64_t completed;uint32_t xp0[2];uint16_t gold0[2],ticks;uint8_t valid,phase;} keep={{c->weapons[0],c->weapons[1]},{c->pets[0].equipped,c->pets[1].equipped},c->completed,{c->tally_xp0[0],c->tally_xp0[1]},{c->tally_gold0[0],c->tally_gold0[1]},c->tally_ticks,c->tally_valid,c->phase};
    memset(a,0,sizeof(*a));memset(c,0,sizeof(*c));for(int i=0;i<2;i++){CampaignActor hero=p->actors[i];hero.hp&=2047;actor_unpack(&a->p[i],&hero);if(p->map_id>=FOREST_MAP&&(p->actors[i].flags&64))c->mount_mask|=(uint8_t)(1u<<(6+i));}for(int i=0;i<CAMPAIGN_ENEMIES;i++){actor_unpack(&c->enemies[i],&p->actors[i+2]);c->species[i]=p->actors[i+2].attack_kind>>4;if(p->actors[i+2].flags&64)c->mount_mask|=1u<<i;}
    actor_unpack(&c->ally,&p->actors[8]);
    memcpy(a->shots,p->shots,sizeof(p->shots));memcpy(c->profiles,p->profiles,sizeof(c->profiles));memcpy(c->drops,p->drops,sizeof(c->drops));
    for(int d=0;d<CAMPAIGN_DROPS;d++){
        c->drop_age[d]=(((uint16_t)p->drops[d].x>>14)<<7)|((uint16_t)p->drops[d].y>>9);
        c->drops[d].x&=16383;c->drops[d].y&=511;
    }
    if(p->map_id==WAR_MAP||p->map_id==BRIDGE_MAP||p->map_id==BOSS_MAP||p->map_id>=FOREST_MAP){
        c->war.gate_hp=p->countdown;c->war.flags=p->round&31;c->broken=p->round>>5;
        if(p->actors[8].flags&128){actor_unpack(&c->war.machine,&p->actors[8]);c->war.machine.animation=p->actors[8].animation;c->war.mode=c->war.machine.attack_kind;c->ally=(Fighter){0};}
    }
    if(p->map_id==BOSS_MAP&&p->phase>=BOSS_TREASURE){
        uint32_t mask=p->actors[8].event_attack_hit|((uint32_t)p->actors[8].event_block_magic<<8)|((uint32_t)p->actors[8].event_arrow_equip<<16)|((uint32_t)p->actors[8].animation<<24);
        c->war.hit_seq[0]=mask;c->war.hit_seq[1]=mask>>16;
    }
    for(int i=0;i<2;i++){c->weapons[i]=keep.weapons[i];c->owned_weapons[i]=weapon_set_low(p->profiles[i].knight>>4);
        c->pets[i].equipped=keep.pet[i];c->tools[i]=p->actors[i].attack_kind>>6;c->shovel_equipped[i]=!!(p->actors[i].cooldown&32);
        if(p->map_id==BOSS_MAP&&p->phase==BOSS_FIGHT&&(p->actors[i].cooldown&64)){unsigned frame=p->actors[i].animation;c->break_hp[14+i]=frame<30?8:frame<38?38-frame:0;c->break_hit_seq[15][i]=frame<30?frame-1:frame<38?29:frame-8;a->p[i].animation=BOSS_BURY_WIGGLES-c->break_hp[14+i];}
        c->pet_followers[i]=(PetFollower){.x=a->p[i].x-a->p[i].facing*25,.y=a->p[i].y,.facing=a->p[i].facing};c->bow_on[i]=a->p[i].bow_equipped;
        c->profiles[i].knight&=3;
        c->stats[i][0]=(p->profiles[i].xp>>17)&31;c->stats[i][1]=(p->profiles[i].xp>>22)&31;c->stats[i][2]=p->profiles[i].xp>>27;c->stats[i][3]=(p->actors[i].freeze>>5)|((p->actors[i].burn>>6)<<3);c->profiles[i].xp&=131071u;
        a->p[i].freeze&=31;a->p[i].burn&=63;c->menu_ack[i]=p->actors[i].event_arrow_equip&15;a->p[i].arrow_seq=0;
    }
    a->tick=p->tick;a->countdown=0;a->round=p->map_id;c->broken=(p->map_id==WAR_MAP||p->map_id==BRIDGE_MAP||p->map_id==BOSS_MAP||p->map_id>=FOREST_MAP)?p->round>>5:p->countdown;c->break_tick=p->round;a->running=p->running&1;c->paused=(p->running>>1)&1;a->winner=p->winner;a->map_id=p->map_id;
    if(p->map_id==BOSS_MAP&&p->phase==BOSS_DUEL){a->countdown=p->countdown&127;a->clash_score[0]=(p->countdown>>7)&15;a->clash_score[1]=(p->countdown>>11)&15;a->clash_timer=p->round>>5;}
    else for(int h=0;h<2;h++)if(a->p[h].pose==POSE_CLASH)for(int e=0;e<6;e++)if(c->enemies[e].pose==POSE_CLASH){a->clash_timer=90;a->clash_score[0]=p->actors[h].animation;a->clash_score[1]=p->actors[e+2].animation;}
    c->timer=p->timer;c->phase=p->phase;c->group=p->group;c->spawned=p->spawned;c->kills=p->kills;c->completed=keep.completed|(p->completed&1)|((p->completed>>6)&2)|(p->running&28);c->potions[0]=(p->completed>>1)&7;c->potions[1]=(p->completed>>4)&7;memcpy(c->revive,p->revive,2);c->npc_ticks=p->reserved[0];c->story_flags=p->reserved[7];
    for(int i=0;i<2;i++){c->pickup_seq[i]=p->reserved[1+i*3]>>3;c->pickup_kind[i]=p->reserved[1+i*3]&7;c->pickup_value[i]=p->reserved[2+i*3];c->pickup_timer[i]=p->reserved[3+i*3];}
    /* The score sign compares against what the party held when the level began: the first packet of a level sets that baseline. */
    c->tally_ticks=keep.ticks;
    if(!keep.valid||(keep.phase==CAMPAIGN_WORLD_MAP&&p->phase!=CAMPAIGN_WORLD_MAP)){for(int i=0;i<2;i++){c->tally_xp0[i]=c->profiles[i].xp;c->tally_gold0[i]=c->profiles[i].gold;}}
    else for(int i=0;i<2;i++){c->tally_xp0[i]=keep.xp0[i];c->tally_gold0[i]=keep.gold0[i];}
    c->tally_valid=1;
    if(p->map_id>=FOREST_MAP&&p->phase==WAR_COMPLETE){c->level_kills[0]=p->kills;c->level_kills[1]=p->countdown;}
    ext_apply(c,p->sequence,((p->actors[0].hp>>11)&15)|(((p->actors[1].hp>>11)&15)<<4));
}
void campaign_sync_cycle(const Campaign *c,const Arena *a,Campaign *guest,Arena *remote,uint32_t first){
    CampaignPacket p;
    for(uint32_t k=0;k<32;k++){campaign_pack(&p,c,a,first+k);campaign_unpack(&p,guest,remote);}
}

/* LEVEL18 mount2 (f_HorseWaitCheck): a hero who jumps within 50 units across and 20 deep of a waiting crocodile lands on it; the
   seat is 44 source pixels up. Jumping again drops the hero back to the road and leaves the crocodile standing. */
#define MOUNT_ARC_TICKS 12
void campaign_mount_step(Campaign *c,Arena *a,uint16_t keys[2]){
    for(unsigned h=0;h<2;h++){
        Fighter *p=&a->p[h];uint8_t bit=(uint8_t)(1u<<(6+h));
        if(!p->ready)continue;
        int jump=(keys[h]&JUMP_EDGE)!=0;
        if(c->mount_cool[h])c->mount_cool[h]--;
        if(c->mount_arc[h]){keys[h]=0;continue;} /* leaping onto the crocodile */
        if(c->mount_mask&bit){
            if(p->hp<=0||jump){
                campaign_croc_place(c,p->x,p->y,p->facing);c->mount_mask&=(uint8_t)~bit;c->mount_cool[h]=60;
            }
            continue;
        }
        if(p->hp<=0||p->z||p->hitstun||c->mount_cool[h])continue;
        for(int d=0;d<CAMPAIGN_DROPS;d++){
            CampaignDrop *item=&c->drops[d];
            if(item->kind!=DROP_CROC)continue;
            /* Walking into a waiting crocodile climbs on; so does a jump from a little farther away (f_HorseWaitCheck: 50 across, 20 deep). */
            int near=a->map_id!=FLOOD_MAP&&abs(p->x-item->x)<26&&abs(p->y-item->y)<12,jumped=jump&&abs(p->x-item->x)<50&&abs(p->y-item->y)<20;
            if(!near&&!jumped)continue;
            item->kind=0;c->mount_arc[h]=MOUNT_ARC_TICKS;c->mount_from[h][0]=p->x;c->mount_from[h][1]=p->y;c->mount_to[h][0]=item->x;c->mount_to[h][1]=item->y;
            p->facing=item->value?-1:1;keys[h]=0;
            break;
        }
    }
}
void campaign_mount_arc(Campaign *c,Arena *a){
    for(unsigned h=0;h<2;h++){
        Fighter *p=&a->p[h];if(!c->mount_arc[h]||!p->ready)continue;
        unsigned left=--c->mount_arc[h],done=MOUNT_ARC_TICKS-left;
        p->x=(int16_t)(c->mount_from[h][0]+(c->mount_to[h][0]-c->mount_from[h][0])*(int)done/MOUNT_ARC_TICKS);
        p->y=(int16_t)(c->mount_from[h][1]+(c->mount_to[h][1]-c->mount_from[h][1])*(int)done/MOUNT_ARC_TICKS);
        if(left){p->z=(int16_t)(done*left/2);p->vz=0;p->pose=POSE_JUMP;p->animation=(int16_t)(done/3);}
        else{p->z=0;p->vz=0;p->pose=POSE_IDLE;p->animation=0;c->mount_mask|=(uint8_t)(1u<<(6+h));}
    }
}
