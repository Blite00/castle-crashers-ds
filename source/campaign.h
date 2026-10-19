#ifndef CAMPAIGN_H
#define CAMPAIGN_H
#include "protocol.h"
#include "campaign_sequence.h"
#include "campaign_combat.h"
#include "campaign_weapons.h"
#include "pets.h"
#include "campaign_war.h"
#include "campaign_hubs.h"
#include "campaign_boss.h"
#define CAMPAIGN_MAGIC 0xCA03u
#define CAMPAIGN_VERSION 42u
#define CAMPAIGN_MAP 3
#define CAMPAIGN_SCENES 6
#define CAMPAIGN_ENEMIES 6
#define CAMPAIGN_DROPS 4
#define CAMPAIGN_PENDING_DROPS 48
#define CAMPAIGN_WAVES (KEEP_TOTAL_ENEMY_COUNT+6)
enum { KEEP_INTRO,KEEP_TRAVEL,KEEP_FIGHT,KEEP_ABDUCTION,KEEP_ENDING,KEEP_COMPLETE,KEEP_PAN_IN,KEEP_PAN_OUT,KEEP_STORE_FIGHT,KEEP_STORE,KEEP_DOOR,KEEP_BARRACKS_INTRO,KEEP_BARRACKS_TRAVEL };
#define CAMPAIGN_WORLD_MAP 22
#define CAMPAIGN_BARRACKS_MAP 5
#define CAMPAIGN_BARRACKS_INTRO_TICKS 486
static const int16_t campaign_barrel_positions[5][2]={{312,358},{114,297},{96,313},{63,344},{644,355}};
enum { DROP_NONE,DROP_COIN,DROP_BAG,DROP_FOOD,DROP_CHICKEN,DROP_GEM,DROP_POTION=6,DROP_WEAPON=7,DROP_PET=8,DROP_BOW=9,DROP_CROC=10 };
typedef struct {uint32_t xp;uint16_t gold;uint8_t level,knight;} CampaignProfile;
typedef struct {int16_t x,y;uint8_t kind,value;} CampaignDrop;
/* Clients render host-authoritative state; no client combat timers required. */
typedef struct {
    int16_t x,y,hp;
    uint8_t mana,animation,pose,flags,z,freeze,burn;
    uint8_t event_attack_hit,event_block_magic,event_arrow_equip,cooldown,attack_kind;
} CampaignActor;
_Static_assert(sizeof(CampaignActor)==18,"Compact actor layout");
typedef struct {
    uint16_t magic,version;uint32_t sequence;
    CampaignActor actors[9];Projectile shots[2];uint32_t tick;
    CampaignProfile profiles[2];CampaignDrop drops[CAMPAIGN_DROPS];
    uint16_t countdown,round,timer;
    uint8_t running,winner,map_id,phase,group,spawned,kills,completed,revive[2];
    uint8_t reserved[8];
} CampaignPacket;
_Static_assert(sizeof(CampaignPacket)==276,"Campaign radio budget");
typedef struct Campaign {
    uint8_t paused; /* Host pause, carried in snapshot running bit 1. */
    WarState war;
    uint8_t pickup_seq[2],pickup_kind[2],pickup_value[2],pickup_timer[2];
    CampaignCombatState combat;
    Fighter enemies[CAMPAIGN_ENEMIES];uint8_t targets[CAMPAIGN_ENEMIES],species[CAMPAIGN_ENEMIES]; /* species: see species.h (wire: attack_kind high nibble) */
    CampaignProfile profiles[2];CampaignDrop drops[CAMPAIGN_DROPS];
    uint16_t drop_age[CAMPAIGN_DROPS];
    CampaignDrop pending_drops[CAMPAIGN_PENDING_DROPS];uint8_t pending_count;
    Fighter ally;
    uint32_t rng;
    uint16_t broken,break_tick,timer;
    WeaponSet owned_weapons[2];
    uint8_t weapons[2],bow_on[2];
    uint8_t shovel_equipped[2],dig_timer[2];
    PetProfile pets[2];PetFollower pet_followers[2];uint8_t tools[2];
    uint8_t stats[2][4],menu_ack[2];
    uint8_t story_flags,potions[2],item_held[2],break_hp[16],break_hit_seq[16][2];
    uint8_t owlet_state[2],owlet_timer[2],fruit_taken,mount_mask;uint16_t troll_timer[2]; /* mount_mask: enemy slots riding the LEVEL18 crocodile (wire: enemy flag 64). */ /* Host only: Owlet fetches fruit (LEVEL18 tree). */
    uint64_t completed; /* Level-clear bit set, one bit per level id (CONTENT_LEVEL_IDS). */
    uint8_t phase,group,spawned,kills,revive[2],drop_serial,npc_ticks;
    /* Post-level score tally (MAIN f_ScoreTally): per hero kills this level, and the xp and gold held when the level began. */
    uint32_t tally_xp0[2];uint16_t tally_gold0[2],level_kills[2],tally_ticks;uint8_t tally_valid;
    /* Crocodile mount (host only): ticks before a dismounted hero may mount again, and the jump onto the mount in progress. */
    uint8_t mount_cool[2],mount_arc[2];int16_t mount_from[2][2],mount_to[2][2];
} Campaign;
#define CAMPAIGN_TALLY_TICKS 360
/* Values shown on a hero's score sign: kills, gold gained, experience gained. */
void campaign_tally_values(const Campaign *c,unsigned hero,unsigned out[3]);
/* Host: advances the sign; returns 1 when the party leaves for the world map. */
int campaign_tally_step(Campaign *c,Arena *a,const uint16_t keys[2]);
int campaign_select_item(Campaign *c,unsigned hero,unsigned item);
/* The bow is dropped by thieves; sandwich acquisition is not implemented, so never advertise it. */
static inline unsigned campaign_item_mask(const Campaign *c,unsigned hero){return hero<2?((c->potions[hero]?2u:0u)|((c->tools[hero]&1)?4u:0u)|((c->tools[hero]&2)?8u:0u)):0;}
int campaign_attack_contact(const Fighter *p);
void campaign_award_hit(Campaign *c,Arena *a,unsigned hero);
void campaign_queue_drop(Campaign *c,int x,int y,unsigned kind,unsigned value);
int campaign_pet_place(Campaign *c,unsigned id,int x,int y,int unlock);
int campaign_dig_buddy(Campaign *c,const Arena *a,unsigned hero,int completed);
int campaign_bag_place(Campaign *c,int x,int y);
/* The crocodile mount of LEVEL18 left standing where its rider fell; heroes jump on it and jump off again. mount_mask bits 6 and 7 mark a mounted hero. */
int campaign_croc_place(Campaign *c,int x,int y,int facing);
void campaign_mount_step(Campaign *c,Arena *a,uint16_t keys[2]);
/* Applies the leap onto the mount after the combat tick, so the arena step cannot undo it. */
void campaign_mount_arc(Campaign *c,Arena *a);
int campaign_weapon_place(Campaign *c,unsigned slot,int x,int y);
void campaign_world_map(Campaign *c,Arena *a);
int campaign_world_enter(Campaign *c,Arena *a,unsigned node);
/* Clear bits: Keep1, War2, Boss4, Home Castle8, Forest Entrance16. Later clears imply
 * prerequisites for saves predating independent Home Castle progression. */
static inline int campaign_world_unlocked(const Campaign *c,unsigned node){
#ifdef LEVEL_SELECT_TEST
    (void)c;return node<9;
#endif
    return node<=1?1:node==2?!!(c->completed&15):node==3?!!(c->completed&7):node==4?!!(c->completed&6):node==5?!!(c->completed&4):node==6?!!(c->completed&16):node==7?!!(c->completed&32):node==8?!!(c->completed&64):0;
}
int campaign_world_choose(Campaign *c,unsigned node);
void campaign_world_tick(Campaign *c,Arena *a);
#define CAMPAIGN_WORLD_NODES 9u
/* Positions in the 672x320 map crop whose origin is original-map (128,864): Blacksmith, Home, Keep, War, Boss, Forest Entrance, Thieves' Forest, Mill Run (original node 22, at map (146,-235)), Flooded Castle (node 23 at (259,-378)). The crop starts 128 rows above the first one. */
static const int16_t world_nodes[CAMPAIGN_WORLD_NODES][2]={{134,319},{219,310},{276,306},{401,315},{523,314},{645,241},{608,185},{457,170},{513,99}};
/* The original map paths bend: Barbarian Boss to Forest Entrance through (593,185), Forest Entrance to Thieves' Forest through
   (628,92) and (611,74). Each bent link is a polyline of intermediate points, walked at two pixels per tick. */
static const int16_t world_bends[2][2][2]={{{593,313},{-1,-1}},{{628,220},{611,202}}};
static inline int world_bend_set(unsigned from,unsigned to){
    unsigned lo=from<to?from:to,hi=from<to?to:from;return lo==4&&hi==5?0:lo==5&&hi==6?1:-1;
}
static inline unsigned world_polyline(unsigned from,unsigned to,int pts[4][2]){
    unsigned n=0;pts[n][0]=world_nodes[from][0];pts[n][1]=world_nodes[from][1];n++;
    int set=world_bend_set(from,to);
    if(set>=0)for(unsigned k=0;k<2;k++){
        unsigned i=from<to?k:1-k;if(world_bends[set][i][0]<0)continue;
        pts[n][0]=world_bends[set][i][0];pts[n][1]=world_bends[set][i][1];n++;
    }
    pts[n][0]=world_nodes[to][0];pts[n][1]=world_nodes[to][1];return n+1;
}
static inline unsigned world_seg(const int a[2],const int b[2]){
    int dx=b[0]-a[0],dy=b[1]-a[1];unsigned ux=(unsigned)(dx<0?-dx:dx),uy=(unsigned)(dy<0?-dy:dy);
    return ux>uy?ux+uy/2:uy+ux/2; /* octagonal length estimate, within 12 percent */
}
static inline unsigned world_walk_duration(unsigned from,unsigned to){
    if(world_bend_set(from,to)>=0){int p[4][2];unsigned n=world_polyline(from,to,p),len=0;for(unsigned i=1;i<n;i++)len+=world_seg(p[i-1],p[i]);return (len+1)/2;}
    int dx=world_nodes[to][0]-world_nodes[from][0];if(dx<0)dx=-dx;
    return (unsigned)(dx+1)/2;
}
static inline int campaign_world_travelling(const Campaign *c){return c->phase==CAMPAIGN_WORLD_MAP&&c->spawned!=c->group;}
static inline void campaign_world_position(const Campaign *c,int *x,int *y){
    unsigned from=c->spawned<CAMPAIGN_WORLD_NODES?c->spawned:1,to=c->group<CAMPAIGN_WORLD_NODES?c->group:1;
    unsigned duration=world_walk_duration(from,to),elapsed=c->timer;
    if(elapsed>duration)elapsed=duration;
    *x=world_nodes[from][0];*y=world_nodes[from][1];
    if(!duration)return;
    if(world_bend_set(from,to)>=0){
        int p[4][2];unsigned n=world_polyline(from,to,p),total=0;for(unsigned i=1;i<n;i++)total+=world_seg(p[i-1],p[i]);
        unsigned along=total*elapsed/duration;
        for(unsigned i=1;i<n;i++){
            unsigned len=world_seg(p[i-1],p[i]);
            if(along<=len||i==n-1){if(!len)len=1;if(along>len)along=len;*x=p[i-1][0]+(p[i][0]-p[i-1][0])*(int)along/(int)len;*y=p[i-1][1]+(p[i][1]-p[i-1][1])*(int)along/(int)len;return;}
            along-=len;
        }
        return;
    }
    *x+=(world_nodes[to][0]-*x)*(int)elapsed/(int)duration;*y+=(world_nodes[to][1]-*y)*(int)elapsed/(int)duration;
}

void campaign_start(Campaign *c,Arena *a,int coop);
void campaign_cinematic_tick(Campaign *c,Arena *a);
void campaign_combat_tick(Campaign *c,Arena *a,const uint16_t keys[2]);
void campaign_step(Campaign *c,Arena *a,const uint16_t keys[2]);
void campaign_pack(CampaignPacket *p,const Campaign *c,const Arena *a,uint32_t sequence);
/* Test and tool helper: feed a guest a full 32-snapshot cycle so every rotating field arrives. */
void campaign_sync_cycle(const Campaign *c,const Arena *a,Campaign *guest,Arena *remote,uint32_t first);
int campaign_packet_valid(const CampaignPacket *p);
void campaign_unpack(const CampaignPacket *p,Campaign *c,Arena *a);
const char *campaign_objective(const Campaign *c);
static inline unsigned campaign_equipped_item(const Campaign *c,unsigned hero){return hero>=2?0:(c->tools[hero]&2)&&c->bow_on[hero]?3u:c->shovel_equipped[hero]?2u:(c->potions[hero]&&(c->story_flags&(2u<<hero)))?1u:0u;}
static inline int campaign_potion_equipped(const Campaign *c,unsigned hero){return hero<2&&c->potions[hero]&&(c->story_flags&(2u<<hero));}
/* FX DefineSprite881: age0 is authored frame1; pickup frame20, remove304.
 * Each item has its own clock, independent of corpse/wave/global tick. */
static inline int campaign_drop_height(const Campaign *c,unsigned slot){
    static const uint8_t bounce[20]={6,22,34,43,48,50,50,48,46,44,40,27,15,3,0,4,8,6,4,0};
    return slot<CAMPAIGN_DROPS&&c->drops[slot].kind!=DROP_WEAPON&&c->drops[slot].kind!=DROP_PET&&c->drops[slot].kind!=DROP_CROC&&c->drop_age[slot]<20?bounce[c->drop_age[slot]]:0;
}
static inline int campaign_drop_visible(const Campaign *c,unsigned slot){
    if(slot>=CAMPAIGN_DROPS||!c->drops[slot].kind)return 0;
    if(c->drops[slot].kind==DROP_WEAPON||c->drops[slot].kind==DROP_PET||c->drops[slot].kind==DROP_CROC)return 1; /* Existing persistent swaps. */
    unsigned frame=c->drop_age[slot]+1;
    if(frame>=304)return 0;
    if(frame<262)return 1;
    if(frame<=278)return (frame-262)%4!=0;
    if(frame<=290)return (frame-278)%3!=0;
    return frame&1;
}
unsigned campaign_roll_drop(Campaign *campaign,unsigned *value);
int campaign_max_health(unsigned level);
unsigned campaign_stat_budget(unsigned level);
unsigned campaign_points(const Campaign *c,unsigned hero);
ArenaCampaignAttributes campaign_attributes(const Campaign *c,unsigned hero);
int campaign_health(const Campaign *c,unsigned hero);
int campaign_mana(const Campaign *c,unsigned hero);
int campaign_equip(Campaign *c,Arena *a,unsigned hero,unsigned slot);
int campaign_spend(Campaign *c,Arena *a,unsigned hero,unsigned stat);
#endif
