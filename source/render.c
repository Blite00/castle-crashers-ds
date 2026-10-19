#include <stdlib.h>
#include <string.h>
#include "selector_art.h"
#include "item_selector.h"
#include "render.h"
#include "cinematic.h"
#include "castle_art.h"
#include "weapon_art.h"
#include "barbarian_packed.h"
#include "thief_art.h"
#include "loot_art.h"
#include "pickup_feedback_art.h"
#include "neutral_npc_art.h"
#include "campaign_world.h"
#include "war_art.h"
#include "forest_art.h"
#include "thieves_art.h"
#include "deer_art.h"
#include "dance_art.h"
#include "tally_art.h"
#include "deer_actor_art.h"
#include "flood_actor_art.h"
#include "flood_art.h"
#include "campaign_flood.h"
#include "campaign_deer.h"
#include "forest_thief_art.h"
#include "thieves_thief_art.h"
#include "troll_art.h"
#include "thieves_scene_art.h"
#include "lobby_art.h"
#include "campaign_breaks.h"
#include "campaign_thieves_scenes.h"
#include "imp_art.h"
#include "mother_art.h"
#include "species.h"
#include "forest_props_art.h"
#include "forest_croc_art.h"
#include "rider_art.h"
#include "campaign_forest.h"
#include "campaign_thieves.h"
#include "boss_art.h"
#include "boss_actor_art.h"
#include "boss_door_art.h"
#include "boss_gold.h"
#include "blacksmith_art.h"
#include "hub_ambient_art.h"
#include "frog_art.h"
#include "ark_art.h"
#include "war_alternate_art.h"
#include "original_worldmap.h"
#include "pet_art.h"
#include "dig_art.h"
#include "bridge_art.h"
#include "war_machine_art.h"
#include "war_cannon_art.h"
#include "war_live_art.h"
#include "buried_art.h"
#include "war_choreography.h"
#include "campaign_shop.h"
#include "shop_board_art.h"
#include "campaign_barracks.h"
#include "barracks_actor_art.h"
#include "barracks_choreography.h"
#include "barracks_door_art.h"
#include "barracks_nearwall.h"
#include "run_dust_art.h"
#include "barracks_rail.h"
#include "arena_background.h"
#include "arena_king.h"
#include "arena_volcano.h"
#include "arena_occlusion_packed.h"
#include "arena_terrain_packed.h"
#include "campaign_breakables.h"
static const char *banner_title,*banner_keys;
#include "menu.h"
#include "effects.h"
#include "camera.h"
#include "raster.h"
#include "stage.h"
#include "campaign_war.h"
#include <string.h>
#include <stdio.h>
#ifdef ARM9
#include <nds.h>
#endif
typedef struct {
    const uint16_t *palette;
    const uint8_t *pixels,*occlusion,*layer_pixels;
    const ArenaSceneryLayer *layers;
    int prop_count,foreground_index;
} Scene;
static Scene scenes[]={
    {arena_palette,NULL,NULL,arena_layer_pixels,arena_layers,ARENA_PROP_COUNT,ARENA_FOREGROUND_INDEX},
    {king_palette,NULL,NULL,king_layer_pixels,king_layers,KING_PROP_COUNT,KING_FOREGROUND_INDEX},
    {volcano_palette,NULL,NULL,volcano_layer_pixels,volcano_layers,VOLCANO_PROP_COUNT,VOLCANO_FOREGROUND_INDEX}
};
_Static_assert(sizeof(scenes)/sizeof(scenes[0])==STAGE_ARENA_COUNT,"Map art count");
_Static_assert(ARENA_PROP_COUNT<=32&&KING_PROP_COUNT<=32&&VOLCANO_PROP_COUNT<=32,"Prop depth capacity");
/* BG3 is a horizontal ring. Rows318..511 are never written here: the
   alternate actor overlay occupies rows320..511. Only one strip is decoded. */
static Scene campaign_scene={campaign_world_palette,NULL,NULL,NULL,NULL,0,-1};
/* Arena masks and campaign scenery are never drawn together. Reuse their
   scratch storage so both-player campaigns retain headroom for the radio. */
static union {
    uint8_t arena[WORLD_WIDTH*WORLD_HEIGHT];
    struct {
        uint16_t terrain[512*WORLD_HEIGHT/2];
        uint8_t scratch[CAMPAIGN_WORLD_STRIP_WIDTH*CAMPAIGN_WORLD_HEIGHT>DEER_ACTOR_PIXELS_MAX?(CAMPAIGN_WORLD_STRIP_WIDTH*CAMPAIGN_WORLD_HEIGHT>THIEVES_SCENE_FRAME_PIXELS_MAX?CAMPAIGN_WORLD_STRIP_WIDTH*CAMPAIGN_WORLD_HEIGHT:THIEVES_SCENE_FRAME_PIXELS_MAX):(DEER_ACTOR_PIXELS_MAX>THIEVES_SCENE_FRAME_PIXELS_MAX?DEER_ACTOR_PIXELS_MAX:THIEVES_SCENE_FRAME_PIXELS_MAX)];
    } campaign;
} scenery_scratch __attribute__((aligned(32)));
#define arena_mask scenery_scratch.arena
#define world_strip scenery_scratch.campaign.scratch
#define world_foreground scenery_scratch.campaign.scratch
#define world_staging scenery_scratch.campaign.terrain
_Static_assert(BARRACKS_ACTOR_MAX_PIXELS<=32*448,"Barracks actor must fit shared strip scratch");
_Static_assert(CAMPAIGN_WORLD_FG_MAX_PIXELS<=sizeof(world_strip),"Foreground scratch budget");
/* 1: changed columns only, 2: rows moved or cache rebuilt. */
static int world_dirty;
static uint16_t world_columns;
static unsigned terrain_bytes_uploaded;
static uint16_t *world_surface,*world_hw_palette;
#ifndef ARM9
static uint16_t software_world_surface[512*WORLD_HEIGHT/2];
#endif
static int world_slots[16],world_ready,world_y,world_map=-1;
static int worldmap_cached;
static ArenaCamera background_camera;
static int mask_map=-1,terrain_map=-1;
static const uint16_t *active_terrain_palette;
/* Per-map campaign artwork. Each map's generated header (or chapter bank view) is reached through one switch, so a new
   level adds one case to each helper. */
static const uint16_t *map_palette(unsigned map){
    switch(map){
    case 15:return campaign_flood_palette;case 14:return campaign_deer_palette;case 13:return campaign_thieves_palette;case 12:return campaign_forest_palette;case 11:return campaign_boss_palette;
    case 10:return campaign_frog_palette;case 9:return campaign_ark_palette;case 8:return campaign_blacksmith_palette;
    case 7:return campaign_bridge_palette;case 6:return campaign_war_palette;case 5:return campaign_barracks_palette;
    case 4:return campaign_shop_palette;default:return campaign_world_palette;
    }
}
static int map_rows(unsigned map,unsigned strip,unsigned first,unsigned count,uint8_t *out){
    switch(map){
    case 15:return campaign_flood_decode_rows(strip,first,count,out);case 14:return campaign_deer_decode_rows(strip,first,count,out);case 13:return campaign_thieves_decode_rows(strip,first,count,out);case 12:return campaign_forest_decode_rows(strip,first,count,out);
    case 11:return campaign_boss_decode_rows(strip,first,count,out);case 10:return campaign_frog_decode_rows(strip,first,count,out);
    case 9:return campaign_ark_decode_rows(strip,first,count,out);case 8:return campaign_blacksmith_decode_rows(strip,first,count,out);
    case 7:return campaign_bridge_decode_rows(strip,first,count,out);case 6:return campaign_war_decode_rows(strip,first,count,out);
    case 5:return campaign_barracks_decode_rows(strip,first,count,out);case 4:return campaign_shop_decode_rows(strip,first,count,out);
    default:return campaign_world_decode_rows(strip,first,count,out);
    }
}
static unsigned map_fg_count(unsigned map){
    switch(map){
    case 15:return CAMPAIGN_FLOOD_FG_COUNT;case 14:return CAMPAIGN_DEER_FG_COUNT;case 13:return CAMPAIGN_THIEVES_FG_COUNT;case 12:return CAMPAIGN_FOREST_FG_COUNT;case 11:return CAMPAIGN_BOSS_FG_COUNT;
    case 10:return CAMPAIGN_FROG_FG_COUNT;case 9:return CAMPAIGN_ARK_FG_COUNT;case 8:return CAMPAIGN_BLACKSMITH_FG_COUNT;
    case 7:return CAMPAIGN_BRIDGE_FG_COUNT;case 6:return CAMPAIGN_WAR_FG_COUNT;case 3:return CAMPAIGN_WORLD_FG_COUNT;default:return 0;
    }
}
static const CampaignRouteLayer *map_fg_layer_or_null(unsigned map,unsigned layer){
    switch(map){
    case 15:return &campaign_flood_fg_layers[layer];case 14:return &campaign_deer_fg_layers[layer];case 13:return &campaign_thieves_fg_layers[layer];case 12:return &campaign_forest_fg_layers[layer];case 11:return &campaign_boss_fg_layers[layer];
    case 10:return &campaign_frog_fg_layers[layer];case 9:return &campaign_ark_fg_layers[layer];case 8:return &campaign_blacksmith_fg_layers[layer];
    case 7:return &campaign_bridge_fg_layers[layer];case 6:return &campaign_war_fg_layers[layer];default:return NULL;
    }
}
static int map_fg_depth(unsigned map,unsigned layer){
    const CampaignRouteLayer *r=map_fg_layer_or_null(map,layer);return r?r->depth:campaign_world_fg_layers[layer].depth;
}
static int map_fg_decode(unsigned map,unsigned layer,uint8_t *out){
    switch(map){
    case 15:return campaign_flood_decode_fg(layer,out);case 14:return campaign_deer_decode_fg(layer,out);case 13:return campaign_thieves_decode_fg(layer,out);case 12:return campaign_forest_decode_fg(layer,out);case 11:return campaign_boss_decode_fg(layer,out);
    case 10:return campaign_frog_decode_fg(layer,out);case 9:return campaign_ark_decode_fg(layer,out);case 8:return campaign_blacksmith_decode_fg(layer,out);
    case 7:return campaign_bridge_decode_fg(layer,out);case 6:return campaign_war_decode_fg(layer,out);default:return campaign_world_decode_fg(layer,out);
    }
}
/* Decoded depth layers are kept between frames: trees and ruins that overlap a fighter are redrawn every frame, and re-inflating
   them each time dominated frame cost in the longer levels. */
#define FG_CACHE_POOL 86016u
#define FG_CACHE_ENTRIES 40
/* One ring of pixels shared by variable-size layers: dense thickets hold many small layers, which a few fixed slots could not keep. */
static uint8_t fg_pool[FG_CACHE_POOL] __attribute__((aligned(4)));
static struct {int map,layer;unsigned off,size,stamp;} fg_entry[FG_CACHE_ENTRIES];
static unsigned fg_head,fg_clock,foreground_decodes;
static void fg_cache_flush(void){for(int i=0;i<FG_CACHE_ENTRIES;i++)fg_entry[i].map=0;fg_head=0;}
static const uint8_t *fg_cached(unsigned map,unsigned layer,unsigned size){
    int free_slot=-1;
    for(int i=0;i<FG_CACHE_ENTRIES;i++){
        if(fg_entry[i].map==(int)map+1&&fg_entry[i].layer==(int)layer)return fg_pool+fg_entry[i].off;
        if(!fg_entry[i].map&&free_slot<0)free_slot=i;
    }
    if(size>FG_CACHE_POOL)return NULL;
    if(fg_head+size>FG_CACHE_POOL)fg_head=0;
    unsigned lo=fg_head,hi=fg_head+size;
    for(int i=0;i<FG_CACHE_ENTRIES;i++)if(fg_entry[i].map&&fg_entry[i].off<hi&&fg_entry[i].off+fg_entry[i].size>lo){fg_entry[i].map=0;if(free_slot<0)free_slot=i;}
    if(free_slot<0){free_slot=0;unsigned oldest=~0u;for(int i=0;i<FG_CACHE_ENTRIES;i++)if(fg_entry[i].stamp<oldest){oldest=fg_entry[i].stamp;free_slot=i;}}
    if(!map_fg_decode(map,layer,fg_pool+lo))return NULL;
    fg_entry[free_slot].map=(int)map+1;fg_entry[free_slot].layer=(int)layer;fg_entry[free_slot].off=lo;fg_entry[free_slot].size=size;fg_entry[free_slot].stamp=++fg_clock;
    fg_head=hi;foreground_decodes++;return fg_pool+lo;
}
static int lobby_loaded; /* Lobby bitmaps currently sit in the scenery workspace. */
static void prepare_arena_surface(unsigned map);
static const Scene *scene_for(unsigned map){
    if(map>=CAMPAIGN_MAP){
        if(world_map!=(int)map){for(int i=0;i<16;i++)world_slots[i]=-1;world_ready=0;world_map=map;mask_map=-1;fg_cache_flush();}
        campaign_scene.palette=map_palette(map);
        return &campaign_scene;
    }
    if(map>=STAGE_ARENA_COUNT)map=0;
    prepare_arena_surface(map);
    scenes[map].occlusion=arena_mask;
    return &scenes[map];
}
static void world_invalidate(void){fg_cache_flush();worldmap_cached=0;for(int i=0;i<16;i++)world_slots[i]=-1;world_ready=0;}
void *arena_cinematic_audio_workspace(unsigned bytes){
    if(bytes>49152)return NULL;
    cinematic_reset();
    world_invalidate();mask_map=terrain_map=-1;world_dirty=0;
    return &scenery_scratch;
}
static unsigned terrain_rows_decoded;
unsigned arena_render_terrain_rows(void){return terrain_rows_decoded;}
unsigned arena_render_terrain_upload_bytes(void){return terrain_bytes_uploaded;}
static void world_rows(int strip,int first,int count,int destination){
    if(count<=0)return;
    int ok=map_rows((unsigned)world_map,(unsigned)strip,(unsigned)first,(unsigned)count,world_strip);
    if(!ok)memset(world_strip,0,(unsigned)count*32);
    terrain_rows_decoded+=(unsigned)count;
    int slot=strip&15;
    for(int y=0;y<count;y++){
        const uint8_t *src=world_strip+y*32;uint16_t *out=world_staging+(y+destination)*256+slot*16;
        for(int x=0;x<16;x++)out[x]=src[x*2]|((uint16_t)src[x*2+1]<<8);
    }
    world_columns|=(uint16_t)(1u<<slot);
    if(!world_dirty)world_dirty=1;
}
static void world_stream(const ArenaCamera *c){
#ifndef ARM9
    if(!world_surface)world_surface=software_world_surface;
#endif
    terrain_rows_decoded=0;if(!world_surface)return;
    terrain_map=-1;
    int origin=world_y;
    if(!world_ready||c->y<origin||c->y+c->height>origin+WORLD_HEIGHT){
        origin=c->y-(WORLD_HEIGHT-c->height)/2;
        if(origin<0)origin=0;
        if(origin>CAMPAIGN_WORLD_HEIGHT-WORLD_HEIGHT)origin=CAMPAIGN_WORLD_HEIGHT-WORLD_HEIGHT;
    }
    int shift=origin-world_y;
    if(!world_ready||shift)world_dirty=2;
    if(world_ready&&shift&&shift>-WORLD_HEIGHT&&shift<WORLD_HEIGHT){
        /* Preserve decoded rows during vertical pans. Previously even one pixel
           invalidated every strip, repeatedly inflating the entire backdrop. */
        unsigned rows=(unsigned)(shift<0?-shift:shift);
        if(shift>0)memmove(world_staging,world_staging+rows*256,(WORLD_HEIGHT-rows)*512);
        else memmove(world_staging+rows*256,world_staging,(WORLD_HEIGHT-rows)*512);
        int destination=shift>0?WORLD_HEIGHT-(int)rows:0;
        for(int slot=0;slot<16;slot++)if(world_slots[slot]>=0)world_rows(world_slots[slot],origin+destination,(int)rows,destination);
    }else if(!world_ready||shift){for(int slot=0;slot<16;slot++)world_slots[slot]=-1;}
    world_y=origin;
    int first=c->x/CAMPAIGN_WORLD_STRIP_WIDTH,last=(c->x+c->width-1)/CAMPAIGN_WORLD_STRIP_WIDTH;
    for(int strip=first;strip<=last;strip++){
        int slot=strip&15;if(world_slots[slot]==strip)continue;
        world_rows(strip,origin,WORLD_HEIGHT,0);world_slots[slot]=strip;
    }
    world_ready=1;background_camera=*c;background_camera.x=c->x&511;background_camera.y=c->y-origin;
}
static uint16_t world_color(int x,int y){
    if(!world_surface||x<0||x>=stage_world_width(world_map)||y<world_y||y>=world_y+WORLD_HEIGHT)return campaign_scene.palette[0];
    unsigned xx=(unsigned)x&511;uint16_t pair=world_staging[(y-world_y)*256+xx/2];
    return campaign_scene.palette[(pair>>((xx&1)*8))&255];
}
static unsigned world_index(int x,int y){
    if(!world_surface||x<0||x>=stage_world_width(world_map)||y<world_y||y>=world_y+WORLD_HEIGHT)return 0;
    unsigned xx=(unsigned)x&511,pair=world_staging[(y-world_y)*256+xx/2];
    return (pair>>((xx&1)*8))&255;
}
static uint16_t scene_color(const Scene *scene,int x,int y){
    if(scene==&campaign_scene)return world_color(x,y);
    if(!world_surface||x<0||x>=WORLD_WIDTH||y<0||y>=WORLD_HEIGHT)return 0x8000;
    unsigned pair=world_surface[y*256+x/2];return scene->palette[(pair>>((x&1)*8))&255];
}
static uint16_t arena_surface_color(int x,int y){
    if(!world_surface||x<0||x>=WORLD_WIDTH||y<0||y>=WORLD_HEIGHT)return 0x8000;
    unsigned pair=world_surface[y*256+x/2];return active_terrain_palette[(pair>>((x&1)*8))&255];
}
static void prepare_arena_surface(unsigned map){
#ifndef ARM9
    if(!world_surface)world_surface=software_world_surface;
#endif
    if(terrain_map!=(int)map&&world_surface){
        cinematic_reset();
        decode_arena_terrain(map,arena_mask);
        for(int y=0;y<WORLD_HEIGHT;y++)for(int x=0;x<WORLD_WIDTH;x+=2)world_surface[y*256+x/2]=arena_mask[y*WORLD_WIDTH+x]|((unsigned)arena_mask[y*WORLD_WIDTH+x+1]<<8);
        terrain_map=map;mask_map=-1;world_map=-1;world_dirty=0;world_invalidate();
    }
    active_terrain_palette=scenes[map].palette;
    if(world_hw_palette)memcpy(world_hw_palette,scenes[map].palette,512);
    if(mask_map!=(int)map){decode_arena_occlusion(map,arena_mask);mask_map=map;}
}
static int16_t layer_depth[32];
_Static_assert(sizeof(pose_count)/sizeof(pose_count[0])==POSE_HEAVY3,"Base poses precede the appended combo bank");
static const Campaign *campaign_state;
static const Fighter *campaign_enemy;
static int campaign_enemy_count;
void arena_render_campaign(const Campaign *state){campaign_state=state;campaign_enemy=state?state->enemies:NULL;campaign_enemy_count=state?CAMPAIGN_ENEMIES:0;}
void arena_render_enemy(const Fighter *enemy){campaign_state=NULL;campaign_enemy=enemy;campaign_enemy_count=enemy?1:0;}
static RasterView view;
static int opaque_scene;
int arena_render_is_opaque(void){return opaque_scene;}
/* A vertically moving intro is composed entirely into the hidden page. This
   removes live BG3 rebases rather than trying to fit them into VBlank. */
static RASTER_FAST void campaign_background_draw(void){
    for(int y=0;y<192;y++){
        const uint16_t *row=world_staging+(view.y[y]-world_y)*256;
        uint16_t *out=view.pixels+y*256;
        for(int x=0;x<256;x++){
            unsigned xx=(unsigned)view.x[x]&511,pair=row[xx>>1];
            out[x]=campaign_scene.palette[(pair>>((xx&1)*8))&255]|0x8000;
        }
        view.dirty_left[y]=0;view.dirty_right[y]=256;
    }
}
static ArenaCamera camera;
static int foreground_alpha=16,foreground_ready;
static uint16_t foreground_round;
#define RGB(r,g,b) (0x8000|((r))|((g)<<5)|((b)<<10))
static void rect(uint16_t *pixels,int x,int y,int w,int h,uint16_t c){
    for(int j=y;j<y+h;j++)if(j>=0&&j<192)for(int i=x;i<x+w;i++)if(i>=0&&i<256)pixels[j*256+i]=c;
}
static int knight_frame(const Fighter *p,int pose){
    /* Hold completed action poses, including the shield's three-frame raise.
       Replaying the raise while R was held caused the visible block jitter. */
    if(pose!=POSE_IDLE&&pose!=POSE_WALK&&pose!=POSE_CLASH&&p->animation>=pose_count[pose])return pose_count[pose]-1;
    return p->animation%pose_count[pose];
}
/* Authored keyframes retain their original timeline; duplicate in-between
   drawings are held so the two new actions need no extra image buffers. */
static unsigned combo_sample(const Fighter *p){
    unsigned t=p->animation>0?(unsigned)p->animation:0;
    if(p->pose==POSE_HEAVY3)return t<3?0:t<6?1:t<9?2:3;
    return t<5?4:t<8?5:t<11?6:t<14?7:t<19?8:9;
}
_Static_assert(DEFAULT_COMBO_PIXELS_MAX<=sizeof(world_foreground),"Combo scratch must fit existing scenery workspace");
static unsigned dash_sample(const Fighter *p){return (p->pose==POSE_DASHSLAM?3:0)+(p->animation<4?0:p->animation<11?1:2);}
_Static_assert(DASH_ATTACK_PIXELS_MAX<=sizeof(world_foreground),"Dash scratch fits existing scenery workspace");
_Static_assert(LOBBY_ART_PIXELS_MAX<=sizeof(world_foreground),"Lobby art fits the scenery workspace");
_Static_assert(THIEVES_SCENE_FRAME_PIXELS_MAX<=sizeof(world_foreground),"Cutscene frames fit the scenery workspace");
static uint8_t hero_cache[3][KNIGHT_WEAPON_FRAME_PIXELS_MAX];
#define ENEMY_CACHE_PIXELS (MOTHER_FRAME_PIXELS_MAX>BARBARIAN_FRAME_PIXELS_MAX?MOTHER_FRAME_PIXELS_MAX:BARBARIAN_FRAME_PIXELS_MAX)
static uint8_t enemy_cache[6][ENEMY_CACHE_PIXELS] __attribute__((aligned(32)));
/* Enemy species art shares the thief pose layout; weapon bits 5..7 select it (species.h). */
static const ArtFrame *species_frame(unsigned sp,unsigned id){
    switch(sp){case SPECIES_TROLL:return &troll_frames[id];case SPECIES_IMP:return &imp_frames[id];case SPECIES_TROLL_MOTHER:return &mother_frames[id];default:return NULL;}
}
static const uint16_t *species_palette(unsigned sp){
    switch(sp){case SPECIES_TROLL:return troll_palette;case SPECIES_IMP:return imp_palette;default:return mother_palette;}
}
static int species_decode(unsigned sp,unsigned id,uint8_t *out){
    switch(sp){case SPECIES_TROLL:return troll_decode_frame(id,out);case SPECIES_IMP:return imp_decode_frame(id,out);case SPECIES_TROLL_MOTHER:return mother_decode_frame(id,out);default:return 0;}
}
static int knight_cache_id[9]={-1,-1,-1,-1,-1,-1,-1,-1,-1};
static int knight_cache_weapon[9]={-1,-1,-1,-1,-1,-1,-1,-1,-1};
static void boss_cache_reset(void);
void arena_render_invalidate_assets(void){
    boss_cache_reset();
    cinematic_reset();world_invalidate();mask_map=terrain_map=world_map=-1;
    world_dirty=world_columns=0;foreground_ready=0;
    for(unsigned i=0;i<9;i++)knight_cache_id[i]=knight_cache_weapon[i]=-1;
}
void *arena_barracks_audio_workspace(unsigned bytes){
    if(bytes>sizeof(enemy_cache))return NULL;
    for(int i=2;i<8;i++)knight_cache_id[i]=-1;
    return enemy_cache;
}
static unsigned sprite_decodes;
unsigned arena_render_foreground_decodes(void){return foreground_decodes;}
unsigned arena_render_sprite_decodes(void){return sprite_decodes;}
static int sprite_visible(int x,int y,int w,int h){
    return x<camera.x+camera.width&&x+w>camera.x&&y<camera.y+camera.height&&y+h>camera.y;
}
/* Gate hit feedback is derived only from the synchronized gate health, so host
   and guest show the same door damage, shove and wood chips without wire changes. */
#pragma GCC push_options
#pragma GCC optimize("Os")
/* Cut-the-rope hint: a bold scissors beside the bound princess that snips while the rope can be cut. */
#define SC 20
static void scissors_line(uint8_t *buf,int x0,int y0,int x1,int y1,uint8_t color){
    int dx=abs(x1-x0),dy=abs(y1-y0),sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx-dy;
    for(;;){
        for(int o=0;o<2;o++){int xx=x0+o;if(xx>=0&&xx<SC&&y0>=0&&y0<SC)buf[y0*SC+xx]=color;}
        if(x0==x1&&y0==y1)break;
        int e=2*err;if(e>-dy){err-=dy;x0+=sx;}if(e<dx){err+=dx;y0+=sy;}
    }
}
static void scissors_draw(unsigned tick,int x,int y,int depth){
    static const uint16_t palette[4]={0,0x8000|(2|(1<<5)|(1<<10)),0x8000|(31|(31<<5)|(31<<10)),0x8000|(31|(22<<5)|(2<<10))};
    uint8_t *buf=world_foreground;memset(buf,0,SC*SC);
    int open=(tick%14)<8?7:2,bob=(int)((tick/4)%4);bob=bob>2?4-bob:bob;
    for(int pass=0;pass<2;pass++){
        uint8_t blade=pass?2:1,handle=pass?3:1;int g=pass?0:1;
        scissors_line(buf,5,17+g,10+open,0+g,blade);
        scissors_line(buf,14,17+g,9-open,0+g,blade);
        for(int r=0;r<2;r++){int cx=r?14:4;
            scissors_line(buf,cx-1,14+g,cx+2,14+g,handle);scissors_line(buf,cx-2,15+g,cx-2,18,handle);
            scissors_line(buf,cx+3,15+g,cx+3,18,handle);scissors_line(buf,cx-1,19,cx+2,19,handle);}
    }
    raster_sprite(&view,buf,palette,x-SC/2,y+bob,SC,SC,0,16,SC,depth);
}
#pragma GCC pop_options
#pragma GCC push_options
#pragma GCC optimize("Os")
/* Chips are a pure function of the last hit, so there is no particle pool to store. */
static struct {uint16_t hp,tick,hit_tick,hit_y;uint8_t known,chips;} gate_fx;
static void gate_fx_step(const Campaign *c,const Arena *a){
    unsigned hp=c->war.gate_hp;
    if(!gate_fx.known||a->tick<gate_fx.tick||a->tick-gate_fx.tick>8){memset(&gate_fx,0,sizeof gate_fx);gate_fx.known=1;gate_fx.hit_tick=a->tick-100;}
    else if(hp+2<=gate_fx.hp&&a->tick!=gate_fx.tick){ /* A melee swing removes 7; arrows remove 1 and make no chips. */
        gate_fx.hit_tick=a->tick;gate_fx.hit_y=a->p[0].y;gate_fx.chips=campaign_war_gate_chips(campaign_war_gate_frame(hp,a->p[1].ready?2:1));
    }
    gate_fx.hp=hp;gate_fx.tick=a->tick;
}
/* LEVEL18 cutscene props: the King, a litter carried by two soldiers and the thieves carrying the other princesses.
   Their clips idle on frame 1 until the cutscene starts them (King at 30, litter at 3, thieves at 87). */
static void forest_props_draw(int low,int high){
    static const int16_t depth[FOREST_PROP_COUNT]={238,212,185};
    if(!campaign_state)return;
    unsigned tick=campaign_state->timer,group=campaign_state->group;
    int cinema=campaign_state->phase==WAR_FOREST_CINEMA;
    for(unsigned prop=0;prop<FOREST_PROP_COUNT;prop++){
        if(depth[prop]<low||depth[prop]>=high)continue;
        unsigned start=prop==FOREST_PROP_KING?30:prop==FOREST_PROP_LITTER?3:87,frame=1;
        if(group>FOREST_WAVE_CROC||cinema)frame=cinema?(tick>=start?tick-start+1:1):prop==FOREST_PROP_KING?97:0;
        if(!frame)continue;
        int index=forest_prop_frame_for(prop,frame);if(index<0)continue;
        const ForestPropFrame *f=&forest_prop_frames[index];
        for(unsigned i=0;i<f->count;i++){
            const ForestPropTile *t=&forest_prop_tiles[f->first+i];
            if(!sprite_visible(t->x,t->y,t->w,t->h)||!forest_prop_decode_tile(f->first+i,world_foreground))continue;
            raster_sprite(&view,world_foreground,forest_prop_palette,t->x,t->y,t->w,t->h,0,16,t->h,depth[prop]);
        }
    }
}
static void gate_draw(const Campaign *c,const Arena *a,const CampaignWorldLayer *f,unsigned layer,const uint16_t *palette){
    gate_fx_step(c,a);
    unsigned frame=(c->war.flags&WAR_FLAG_GATE_OPEN)?6:campaign_war_gate_frame(c->war.gate_hp,a->p[1].ready?2:1);
    int shake=campaign_war_gate_shake((unsigned)(a->tick-gate_fx.hit_tick));
    if(frame==1){
        if(sprite_visible(f->x+shake,f->y,f->w,f->h)&&campaign_war_decode_fg(layer,world_foreground))raster_sprite(&view,world_foreground,palette,f->x+shake,f->y,f->w,f->h,0,16,f->h,32767);
    }else for(unsigned i=0;i<WAR_ALTERNATE_COUNT;i++){
        const CampaignRouteLayer *alt=&war_alternate_layers[i];if(alt->source!=133||alt->dynamic!=frame)continue;
        if(sprite_visible(alt->x+shake,alt->y,alt->w,alt->h)&&war_alternate_decode(i,world_foreground))raster_sprite(&view,world_foreground,palette,alt->x+shake,alt->y,alt->w,alt->h,0,16,alt->h,32767);
    }
    unsigned age=(uint16_t)(a->tick-gate_fx.hit_tick);
    for(unsigned k=0;k<gate_fx.chips&&age<60;k++){
        /* Source door_shrapnel: leftward 0..4, upward 4..13 per tick, gravity 3.5, one or two bounces. */
        unsigned h=(gate_fx.hit_tick*2654435761u+k*40503u)>>7;
        int vx=-(int)(h%10)*9/20,vz=9+(int)((h>>4)%18),limit=1+(int)((h>>9)&1),t=(int)age,x=WAR_GATE_X,z2=0;
        for(int arc=0;;arc++){
            int fall=(2*vz+6)/7;if(fall<1)fall=1;
            if(t<=fall){z2=vz*t-7*t*(t-1)/2;x+=vx*t;break;}
            x+=vx*fall;t-=fall;
            if(arc>=limit)break;
            vz/=2;
        }
        if(z2<0)z2=0;
        unsigned shape=(h>>11)&3;
        for(unsigned i=0;i<WAR_ALTERNATE_COUNT;i++){
            const CampaignRouteLayer *alt=&war_alternate_layers[i];if(alt->source!=1180||alt->placement!=shape)continue;
            int cx=x-alt->w/2,cy=gate_fx.hit_y-z2/2-alt->h/2;
            if(sprite_visible(cx,cy,alt->w,alt->h)&&war_alternate_decode(i,world_foreground))raster_sprite(&view,world_foreground,palette,cx,cy,alt->w,alt->h,(h>>13)&1,16,alt->h,32767);
            break;
        }
    }
}
#pragma GCC pop_options
static const uint8_t *frame_pixels(int id,int slot,unsigned weapon){
    uint8_t *cache=(slot>=2&&slot<8)?enemy_cache[slot-2]:hero_cache[slot==8?2:slot];
    if(knight_cache_id[slot]!=id||knight_cache_weapon[slot]!=(int)weapon){
        sprite_decodes++;
        if(slot<2||slot==8){
            if(!knight_decode_frame((unsigned)id,cache)||!weapon_art_apply(slot<2?(unsigned)slot:2u,(unsigned)id,weapon,cache))memset(cache,0,KNIGHT_WEAPON_FRAME_PIXELS_MAX);
            knight_cache_id[slot]=id;knight_cache_weapon[slot]=(int)weapon;return cache;
        }
        if(!(weapon>>5?species_decode(weapon>>5,(unsigned)id,cache):weapon&16?(world_map==13?thieves_thief_decode_frame:world_map==12?forest_thief_decode_frame:thief_decode_frame)((unsigned)id,cache):barbarian_decode_frame((unsigned)id,cache)))memset(cache,0,BARBARIAN_FRAME_PIXELS_MAX);
        knight_cache_id[slot]=id;knight_cache_weapon[slot]=(int)weapon;
    }
    return cache;
}
/* Keep's three fleeing peasants share an otherwise idle sixth enemy cache.
   Ownership marker invalidates these keys after enemy/audio cache reuse. */
static int8_t neutral_cache_id[3]={-1,-1,-1};
_Static_assert(BARBARIAN_FRAME_PIXELS_MAX>=4364,"Neutral cache must fit enemy slot");
static void neutral_actor(const Fighter *p,int index){
    static const uint16_t offsets[3]={0,1364,3284};
    static const uint16_t capacities[3]={1364,1920,1080};
    unsigned npc=(unsigned)(index-9),f=npc*2+(p->animation/4)%2;
    if(npc>=3)return;
    const ArtFrame *a=&neutral_npc_frames[f];
    if(!sprite_visible(p->x+a->x,p->y+a->y,a->w,a->h))return;
    unsigned size=(unsigned)a->w*a->h;
    int shared=campaign_state&&!campaign_state->enemies[5].ready&&size<=capacities[npc];
    uint8_t *pixels=world_foreground;
    if(shared){
        if(knight_cache_id[7]!=-2){
            for(unsigned i=0;i<3;i++)neutral_cache_id[i]=-1;
            knight_cache_id[7]=-2;
        }
        pixels=enemy_cache[5]+offsets[npc];
    }
    if(!shared||neutral_cache_id[npc]!=(int)f){
        sprite_decodes++;
        const uint8_t *src=neutral_npc_pixels+a->offset;unsigned n=0;
        while(n<size){unsigned token=*src++,count=(token&127)+1;
            if(count>size-n){memset(pixels,0,size);return;}
            if(token&128){memset(pixels+n,*src++,count);n+=count;}
            else {memcpy(pixels+n,src,count);src+=count;n+=count;}
        }
        if(shared)neutral_cache_id[npc]=(int8_t)f;
    }
    raster_sprite(&view,pixels,neutral_npc_palette,p->x+a->x,p->y+a->y,a->w,a->h,0,16,a->h,p->y);
}
static void cannon_picture(unsigned frame,int x,int y,int flip,int depth){
    const WarCannonFrame *f=&war_cannon_frames[frame];
    for(unsigned i=0;i<f->count;i++){
        unsigned id=f->first+i;const WarCannonTile *t=&war_cannon_tiles[id];
        int left=x+(flip?-t->x-t->w+1:t->x),top=y+t->y;
        if(sprite_visible(left,top,t->w,t->h)&&war_cannon_decode_tile(id,world_foreground))raster_sprite(&view,world_foreground,war_cannon_palette,left,top,t->w,t->h,flip,16,t->h,depth);
    }
}
static uint32_t boss_tile_keys[24];
static unsigned boss_cache_next;
static void boss_cache_reset(void){memset(boss_tile_keys,0,sizeof(boss_tile_keys));boss_cache_next=0;}
static void boss_actor_picture(unsigned pose,unsigned tick,int x,int y,int flip,int depth){
    /* LEVEL7 sprite111 source269 replaces its departing actors with sprite110
       (camera/green-arrow guide shapes106/109). Keep240/260 departure keys;
       never paint the authoring guide once source269 is reached. */
    if(pose==BOSS_ACTOR_CARRIED&&tick>=268)return;
    unsigned frame=boss_actor_frame_for(pose,tick);const BossActorFrame *f=&boss_actor_frames[frame];
    /* Damage feedback changes only the boss colors, never its attack pose or
       movement. Keep the tint on the stack, not another permanent sprite bank. */
    uint16_t flash_palette[64];const uint16_t *palette=boss_actor_palette;
    if(pose<=BOSS_DIE&&campaign_state&&(campaign_state->war.machine.flash&1)){
        for(unsigned i=0;i<64;i++){
            unsigned c=boss_actor_palette[i];
            flash_palette[i]=RGB(((c&31)+31)/2,(((c>>5)&31)+31)/2,(((c>>10)&31)+31)/2);
        }
        palette=flash_palette;
    }
    for(unsigned i=0;i<f->count;i++){
        unsigned id=f->first+i;const BossActorTile *t=&boss_actor_tiles[id];
        int left=x+(flip?-t->x-t->w+1:t->x),top=y+t->y;
        if(!sprite_visible(left,top,t->w,t->h))continue;
        /* Associative lookup avoids evicting tiles of the same held pose just
           because their compressed offsets share a hash bucket. */
        uint32_t key=t->offset+1;unsigned slot=0;
        while(slot<24&&boss_tile_keys[slot]!=key)slot++;
        if(slot==24){
            for(slot=0;slot<24&&boss_tile_keys[slot];slot++);
            if(slot==24){slot=boss_cache_next;boss_cache_next=(boss_cache_next+1)%24;}
        }
        uint8_t *cached=NULL;
#if defined(ARM9) || defined(CHAPTER_ASSETS_TEST)
        /* Boss audio reserves this bank tail; never borrow a playing sample. */
        uint8_t *tail=chapter_assets_boss_tile_workspace();if(tail)cached=tail+slot*2048;
#endif
        uint8_t *decoded=cached?cached:world_foreground;
        if(!cached||boss_tile_keys[slot]!=key){if(!boss_actor_decode_tile(id,decoded))continue;if(cached)boss_tile_keys[slot]=key;}
        raster_sprite(&view,decoded,palette,left,top,t->w,t->h,flip,16,t->h,depth);
    }
}
static void machine_actor(const Fighter *p){
    unsigned pose=p->pose==POSE_DEAD?WAR_MACHINE_DIE:p->pose==POSE_CAST?WAR_MACHINE_JACK:p->pose==POSE_WALK?WAR_MACHINE_WALK:WAR_MACHINE_IDLE;
    const WarMachineFrame *frame=&war_machine_frames[war_machine_frame_for(pose,p->animation)];
    for(unsigned i=0;i<frame->count;i++){
        unsigned id=frame->first+i;const WarMachineTile *tile=&war_machine_tiles[id];
        int x=p->x+(p->facing<0?-tile->x-tile->w+1:tile->x),y=p->y+tile->y;
        if(!sprite_visible(x,y,tile->w,tile->h))continue;
        if(war_machine_decode_tile(id,world_foreground))raster_sprite(&view,world_foreground,war_machine_palette,x,y,tile->w,tile->h,p->facing<0,16,tile->h,p->y);
    }
}
/* A cut tuft or an opened chest: paint the ground over the baked object. Each opaque pixel of the object's own layer takes the colour
   of the ground beside the object on the same row, so the horizontal bands of the earth continue through the gap. */
static void break_cover(const Arena *a,const CampaignRouteLayer *r,int layer,int index,const Scene *scene){
    unsigned n;const BreakPoint *t=campaign_break_table(a->map_id,&n);if((unsigned)index>=n)return;
    if(!sprite_visible(r->x,r->y,r->w,r->h))return;
    const uint8_t *src=fg_cached(a->map_id,(unsigned)layer,(unsigned)(r->w*r->h));if(!src)return;
    /* The ground under a cut tuft is copied from beside it on the same rows, so the dirt keeps its horizontal texture; it is painted before any actor
       so heroes standing there are never covered by the patch. */
    int lo=camera.x+2,hi=camera.x+camera.width-3,shift=r->w+8,dir=(r->x-shift>=lo)?-1:1;
    uint8_t *tmp=world_foreground;
    for(int yy=0;yy<r->h;yy++)for(int xx=0;xx<r->w;xx++){
        unsigned v=src[yy*r->w+xx];int px=r->x+xx+dir*shift;px=px<lo?lo:px>hi?hi:px;
        tmp[yy*r->w+xx]=v?(uint8_t)world_index(px,r->y+yy):0;
    }
    (void)t;
    raster_sprite(&view,tmp,scene->palette,r->x,r->y,r->w,r->h,0,16,r->h,32767);
}
static unsigned render_map,sprite_tick; /* Current map and animation clock for scenery effects. */
/* The Thieves' Forest river (x 2654..2838): ripples spread around the feet of anyone wading, wider and quicker while moving. */
static void water_ripples(int x,int y,unsigned seed,int moving){
    if(render_map!=THIEVES_MAP||x<2654||x>2838)return;
    unsigned t=(unsigned)(sprite_tick/3)+seed;
    for(unsigned ring=0;ring<2;ring++){
        unsigned phase=(t+ring*5)%10;int w=6+(int)phase*(moving?3:2),h=1+(int)phase/5;
        raster_rect(&view,x-w,y-h,2*w,1,phase<7?RGB(24,29,31):RGB(16,24,28),y+1);
    }
}
static void draw_pet(unsigned id,int x,int y,int facing,unsigned tick,unsigned happy){
    unsigned frame=pet_art_frame(id,happy?PET_ART_HAPPY:PET_ART_FOLLOW,happy?60-happy:tick);
    if(frame>=PET_FRAME_COUNT)return;
    const ArtFrame *f=&pet_frames[frame];
    int left=x+(facing<0?-f->x-f->w+1:f->x),top=y-18+f->y;
    water_ripples(x,y,x,0);
    if(sprite_visible(left,top,f->w,f->h)&&pet_decode_frame(frame,world_foreground))raster_sprite(&view,world_foreground,pet_palette,left,top,f->w,f->h,facing<0,16,f->h,y);
}
static void war_live_sprite(unsigned id,int x,int y,int facing,int depth,int slot){
    const ArtFrame *f=&war_live_frames[id];int left=x+(facing<0?-f->x-f->w+1:f->x),top=y+f->y;
    if(!sprite_visible(left,top,f->w,f->h))return;
    _Static_assert(WAR_LIVE_PIXELS_MAX<=BARBARIAN_FRAME_PIXELS_MAX,"Sleeping actor cache capacity");
    uint8_t *out=slot>=2&&slot<8?enemy_cache[slot-2]:world_foreground;
    int key=4000+(int)id;
    if(out==world_foreground||knight_cache_id[slot]!=key){
        if(!war_live_decode(id,out))return;
        sprite_decodes++;
        if(out!=world_foreground){knight_cache_id[slot]=key;knight_cache_weapon[slot]=-1;}
    }
    raster_sprite(&view,out,war_live_palette,left,top,f->w,f->h,facing<0,16,f->h,depth);
}
/* Barracks party (LEVEL2): the grey knights and, as in the source (each live player goes to gotoAndStop("dance"+n)), the heroes dance the PLAYER
   dance1..dance4 clips while the band plays. Keys are every STRIDE source frames; every dancer has its own phase and style. */
static void dance_sprite(unsigned costume,unsigned style,unsigned phase,int x,int y,int flip,int depth){
    static const uint8_t stride[4]={1,3,4,1};
    const DanceClip *clip=&dance_clips[costume][style&3];if(!clip->count)return;
    unsigned id=clip->first+(sprite_tick/stride[style&3]+phase)%clip->count;const ArtFrame *f=&dance_frames[id];if(!f->w)return;
    int left=x+(flip?-f->x-f->w+1:f->x),top=y+f->y;
    if(!sprite_visible(left,top,f->w,f->h)||!dance_decode(id,world_foreground))return;
    raster_sprite(&view,world_foreground,dance_palette,left,top,f->w,f->h,flip,16,f->h,depth);
}
static int barracks_dancing(void){return campaign_state&&render_map==5&&campaign_state->phase==KEEP_BARRACKS_INTRO&&campaign_state->timer<425;}
/* LEVEL22: the deer, the knights riding it, the Mill Monster and the logs (tools/deer_actors.py). */
static void deer_sprite_op(unsigned group,unsigned key,int x,int y,int depth,int opacity){
    const DeerGroup *g=&deer_groups[group];if(!g->count)return;
    unsigned id=g->first+(key<g->count?key:g->count-1u);const ArtFrame *f=&deer_actor_frames[id];if(!f->w)return;
    int left=x+f->x,top=y+f->y;
    if(!sprite_visible(left,top,f->w,f->h)||!deer_actor_decode(id,world_foreground))return;
    raster_sprite(&view,world_foreground,deer_actor_palette,left,top,f->w,f->h,0,opacity,f->h,depth);
}
static void deer_sprite(unsigned group,unsigned key,int x,int y,int depth){deer_sprite_op(group,key,x,y,depth,16);}
static unsigned deer_rider_group(unsigned knight,unsigned clip){ /* clip: 0 walk, 1 toss, 2 slam, 3 scared, 4 dock, 5 mount */
    return DA_RIDER_BLUE_WALK+(knight&3u)*7u+clip; /* 6 is the pooping ride (deerpoo_walk) */
}
/* Returns 1 when the hero is drawn as a rider; intro poses stand beside the deer and use the normal knight art. */
static int deer_hero(const Fighter *p,int index){
    if(p->hp<=0){deer_sprite(DA_DEER_WALK,(unsigned)(sprite_tick/2)%11u,p->x,p->y,p->y);return 1;}
    unsigned anim=p->animation<0?0:(unsigned)p->animation,knight=p->knight_id;int y=p->y-p->z;
    switch(p->pose){
    case DEER_POSE_RUN:
        if(index==1&&campaign_state&&(campaign_state->story_flags&DEER_STORY_POOP)){ /* the source picks one deer to poop all the way: deer_poop and the deerpoo_walk ride */
            unsigned k=(anim/3u)%3u;deer_sprite(DA_DEER_POOP,k,p->x,p->y,p->y);deer_sprite(deer_rider_group(knight,6),k,p->x,p->y,p->y);return 1;
        }
        deer_sprite(DA_DEER_WALK,anim,p->x,p->y,p->y);deer_sprite(deer_rider_group(knight,0),anim,p->x,p->y,p->y);return 1;
    case DEER_POSE_JUMP:deer_sprite(DA_DEER_JUMP,anim,p->x,y,p->y);deer_sprite(deer_rider_group(knight,0),3,p->x,y,p->y);
        raster_rect(&view,p->x-20,p->y-2,40,4,RGB(8,12,12),p->y-1);return 1;
    case DEER_POSE_SLAM:deer_sprite(deer_rider_group(knight,2),anim,p->x,p->y,p->y);return 1;
    case DEER_POSE_TOSS:deer_sprite(deer_rider_group(knight,1),anim,p->x,p->y,p->y);return 1;
    case DEER_POSE_DOCK:deer_sprite(DA_DEER_DOCK,anim+(anim>7?2:0),p->x,y,p->y);deer_sprite(deer_rider_group(knight,4),anim,p->x,y,p->y);return 1;
    case DEER_POSE_MOUNT:deer_sprite(DA_DEER_SCARED,0,p->x+(index?-12:30),p->y,p->y-1);deer_sprite(deer_rider_group(knight,5),anim,p->x,p->y-p->z,p->y);return 1;
    default:return 0;
    }
}
static void deer_scene_draw(const Arena *a,const Campaign *c,int low,int high){
    int mx,key=campaign_deer_monster_key(c,&mx);
    if(key>=0&&190>=low&&190<high){
        deer_sprite(DA_MONSTER,(unsigned)key,mx,223,190);
    }
    /* Sprite 208, the wall the monster breaks through (it wobbles from frame 242 and breaks at 285), and the monster's arm (sprite 194, from frame 316). */
    if(c->timer<DEER_INTRO_TICKS+40&&400>=low&&400<high){
        unsigned t=c->timer,key=t<242?0u:t<285?1u+((t-242)/8)%4u:5u;
        deer_sprite(DA_OVERLAY,key,DEER_OVERLAY_X,DEER_OVERLAY_Y,400);
    }
    if(key==11&&400>=low&&400<high){ /* sprites 189 and 193: the claws open, slam and rake in 47-frame loops (every sixth frame is kept) */
        unsigned arm=(c->timer/6)%8u;
        deer_sprite(DA_ARM1,arm,mx-DEER_ARM_BEHIND-78,DEER_ARM_Y+85,400);deer_sprite(DA_ARM2,arm,mx-DEER_ARM_BEHIND,DEER_ARM_Y,400);
    }
    DeerLog logs[DEER_LOG_LIMIT];unsigned n=campaign_deer_logs(c,logs);
    for(unsigned i=0;i<n;i++){
        const DeerLog *l=&logs[i];int wall=l->kind>=10;
        if(l->debris){if(l->y>=low&&l->y<high)deer_sprite(DA_LOG,10u+(unsigned)l->debris,l->x,l->y,l->y);continue;}
        unsigned key=l->kind<10?l->kind-2u:l->kind-2u;
        if(wall){
            if(150>=low&&150<high){deer_sprite_op(DA_SHAFT,l->kind-10u,l->x,l->y,150,7);deer_sprite(DA_LOG,key,l->x,l->y,150);} /* the light on the floor shows where the doorway will be */
            if(400>=low&&400<high)deer_sprite(DA_LOGFRONT,l->kind-10u,l->x,l->y,400);
        }else if(l->y>=low&&l->y<high)deer_sprite(DA_LOG,key,l->x,l->y,l->y);
    }
    /* The deer wait beside the party until they jump on (sprite 188 frames 280..305). */
    if(c->timer<DEER_INTRO_TICKS)for(int h=0;h<2;h++){
        const Fighter *p=&a->p[h];if(!p->ready||p->hp<=0||p->y<low||p->y>=high||(p->pose==DEER_POSE_MOUNT))continue;
        int x=p->x+(h?-12:30);
        if(c->timer>=280)deer_sprite(DA_DEER_SCARED,0,x,p->y,p->y);else deer_sprite(DA_DEER_IDLE,0,x,p->y,p->y);
    }
}
/* LEVEL7 bossdoor2 (sprite 202): the gate stands until the knock at frame 35 of the intro clip, little barbarians fall in front of it and the door
   falls on them; it then lies flat for the rest of the level. Steps are every third source frame (tools/boss_door.py). */
#define BOSS_DOOR_X 721
#define BOSS_DOOR_Y 256
static void boss_door_picture(unsigned id){
    const ArtFrame *f=&boss_door_frames[id];if(!f->w)return;
    int x=BOSS_DOOR_X+f->x,y=BOSS_DOOR_Y+f->y;
    if(!sprite_visible(x,y,f->w,f->h)||!boss_door_decode(id,world_foreground))return;
    raster_sprite(&view,world_foreground,boss_door_palette,x,y,f->w,f->h,0,16,f->h,BOSS_DOOR_Y);
}
static void boss_door_draw(const Campaign *c){
    unsigned t=c->phase==BOSS_INTRO?c->timer:100000u,step=0;
    if(t>=35){unsigned frame=2+(t-35);for(unsigned i=1;i<BOSS_DOOR_STEPS;i++)if(boss_door_steps[i][0]<=frame)step=i;}
    if(step==0||!boss_door_steps[step][2])boss_door_picture(boss_door_steps[0][1]);
    if(step)boss_door_picture(boss_door_steps[step][1]);
}
/* LEVEL23: swimming knights, fat fish, bats, the Catfish, rocks and fins (tools/flood_actors.py). Sprites face left. */
static void flood_sprite(unsigned group,unsigned key,int x,int y,int depth,int flip){
    const FloodGroup *g=&flood_groups[group];if(!g->count)return;
    unsigned id=g->first+(key<g->count?key:g->count-1u);const ArtFrame *f=&flood_actor_frames[id];if(!f->w)return;
    int left=x+(flip?-f->x-f->w+1:f->x),top=y+f->y;
    if(!sprite_visible(left,top,f->w,f->h)||!flood_actor_decode(id,world_foreground))return;
    raster_sprite(&view,world_foreground,flood_actor_palette,left,top,f->w,f->h,flip,16,f->h,depth);
}
static int flood_hero(const Fighter *p,int index){
    if(p->hp<=0||p->z||(p->pose!=POSE_IDLE&&p->pose!=POSE_WALK))return 0;
    unsigned c=(unsigned)(p->knight_id&3u),k=(unsigned)(sprite_tick/3u);
    flood_sprite((p->pose==POSE_WALK?FA_SWIM_WALK_BLUE:FA_SWIM_STAND_BLUE)+c*3u-(p->pose==POSE_WALK?0u:0u),k%6u,p->x,p->y,p->y,p->facing<0);
    (void)index;return 1;
}
static void flood_enemy(const Fighter *p){
    int flip=p->facing>0,x=p->x,y=p->y-p->z;unsigned t=(unsigned)sprite_tick;
    switch(p->knight_id){
    case 0:flood_sprite(FA_BAT,(t/3u)%6u,x,y-34-(int)((t/6u)&1u)*2,p->y,flip);break;
    case 1:{
        unsigned attack=p->pose>=POSE_ATTACK&&p->pose<=POSE_FINISHER;
        if(p->hp<=0){flood_sprite(FA_CAT_DIE,p->animation/6u,x,y,p->y,flip);break;}
        if(attack)flood_sprite(p->pose==POSE_ATTACK?FA_CAT_PUNCH1:p->pose==POSE_LIGHT2?FA_CAT_PUNCH2:FA_CAT_PUNCH3,(unsigned)p->animation/3u,x,y,p->y,flip);
        else flood_sprite(FA_CAT_IDLE,(t/8u)%4u,x,y,p->y,flip);
        break;}
    default:
        if(p->hp<=0)flood_sprite(FA_FISH_DIE,p->animation/6u,x,y,p->y,flip);
        else if(p->pose==POSE_HURT||p->pose==POSE_AIRHURT||p->hitstun)flood_sprite(FA_FISH_HIT,(t/3u)%5u,x,y,p->y,flip);
        else if(p->z||(p->pose>=POSE_ATTACK&&p->pose<=POSE_FINISHER))flood_sprite(FA_FISH_JUMP,(unsigned)p->animation/3u,x,y,p->y,flip);
        else flood_sprite(FA_FISH_IDLE,(t/6u)%6u,x,y,p->y,flip);
    }
}
static void flood_scene_draw(const Campaign *c,int low,int high){
    FloodObject o[FLOOD_ROCK_LIMIT];
    unsigned n=campaign_flood_rocks(c->timer,o);
    for(unsigned i=0;i<n;i++)if(o[i].y>=low&&o[i].y<high)flood_sprite(FA_ROCK,0,o[i].x,o[i].y,o[i].y,0);
    n=campaign_flood_fins(c->timer,o);
    for(unsigned i=0;i<n;i++)if(o[i].y>=low&&o[i].y<high)flood_sprite(FA_FIN,0,o[i].x,o[i].y,o[i].y,0);
    if(c->break_tick>=1676&&FLOOD_KINGBOAT_Y>=low&&FLOOD_KINGBOAT_Y<high)flood_sprite(FA_BOAT,0,FLOOD_KINGBOAT_X,FLOOD_KINGBOAT_Y,FLOOD_KINGBOAT_Y,0);
}
static void knight(uint16_t *pixels,const Fighter *p,int index){
    if(index>=9){neutral_actor(p,index);return;}
    if(render_map==FLOOD_MAP&&campaign_state){if(index>=2){flood_enemy(p);return;}if(flood_hero(p,index))return;}
    if(index<2&&render_map==DEER_MAP&&campaign_state&&deer_hero(p,index))return;
    if(index<2&&p->hp>0&&barracks_dancing()){dance_sprite(p->knight_id&3u,(unsigned)index,(unsigned)index*5u,p->x,p->y,0,p->y);return;}
    if(index<2&&campaign_state&&campaign_state->phase==BOSS_FIGHT){
        unsigned remaining=campaign_state->break_hp[14+index],age=campaign_state->break_hit_seq[15][index];
        unsigned source=remaining?(age<29?age+1:38-remaining):age>=30&&age<=33?age+8:0;
        if(source){
            unsigned id=buried_frame(p->knight_id,source);
            if(id<BURIED_FRAME_COUNT){
                _Static_assert(BURIED_PIXELS_MAX<=KNIGHT_WEAPON_FRAME_PIXELS_MAX,"Buried hero cache capacity");
                const ArtFrame *f=&buried_frames[id];uint8_t *out=hero_cache[index];int key=5000+(int)id;
                if(knight_cache_id[index]!=key){if(!buried_decode(id,out,world_foreground))return;knight_cache_id[index]=key;knight_cache_weapon[index]=-1;sprite_decodes++;}
                int left=p->x+(p->facing<0?-f->x-f->w+1:f->x);
                raster_sprite(&view,out,knight_palette,left,p->y+f->y,f->w,f->h,p->facing<0,16,f->h,p->y);
                return;
            }
        }
    }
    /* Movie363 enables AutoRun, not a mount. The original grants a horse only
       after its separate jump/proximity interaction; an on-foot hero stays so. */
    if(index>=2&&index<8&&p->knight_id==2&&p->hp>0){
        unsigned tick=campaign_state?campaign_state->timer:(unsigned)p->animation;
        war_live_sprite(20+(tick%60)/15,p->x,p->y,p->facing,p->y,index);return;
    }

    if(p->pose==POSE_DIG&&index<2){
        unsigned frame=dig_art_frame(p->knight_id,p->animation+1);const ArtFrame *f=&dig_frames[frame];
        int x=p->x+(p->facing<0?-f->x-f->w+1:f->x),y=p->y+f->y;
        if(sprite_visible(x,y,f->w,f->h)&&dig_decode_frame(frame,world_foreground))raster_sprite(&view,world_foreground,dig_palette,x,y,f->w,f->h,p->facing<0,16,f->h,p->y);
        return;
    }

    static uint16_t gray_palette[256];static int gray_ready;
    if(!gray_ready){for(int i=0;i<256;i++){unsigned c=knight_palette[i],r=c&31,g=(c>>5)&31,b=(c>>10)&31;if(b>r+3){unsigned v=(r+g+b)/3;c=0x8000|v|(v<<5)|(v<<10);}gray_palette[i]=c;}gray_ready=1;}
    int pose=p->pose>=0&&p->pose<POSE_COUNT?p->pose:0;
    int gray=index==8||(index>=2&&index<8&&p->knight_id==3);
    int enemy=index>=2&&index<8&&!gray;
    unsigned weapon=campaign_state&&index<2?campaign_state->weapons[index]:0;
    int thief=enemy&&p->knight_id==1;if(thief)weapon|=16;
    unsigned species=thief&&campaign_state&&index>=2&&index<8?campaign_state->species[index-2]:0;
    if(species>=SPECIES_COUNT)species=0;
    weapon|=species<<5;
    /* LEVEL18 mount2: the first thief rides a crocodile whose head bites when he attacks; his seat is 44 source pixels up. */
    int lift=0;
    if(!p->z&&p->hp>0)water_ripples(p->x,p->y,(unsigned)index*5,p->pose==POSE_WALK||p->pose==POSE_ROLL);
    if(species==SPECIES_TROLL_MOTHER&&render_map==THIEVES_MAP&&campaign_state&&campaign_state->war.gate_hp&&p->hp>0){
        /* The Troll Mother's wind-up: a blinking red exclamation mark shows that a charge is coming. */
        int top=p->y-p->z-118;if((campaign_state->war.gate_hp/3)&1){
            raster_rect(&view,p->x-2,top,4,13,RGB(31,4,3),p->y+2);raster_rect(&view,p->x-2,top+16,4,4,RGB(31,4,3),p->y+2);
            raster_rect(&view,p->x-3,top-1,6,1,RGB(31,25,24),p->y+2);}
    }
    if(thief&&p->pose==POSE_JUMP&&p->z==70&&p->hp>0&&campaign_state)
        { /* A thief hanging from the canopy holds a vine that reaches up out of the picture. */
          int top=p->y-p->z-210,bottom=p->y-p->z-34;raster_rect(&view,p->x-1,top,2,bottom-top,RGB(11,8,3),p->y);raster_rect(&view,p->x,top,1,bottom-top,RGB(7,16,4),p->y); }
    int ride=campaign_state&&render_map==FOREST_MAP&&((thief&&(campaign_state->mount_mask&(1u<<(index-2))))||(index<2&&(campaign_state->mount_mask&(1u<<(6+index)))));
    if(ride){
        lift=27;
        int bite=p->pose==POSE_ATTACK||p->pose==POSE_LIGHT2||p->pose==POSE_LIGHT3||p->pose==POSE_FINISHER||p->pose==POSE_HEAVY;
        unsigned croc=bite?1+(p->animation<17?(unsigned)(p->animation<0?0:p->animation):17):0;
        const ForestCrocFrame *cf=&forest_croc_frames[croc];
        int bob=(p->pose==POSE_WALK&&((p->animation/2)&1))?1:0; /* the crocodile rocks as it walks */
        int cx=p->x+(p->facing<0?-(cf->x+cf->w-1):cf->x),cy=p->y+cf->y-bob;
        raster_rect(&view,p->x-24,p->y-2,48,4,RGB(8,12,12),p->y-1);
        if(sprite_visible(cx,cy,cf->w,cf->h)&&forest_croc_decode(croc,world_foreground))raster_sprite(&view,world_foreground,forest_croc_palette,cx,cy,cf->w,cf->h,p->facing<0,16,cf->h,p->y-1);
        /* The rider sits in the original horse_* clips (PLAYER sprites 1025..1034): idle, walk, two punches, bow and jump. */
        unsigned costume=thief?4u:(unsigned)p->knight_id&3,key=0;int mapped=1;
        switch(pose){
        case POSE_IDLE:key=RIDER_IDLE;break;
        case POSE_WALK:key=RIDER_WALK+(unsigned)(p->animation<0?0:p->animation)%8;break;
        case POSE_ATTACK:case POSE_LIGHT3:case POSE_HEAVY:key=RIDER_PUNCHA+(p->animation/2>5?5u:(unsigned)(p->animation<0?0:p->animation/2));break;
        case POSE_LIGHT2:case POSE_FINISHER:case POSE_HEAVY2:key=RIDER_PUNCHB+(p->animation/2>5?5u:(unsigned)(p->animation<0?0:p->animation/2));break;
        case POSE_BOW:key=RIDER_BOW+(p->animation/4>4?4u:(unsigned)(p->animation<0?0:p->animation/4));break;
        case POSE_JUMP:case POSE_AIRATTACK:case POSE_AIRATTACK2:key=RIDER_JUMP+(p->animation/3>5?5u:(unsigned)(p->animation<0?0:p->animation/3));break;
        default:mapped=0;
        }
        if(mapped){
            unsigned id=costume*RIDER_FRAMES_PER_COSTUME+key;const ArtFrame *rf=&rider_frames[id];
            int rx=p->x+(p->facing<0?-(rf->x+rf->w-1):rf->x),ry=p->y-p->z-22+rf->y-bob;
            if(rf->w&&sprite_visible(rx,ry,rf->w,rf->h)&&rider_decode(id,world_foreground))raster_sprite(&view,world_foreground,rider_palette,rx,ry,rf->w,rf->h,p->facing<0,16,rf->h,p->y);
            return;
        }
    }
    if(pose>=POSE_HEAVY3&&!enemy){
        int dash=pose==POSE_DASHPUNCH||pose==POSE_DASHSLAM;
        unsigned sample=dash?dash_sample(p):combo_sample(p),color=gray?0:p->knight_id;
        unsigned hs=index<2?(unsigned)index:2u;
        ArtFrame a=dash?weapon_art_dash_frame(hs,sample,color,weapon):weapon_art_combo_frame(hs,sample,color,weapon);
        int left=p->x+(p->facing<0?-(a.x+a.w-1):a.x);
        if(!sprite_visible(left,p->y-p->z-lift+a.y,a.w,a.h))return;
        uint8_t *out=world_foreground;
        int id=(dash?3000:2000)+(int)(color*10+sample);
        if((index<2||index==8)&&(unsigned)a.w*a.h<=KNIGHT_WEAPON_FRAME_PIXELS_MAX){
            out=hero_cache[index==8?2:index];
            if(knight_cache_id[index]!=id||knight_cache_weapon[index]!=(int)weapon){
                sprite_decodes++;
                if(!(dash?weapon_art_dash_decode(hs,sample,color,weapon,out):weapon_art_combo_decode(hs,sample,color,weapon,out)))return;
                knight_cache_id[index]=id;knight_cache_weapon[index]=(int)weapon;
            }
        }else {sprite_decodes++;if(!(dash?weapon_art_dash_decode(hs,sample,color,weapon,out):weapon_art_combo_decode(hs,sample,color,weapon,out)))return;}
        raster_sprite(&view,out,gray?gray_palette:knight_palette,left,p->y-p->z-lift+a.y,a.w,a.h,p->facing<0,16,a.h,p->y);
        return;
    }
    /* Barbarians retain their own existing slam artwork. */
    if(pose==POSE_HEAVY3)pose=POSE_AIRHEAVY;else if(pose==POSE_STOMP)pose=POSE_HEAVY2;else if(pose==POSE_DASHPUNCH)pose=POSE_ROLL;else if(pose==POSE_DASHSLAM)pose=POSE_AIRHEAVY;
    int frame=knight_frame(p,pose);
    int id=(enemy||gray?0:p->knight_id*KNIGHT_FRAMES_PER_COLOR)+pose_start[pose]+frame;
    if(species&&species!=SPECIES_TROLL_MOTHER&&pose==POSE_DEAD){
        /* Troll and imp deaths play their whole source clip, twelve stages held for four ticks each, stored in the cast pose slot (tools/species_art.py). */
        int k=(p->animation<0?0:p->animation>43?43:p->animation)/4;
        id=pose_start[POSE_CAST]+k;
    }
    ArtFrame equipped=enemy?(species?*species_frame(species,(unsigned)id):thief?thief_frames[id]:barbarian_frames[id]):weapon_art_frame(index<2?(unsigned)index:2u,(unsigned)id,weapon);
    const ArtFrame *a=&equipped;
    int left=p->x+(p->facing<0?-(a->x+a->w-1):a->x);
    if(!sprite_visible(left,p->y-p->z-lift+a->y,a->w,a->h))return;
    const uint8_t *decoded;
    if(gray&&index!=8){if(!knight_decode_frame((unsigned)id,world_foreground)||!weapon_art_apply(2,(unsigned)id,0,world_foreground))return;decoded=world_foreground;}
    else decoded=frame_pixels(id,index,weapon);
    raster_sprite(&view,decoded,enemy?(species?species_palette(species):thief?thief_palette:barbarian_palette):gray?gray_palette:knight_palette,left,p->y-p->z-lift+a->y,
                  a->w,a->h,p->facing<0,16,a->h,p->y);
    (void)pixels;
}
static void foreground(uint16_t *pixels,const Arena *arena){
    const Scene *scene=scene_for(arena->map_id);
    if(scene->foreground_index<0)return;
    const ArenaSceneryLayer *layer=&scene->layers[scene->foreground_index];
    /* Shorten the parapets by 20%, preserving their base and horizontal spacing. */
    int height=layer->h*4/5,top=layer->y+layer->h-height;
    int overlap=0;
    for(int player=0;player<2&&!overlap;player++){
        const Fighter *p=&arena->p[player];if(!p->ready)continue;
        int pose=p->pose>=0&&p->pose<POSE_COUNT?p->pose:POSE_IDLE;
        ArtFrame bounds;
        if(pose==POSE_DASHPUNCH||pose==POSE_DASHSLAM)bounds=weapon_art_dash_frame(2,dash_sample(p),p->knight_id,0);
        else if(pose>=POSE_HEAVY3)bounds=weapon_art_combo_frame(2,combo_sample(p),p->knight_id,0);
        else bounds=knight_frames[p->knight_id*KNIGHT_FRAMES_PER_COLOR+pose_start[pose]+knight_frame(p,pose)];
        const ArtFrame *art=&bounds;
        int left=p->x+(p->facing<0?-(art->x+art->w-1):art->x)-4;
        int right=left+art->w+8,upper=p->y-p->z+art->y-4,lower=upper+art->h+8;
        /* Test opaque scenery within a padded sprite box, including airborne/dead poses. */
        int y0=upper-top;if(y0<0)y0=0;
        int y1=lower-top+1;if(y1>height)y1=height;
        int x0=left-layer->x;if(x0<0)x0=0;
        int x1=right-layer->x+1;if(x1>layer->w)x1=layer->w;
        for(int y=y0;y<y1&&!overlap;y++){
            int sy=y*layer->h/height;
            for(int x=x0;x<x1;x++)if(scene->layer_pixels[layer->offset+sy*layer->w+x]){overlap=1;break;}
        }
    }
    int target=overlap?5:16;
    if(!foreground_ready||foreground_round!=arena->round)foreground_alpha=target;
    else if(foreground_alpha>target){foreground_alpha-=4;if(foreground_alpha<target)foreground_alpha=target;}
    else if(foreground_alpha<target)foreground_alpha++;
    foreground_ready=1;foreground_round=arena->round;
    raster_sprite(&view,scene->layer_pixels+layer->offset,scene->palette,
                  layer->x,top,layer->w,layer->h,0,foreground_alpha,height,32767);
    (void)pixels;
}
/* Screen-space feedback stays readable while the world camera scrolls. */
static void feedback_sprite(uint16_t *pixels,const uint8_t *data,const uint16_t *palette,int x,int y,int w,int h,int shrink){
    for(int yy=0;yy<h;yy+=shrink)for(int xx=0;xx<w;xx+=shrink){
        int sx=x+xx/shrink,sy=y+yy/shrink;unsigned c=data[yy*w+xx];
        if(!c||sx<0||sx>=256||sy<0||sy>=192)continue;
        pixels[sy*256+sx]=palette[c];
        if(view.dirty_left[sy]>sx)view.dirty_left[sy]=sx;
        if(view.dirty_right[sy]<sx+1)view.dirty_right[sy]=sx+1;
    }
}
static void campaign_feedback(const Arena *a,uint16_t *pixels,const Scene *scene){
    if(!campaign_state||!a->running||a->winner)return;
    if(campaign_state->phase==KEEP_TRAVEL||campaign_state->phase==KEEP_STORE){
        const FeedbackFrame *f=&feedback_frames[ONWARD_FRAME_START+(a->tick/2)%ONWARD_FRAME_COUNT];
        feedback_sprite(pixels,feedback_pixels+f->offset,feedback_palette,222+f->x,39+f->y,f->w,f->h,ONWARD_RENDER_SHRINK);
    }
    for(int i=0;i<2;i++){
        unsigned timer=campaign_state->pickup_timer[i];if(!timer||!a->p[i].ready)continue;
        unsigned age=30-timer,kind=campaign_state->pickup_kind[i];const Fighter *p=&a->p[i];
        int x=(p->x-camera.x)*256/camera.width,y=(p->y-p->z-24-camera.y)*192/camera.height;
        if(age<8){
            const FeedbackFrame *f=&feedback_frames[PICKUP_FRAME_START+age/2];
            feedback_sprite(pixels,feedback_pixels+f->offset,feedback_palette,x+f->x,y+f->y,f->w,f->h,1);
        }
        if(kind==DROP_WEAPON){
            {const WeaponIcon *icon=weapon_icon(campaign_state->pickup_value[i]);if(icon)feedback_sprite(pixels,icon->pixels,icon->palette,x-16,y-42,32,32,1);}
        }else if(kind&&(kind<=DROP_BAG||kind==DROP_GEM||kind==DROP_POTION)){
            /* Source fly-to-HUD feedback crosses toward the lower DS screen. */
            if(age<20){
                const ArtFrame *f=&loot_frames[kind==DROP_POTION?LOOT_FRAME_POTION:kind==DROP_GEM?LOOT_FRAME_GEM:kind-1];int dest=i?200:64;
                int fx=x+(dest-x)*(int)age/20,fy=y+(196-y)*(int)(age*age)/400-(int)(age*(20-age))/12;
                feedback_sprite(pixels,loot_pixels+f->offset,loot_palette,fx+f->x,fy+f->y,f->w,f->h,1);
            }
        }else{
            char amount[12];snprintf(amount,sizeof amount,"+%u",campaign_state->pickup_value[i]);
            int tx=x-menu_text_width(amount)/2,ty=y-30-(int)age/2;
            int w=menu_text_width(amount);
            for(int yy=ty;yy<ty+15;yy++)if(yy>=0&&yy<192)for(int xx=tx;xx<tx+w;xx++)if(xx>=0&&xx<256){
                if(!(pixels[yy*256+xx]&0x8000))pixels[yy*256+xx]=scene_color(scene,view.x[xx],view.y[yy]);
                if(view.dirty_left[yy]>xx)view.dirty_left[yy]=xx;
                if(view.dirty_right[yy]<xx+1)view.dirty_right[yy]=xx+1;
            }
            menu_text(pixels,tx,ty,amount,RGB(10,31,10));
        }
    }
}
/* The doorway is an opening in front of a wall, not a decal on the floor.
   Restore the genuine jamb/lintel only after actors entering behind it; actors
   further forward are drawn later by the existing foot-depth ordering. */
static int foreground_door,foreground_rail;
static int barracks_foreground_reject(int x,int y){
    return (foreground_rail&&barracks_rail_pixel(x,y))||(foreground_door&&barracks_nearwall_pixel(x,y));
}
static void barracks_foreground_actor(const Fighter *p){
    foreground_door=barracks_nearwall_behind(p->x,p->y);
    foreground_rail=barracks_rail_behind(p->x,p->y);
    view.foreground_reject=(foreground_door||foreground_rail)?barracks_foreground_reject:NULL;
}
static void running_dust(const Arena *a,const Fighter *p){
    if(!arena_fighter_running(p))return;
    unsigned age=a->tick%18;if(age>=12)return;
    unsigned frame=age/4;
    const RunDustFrame *f=&run_dust_frames[frame];
    int dx=!!(p->previous&RIGHT)-!!(p->previous&LEFT),dy=!!(p->previous&DOWN)-!!(p->previous&UP);
    int x=p->x-dx*(int)age/3,y=p->y-dy*(int)age/3;
    int w=(f->w+1)/2,h=(f->h+1)/2;
    int left=dx?(x+(dx<0?-(f->x/2+w-1):f->x/2)):x-w/2;
    if(run_dust_decode(frame,world_foreground)){
        /* Small intermittent foot puffs, without doubling the source drift. */
        for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)world_foreground[yy*w+xx]=world_foreground[yy*2*f->w+xx*2];
        raster_sprite(&view,world_foreground,run_dust_palette,left,y+f->y/2,w,h,dx<0,16,h,p->y);
    }
}
/* Original movie key poses, with continuous motion between authored positions. */
static unsigned barracks_wounded(unsigned tick,int *x,int *y){
    unsigned elapsed=tick>BARRACKS_WOUNDED_SOURCE_FIRST?tick-BARRACKS_WOUNDED_SOURCE_FIRST:0;
    unsigned k=elapsed/BARRACKS_WOUNDED_SOURCE_STEP;
    if(k>=BARRACKS_WOUNDED_COUNT)k=BARRACKS_WOUNDED_COUNT-1;
    const BarracksActorFrame *f=&barracks_actor_frames[BARRACKS_WOUNDED_FIRST+k];
    *x=f->x+f->w/2;*y=f->y+f->h;
    if(k+1<BARRACKS_WOUNDED_COUNT){
        const BarracksActorFrame *next=f+1;
        unsigned fraction=elapsed%BARRACKS_WOUNDED_SOURCE_STEP;
        *x+=(next->x+next->w/2-*x)*(int)fraction/BARRACKS_WOUNDED_SOURCE_STEP;
        *y+=(next->y+next->h-*y)*(int)fraction/BARRACKS_WOUNDED_SOURCE_STEP;
    }
    return BARRACKS_WOUNDED_FIRST+k;
}

/* Thieves Forest cutscene clips: the owl and deer sit at their first frame until the heroes reach them, play while the clip runs
   and rest on their last frame afterwards; the intro carriers only exist while it plays. Frames are keyed by source frame. */
static void thieves_scene_draw(const Arena *a,const Campaign *c,int low,int high){
    int playing=thieves_scene_playing(c);int reach=0;
    for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].x>reach)reach=a->p[h].x;
    for(unsigned s=0;s<THIEVES_SCENE_COUNT;s++){
        int art=thieves_scenes[s].art;if(art<0)continue;
        const ThievesSceneClip *clip=&thieves_scene_clips[art];unsigned frame;
        if(playing==(int)s)frame=c->timer;
        else if(s==THIEVES_SCENE_INTRO)continue;
        else if(reach>=campaign_thieves_waypoint_x((unsigned)thieves_scenes[s].wp))frame=clip->length-1;
        else frame=0;
        if(clip->anchor_y<low||clip->anchor_y>=high)continue; /* Drawn once, in the depth band of its anchor. */
        unsigned id=0,best=0;
        for(unsigned k=0;k<clip->count;k++){const ThievesSceneKey *key=&thieves_scene_keys[clip->first+k];if(key->source_frame<=frame){id=key->frame;best=k;}}
        (void)best;
        const ArtFrame *f=&thieves_scene_frames[id];
        if(!f->w||!sprite_visible(clip->anchor_x+f->x,clip->anchor_y+f->y,f->w,f->h))continue;
        if(!thieves_scene_decode(id,world_foreground))continue;
        raster_sprite(&view,world_foreground,thieves_scene_palette,clip->anchor_x+f->x,clip->anchor_y+f->y,f->w,f->h,0,16,f->h,clip->anchor_y);
    }
}
/* MAIN f_ScoreTally: after a level, a wooden sign drops in beside each hero's side of the screen with a skull, a gold bag and XP; the
   numbers count up one after the other (kills from clip frame 17, gold from 25, experience from 33). */
static int tally_on;static uint32_t tally_base;
static void tally_draw(uint16_t *pixels,const Arena *a,const Campaign *c){
    if(!tally_on){tally_on=1;tally_base=a->tick;}
    unsigned t=a->tick>=tally_base?a->tick-tally_base:0;
    uint8_t *buf=world_foreground;if(!tally_decode(buf))return;
    for(unsigned h=0;h<2;h++){
        if(!a->p[h].ready)continue;
        int x=h?180:20,idx=t<28?(int)t/2:14,top=10+tally_drop[idx]; /* the sign drops at half the source speed */
        for(int yy=0;yy<TALLY_H;yy++){ /* Screen space, not world space: the sign is part of the HUD. */
            int py=top+yy;if(py<0||py>=192)continue;
            for(int xx=0;xx<TALLY_W;xx++){unsigned ci=buf[yy*TALLY_W+xx];if(ci)pixels[py*256+x+xx]=tally_palette[ci];}
            if(view.dirty_left[py]>x){view.dirty_left[py]=(uint16_t)x;}
            if(view.dirty_right[py]<x+TALLY_W){view.dirty_right[py]=(uint16_t)(x+TALLY_W);}
        }
        if(t<30)continue;
        unsigned v[3];campaign_tally_values(c,h,v);
        static const int rowy[3]={29,79,132};static const unsigned start[3]={30,66,102}; /* one row at a time, with time to read each */
        for(unsigned k=0;k<3;k++){
            if(t<start[k])continue;
            unsigned age=t-start[k],step=v[k]/40+1,shown=age*step/2<v[k]?age*step/2:v[k]; /* counts up at a third of the speed it did before */
            char text[12];snprintf(text,sizeof text,"%u",shown);
            int w=menu_text_width(text),tx=h?x-w-6:x+TALLY_W+6,ty=top+rowy[k]-7;
            if(ty<0||ty+16>192)continue;
            rect(pixels,tx-2,ty-1,w+4,16,RGB(3,4,6));
            for(int row=ty-1;row<ty+15;row++){if(view.dirty_left[row]>tx-2)view.dirty_left[row]=(uint16_t)(tx-2);if(view.dirty_right[row]<tx+w+2)view.dirty_right[row]=(uint16_t)(tx+w+2);}
            menu_text(pixels,tx,ty,text,RGB(31,29,20));
        }
    }
}
void arena_render(const Arena *a,uint16_t *pixels){
    render_map=a->map_id;sprite_tick=a->tick;worldmap_cached=0;lobby_loaded=0;
    cinematic_reset();
    const Scene *scene=scene_for(a->map_id);
    Arena framing=*a;
    if(campaign_enemy_count&&a->map_id<CAMPAIGN_MAP){
        const Fighter *actors[8]={&a->p[0],&a->p[1],&campaign_enemy[0],NULL,NULL,NULL,NULL,NULL};
        for(int i=1;i<campaign_enemy_count;i++)actors[i+2]=&campaign_enemy[i];
        int left=a->p[0].x,right=left,top=a->p[0].y-a->p[0].z,bottom=a->p[0].y;
        for(int i=0;i<2+campaign_enemy_count;i++)if(actors[i]->ready){
            if(actors[i]->x<left)left=actors[i]->x;
            if(actors[i]->x>right)right=actors[i]->x;
            if(actors[i]->y-actors[i]->z<top)top=actors[i]->y-actors[i]->z;
            if(actors[i]->y>bottom)bottom=actors[i]->y;
        }
        framing.p[0].x=left;framing.p[0].y=top;framing.p[0].z=0;
        framing.p[1].x=right;framing.p[1].y=bottom;framing.p[1].z=0;framing.p[1].ready=1;
    }
    if(a->map_id==BOSS_MAP&&campaign_state&&campaign_state->war.machine.ready&&campaign_state->phase!=BOSS_INTRO){
        const Fighter *m=&campaign_state->war.machine;
        int left=a->p[0].x,right=left,top=a->p[0].y-a->p[0].z,bottom=a->p[0].y;
        for(int h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp){if(a->p[h].x<left)left=a->p[h].x;if(a->p[h].x>right)right=a->p[h].x;if(a->p[h].y-a->p[h].z<top)top=a->p[h].y-a->p[h].z;if(a->p[h].y>bottom)bottom=a->p[h].y;}
        /* Focus on the nearest boss edge when party and giant span the arena. */
        int focus=m->x;if(focus>right+220)focus=right+220;if(focus<left-220)focus=left-220;
        if(focus-60<left)left=focus-60;
        if(focus+60>right)right=focus+60;
        if(m->y-100<top)top=m->y-100;
        if(m->y>bottom)bottom=m->y;
        framing.p[0].x=left;framing.p[0].y=top;framing.p[0].z=0;framing.p[0].hp=1;framing.p[0].ready=1;
        framing.p[1].x=right;framing.p[1].y=bottom;framing.p[1].z=0;framing.p[1].hp=1;framing.p[1].ready=1;
    }
    if(a->map_id==BOSS_MAP&&campaign_state&&campaign_state->phase==BOSS_INTRO&&campaign_state->timer<299){
        /* The opening party stands to the left of the three carried captives
           and fourth princess's post. Frame the scene before the boss arrives,
           then the party and the boss walking in from the gate until the fight is released at frame 299. */
        int hero_x=a->p[0].ready?a->p[0].x:a->p[1].x;if(a->p[0].ready&&a->p[1].ready&&a->p[1].x<hero_x)hero_x=a->p[1].x;
        if(campaign_state->timer>=35&&campaign_state->timer<179){if(hero_x<BOSS_DOOR_X-330)hero_x=BOSS_DOOR_X-330;} /* The camera looks at the gate while the little barbarians fall and the door comes down. */
        if(campaign_state->timer>=179){int boss_left=campaign_state->war.machine.x-300;if(hero_x<boss_left)hero_x=boss_left;} /* Then it follows the boss in from the gate. */
        framing.p[0].x=hero_x;framing.p[0].y=campaign_state->timer<179?124:316;framing.p[0].z=0;
        framing.p[1].x=campaign_state->timer<35?519:campaign_state->timer<179?BOSS_DOOR_X+40:campaign_state->war.machine.x+40;framing.p[1].y=336;framing.p[1].z=0;
        framing.p[0].hp=framing.p[1].hp=1;framing.p[0].ready=framing.p[1].ready=1;
    }
    static ArenaCamera pan_origin,pan_return;static int previous_phase=-1;
    int phase=campaign_state?campaign_state->phase:-1;
    opaque_scene=(a->map_id==5&&phase==KEEP_BARRACKS_INTRO)||(a->map_id>=6&&(phase==WAR_HORSE_CINEMA||phase==WAR_MACHINE_INTRO||phase==WAR_BRIDGE_CINEMA));
    if(campaign_state&&a->map_id==5&&phase==KEEP_BARRACKS_INTRO){
        /* Original movie366 guide positions, including its return to the party. */
        unsigned t=campaign_state->timer,k=t<425?t:424;
        ArenaCamera party={0};camera_update(&party,&framing);
        ArenaCamera target={.x=barracks_camera_guide[k][0]-212,.y=barracks_camera_guide[k][1]+50-318,.width=424,.height=318,.ready=1,.round=a->round};
        if(target.x<0)target.x=0;
        if(target.x>1280-424)target.x=1280-424;
        if(target.y<0)target.y=0;
        if(target.y>448-318)target.y=448-318;
        int blend=t<=425?256:t<486?(int)(486-t)*256/61:0;
        camera=target;camera.x=party.x+(target.x-party.x)*blend/256;camera.y=party.y+(target.y-party.y)*blend/256;
        camera.width=party.width+(target.width-party.width)*blend/256;camera.height=camera.width*3/4;
    }else if(campaign_state&&(phase==KEEP_PAN_IN||phase==KEEP_PAN_OUT)){
        ArenaCamera target={.width=424,.height=318,.ready=1,.round=a->round};
        cinematic_camera(CINEMATIC_PRINCESSES,phase==KEEP_PAN_IN?0:cinematic_duration(CINEMATIC_PRINCESSES)-1,&target.x,&target.y);
        if(previous_phase!=phase){
            pan_origin=camera;
            if(phase==KEEP_PAN_IN){
                if(!pan_origin.ready||pan_origin.width<=0)camera_update(&pan_origin,&framing);
                pan_return=pan_origin;
            }else{
                /* Radio snapshots may skip the short pan-in entirely. */
                if(previous_phase!=KEEP_PAN_IN||!pan_return.ready||pan_return.width<=0){pan_return=(ArenaCamera){0};camera_update(&pan_return,&framing);}
                pan_origin=target;target=pan_return;
            }
        }
        if(phase==KEEP_PAN_OUT)target=pan_return;
        unsigned t=campaign_state->timer;if(t>18)t=18;
        camera=target;camera.x=pan_origin.x+(target.x-pan_origin.x)*(int)t/18;camera.y=pan_origin.y+(target.y-pan_origin.y)*(int)t/18;
        camera.width=pan_origin.width+(target.width-pan_origin.width)*(int)t/18;camera.height=camera.width*3/4;
    }else{
        if(campaign_state&&a->map_id==THIEVES_MAP)stage_shake=(int)thieves_scene_shake(campaign_state);
        if(campaign_state&&a->map_id==DEER_MAP)stage_shake=(int)campaign_deer_shake(campaign_state);
        camera_update(&camera,&framing);
    }
    if(a->map_id>=6&&campaign_state&&(phase==WAR_MACHINE_INTRO||phase==WAR_BRIDGE_CINEMA)){
        static ArenaCamera war_origin;
        if(previous_phase!=phase)war_origin=camera;
        unsigned timer=campaign_state->timer;
        if(phase==WAR_MACHINE_INTRO||timer<42){
            WarStoryCamera guide=phase==WAR_MACHINE_INTRO?war_machine_story_camera(timer):war_bridge_story_camera(timer);
            camera.width=WORLD_WIDTH;camera.height=WORLD_HEIGHT;
            camera.x=guide.relative?war_origin.x+guide.x:guide.x-camera.width/2;
            camera.y=guide.relative?war_origin.y+guide.y:guide.y-camera.height/2;
            if(camera.x<0)camera.x=0;
            if(camera.x>stage_world_width(a->map_id)-camera.width)camera.x=stage_world_width(a->map_id)-camera.width;
            if(camera.y<0)camera.y=0;
            if(camera.y>448-camera.height)camera.y=448-camera.height;
        }
    }
    previous_phase=phase;
    /* BG3 uses one 8.8 affine step for both axes. The software height must
       describe exactly that same step, including intermediate zoom frames. */
    camera.width=(camera.width+2)&~3;camera.height=camera.width*3/4;
    camera_clamp_world(&camera,a);
    if(campaign_state&&a->map_id==WAR_MAP){
        if(phase==WAR_FIGHT&&campaign_state->group<WAR_WAVE_COUNT)
            camera_limit_horizontal(&camera,a,0,war_waves[campaign_state->group].right+32);
        else if(phase==WAR_MACHINE_FIGHT)
            camera_limit_horizontal(&camera,a,WAR_MACHINE_LEASH_LEFT-32,WAR_MACHINE_LEASH_RIGHT+32);
    }
    if(campaign_state&&a->map_id==THIEVES_MAP&&phase==WAR_FIGHT){
        int left,right;campaign_thieves_leash(campaign_state,&left,&right);camera_limit_horizontal(&camera,a,left-camera.width/2,right+camera.width/2); /* The leash marks where the heroes may stand; the screen centres on it, as when the encounter begins. */
    }
    if(campaign_state&&a->map_id==DEER_MAP){int cx=campaign_deer_camera_x(campaign_state);camera_limit_horizontal(&camera,a,cx-camera.width/2,cx+camera.width/2);} /* The camera rides the chase guide, not the heroes. */
    if(campaign_state&&a->map_id==FOREST_MAP){
        int left,right;campaign_forest_leash(campaign_state,&left,&right);
        if(phase==WAR_FOREST_CINEMA){
            /* Source cinema_cam: the road between the heroes' marks and the carried princesses. */
            camera.width=WORLD_WIDTH;camera.height=camera.width*3/4;camera.x=720-camera.width/2;camera.y=200-camera.height/2;
            if(camera.y<0)camera.y=0;
            if(camera.y>448-camera.height)camera.y=448-camera.height;
            camera_clamp_world(&camera,a);
        }else if(phase==WAR_FIGHT)camera_limit_horizontal(&camera,a,left-camera.width/2,right+camera.width/2);
    }
    if(scene==&campaign_scene)world_stream(&camera);else background_camera=camera;
#if defined(ARM9) || defined(RENDER_TEST_OVERLAY)
    if(view.pixels!=pixels)memset(pixels,0,256*192*sizeof(*pixels));
    else for(int y=0;y<192;y++)if(view.dirty_right[y]>view.dirty_left[y])
        memset(pixels+y*256+view.dirty_left[y],0,(view.dirty_right[y]-view.dirty_left[y])*sizeof(*pixels));
#endif
    raster_init(&view,pixels,&camera);
    for(int i=0;i<scene->prop_count;i++)layer_depth[i]=scene->layers[i].depth_y;
    view.background_sample=scene==&campaign_scene?world_color:arena_surface_color;
    view.scene=scene->pixels;view.scene_palette=scene->palette;
    view.occlusion=scene->occlusion;view.layer_depth=layer_depth;
#if defined(ARM9) || defined(RENDER_TEST_OVERLAY)
    /* Transparent overlay: DS BG3 hardware supplies the static scenery. */
    /* Only previous overlay spans were cleared above; untouched pixels remain transparent. */
    if(opaque_scene)campaign_background_draw();
#else
    if(opaque_scene)campaign_background_draw();
    else
    for(int y=0;y<192;y++){
        for(int x=0;x<256;x++)pixels[y*256+x]=scene_color(scene,view.x[x],view.y[y]);
    }
#endif
    Fighter neutral[3]={{0}},empty={0},cart={0},barracks[BARRACKS_ACTOR_COUNT+10]={{0}};
    unsigned barracks_frames[BARRACKS_ACTOR_COUNT+10]={0};
    const Fighter *actors[23+BARRACKS_ACTOR_COUNT]={&a->p[0],&a->p[1],&empty,&empty,&empty,&empty,&empty,&empty,&empty,&neutral[0],&neutral[1],&neutral[2],&cart};
    for(int i=0;i<campaign_enemy_count;i++)actors[i+2]=&campaign_enemy[i];
    int order[23+BARRACKS_ACTOR_COUNT],actor_count=campaign_state?13:2+campaign_enemy_count;
    for(unsigned i=0;i<23+BARRACKS_ACTOR_COUNT;i++)order[i]=(int)i;
    if(campaign_state&&a->map_id==5){
        actor_count=23+BARRACKS_ACTOR_COUNT;
        for(unsigned i=0;i<BARRACKS_ACTOR_COUNT;i++){
            const BarracksActor *b=&barracks_actors[i];
            unsigned frame=b->first;
            if(b->barrel){
                unsigned age=(uint16_t)(a->tick-campaign_state->break_tick);
                if(campaign_state->broken&(1u<<b->barrel))frame+=((campaign_state->broken>>8)&7)==b->barrel&&age<24?3:2;
            }else if(campaign_state->phase==KEEP_BARRACKS_INTRO&&campaign_state->timer<161)frame+=(a->tick/8)%b->count;
            else if(b->first==BARRACKS_DRINKER_FIRST)frame+=(a->tick/12)%b->count;
            barracks_frames[i]=frame;
            barracks[i]=(Fighter){.x=b->x,.y=b->y,.ready=1};
            if(b->first==BARRACKS_DRINKER_FIRST&&!(campaign_state->phase==KEEP_BARRACKS_INTRO&&campaign_state->timer<425))barracks[i].ready=0;
            actors[13+i]=&barracks[i];
        }
        int wx,wy;unsigned frame=barracks_wounded(campaign_state->timer,&wx,&wy);
        barracks_frames[BARRACKS_ACTOR_COUNT]=frame;
        barracks[BARRACKS_ACTOR_COUNT]=(Fighter){.x=wx,.y=wy,.ready=campaign_state->phase==KEEP_BARRACKS_INTRO&&campaign_state->timer>=161&&campaign_state->timer<391};
        actors[13+BARRACKS_ACTOR_COUNT]=&barracks[BARRACKS_ACTOR_COUNT];
        /* At source425, drunk placeholders are replaced by movie223's eight walkers. */
        unsigned age=a->tick>=425?a->tick-425+1:167;
        for(unsigned n=0;n<8;n++){
            unsigned i=BARRACKS_ACTOR_COUNT+1+n;
            if(age<167&&(campaign_state->phase==KEEP_BARRACKS_INTRO||(campaign_state->story_flags&64))){
                int x=barracks_exit_positions[age][n][0],y=barracks_exit_positions[age][n][1];
                barracks[i]=(Fighter){.x=x,.y=y,.ready=x!=-32768};barracks_frames[i]=(age-1)%16;
            }
            actors[13+i]=&barracks[i];
        }
        unsigned door=BARRACKS_ACTOR_COUNT+9;
        unsigned tick=campaign_state->phase==KEEP_BARRACKS_INTRO?campaign_state->timer:193;
        barracks_frames[door]=tick<161?0:tick<193?1+(tick-161)/2:16;
        barracks[door]=(Fighter){.x=1089,.y=175,.ready=tick<193};actors[13+door]=&barracks[door];
    }
    if(campaign_state){
        actors[8]=campaign_state->war.machine.ready?&campaign_state->war.machine:&campaign_state->ally;
        unsigned age=(uint16_t)(a->tick-campaign_state->break_tick);
        if(a->map_id==3&&(!(campaign_state->broken&1)||age<26))cart=(Fighter){.x=917,.y=333,.ready=1};
    }
    if(campaign_state&&a->map_id==3&&campaign_state->npc_ticks>0&&campaign_state->npc_ticks<140){
        for(int i=0;i<3;i++){
            const KeepRunnerSource *source=&keep_runner_sources[i];
            neutral[i]=(Fighter){.x=(source->x_twips/20+200)/2-campaign_state->npc_ticks*source->speed/2,.y=(source->y_twips/20+810)/2,.facing=-1,.pose=POSE_WALK,.animation=campaign_state->npc_ticks,.ready=1};
            if(neutral[i].x<0)neutral[i].ready=0;
        }
    }
    if(campaign_state&&a->map_id==4&&campaign_state->phase==KEEP_STORE){
        _Static_assert(SHOP_BOARD_MAX_PIXELS<=BARBARIAN_FRAME_PIXELS_MAX,"Shop cache capacity");
        for(unsigned item=0;item<2;item++){
            unsigned state=0;
            for(unsigned h=0;h<2;h++)if(a->p[h].ready&&a->p[h].hp>0){
                const Fighter *p=&a->p[h];
                int nearby=p->x>=609&&p->x<=725&&p->y>=236&&p->y<=276&&((p->x>=669)==(int)item);
                if(nearby){
                    int full=item?weapon_set_has(&campaign_state->owned_weapons[h],WEAPON_SKINNY):campaign_state->potions[h]>=5;
                    state=full?2:campaign_state->profiles[h].gold<(item?32:12)?3:1;
                }
                if(campaign_state->pickup_timer[h]>15&&campaign_state->pickup_kind[h]==(item?DROP_WEAPON:DROP_POTION))state=4;
            }
            unsigned frame=item*5+state;const ShopBoardFrame *f=&shop_board_frames[frame];
            int x=(item?675:617)+f->x,y=(item?235:236)+f->y;
            if(!sprite_visible(x,y,f->w,f->h))continue;
            int key=-100-(int)frame,shared=!campaign_state->enemies[4+item].ready;
            uint8_t *pic=shared?enemy_cache[4+item]:world_foreground;
            if(!shared||knight_cache_id[6+item]!=key){
                if(!shop_board_decode(frame,pic))continue;
                sprite_decodes++;
                if(shared)knight_cache_id[6+item]=key;
            }
            raster_sprite(&view,pic,shop_board_palette,x,y,f->w,f->h,0,16,f->h,236);
        }
    }
    for(int i=0;i<actor_count;i++){
        if(!actors[i]->ready||i==22+BARRACKS_ACTOR_COUNT)continue;
        if(i>=2&&i<9&&!(i==8&&campaign_state&&campaign_state->war.machine.ready)&&!campaign_npc_visible(actors[i]))continue;
        if(a->map_id==5&&i<2&&actors[i]->x>=1136&&actors[i]->y>=131&&actors[i]->y<=174)continue;
        if(i==13+BARRACKS_ACTOR_COUNT){const BarracksActorFrame *f=&barracks_actor_frames[barracks_frames[i-13]];if(f->w*f->h<=1)continue;}
        if(a->map_id==5)barracks_foreground_actor(actors[i]);
        if(i<13||barracks_frames[i-13]<BARRACKS_FURNITURE_FIRST)raster_rect(&view,actors[i]->x-9,actors[i]->y-1,18,3,RGB(8,12,12),-32768);
    }
    view.foreground_reject=NULL;
    for(int i=0;i<actor_count;i++)for(int j=i+1;j<actor_count;j++)if(actors[order[j]]->y<actors[order[i]]->y){int n=order[i];order[i]=order[j];order[j]=n;}
    int drawn=0,low=-32768;
    int layer_count=scene==&campaign_scene?(int)map_fg_count(a->map_id):scene->prop_count;
    if(campaign_state&&scene==&campaign_scene&&(a->map_id==THIEVES_MAP||a->map_id==FOREST_MAP))for(int layer=0;layer<layer_count;layer++){
        const CampaignRouteLayer *route=map_fg_layer_or_null(a->map_id,(unsigned)layer);if(!route)continue;
        int bi=campaign_break_find(a->map_id,route->source,route->depth,route->x+route->w/2);
        if(bi>=0&&campaign_break_cut(campaign_state,(unsigned)bi))break_cover(a,route,layer,bi,scene);
    }
    for(int layer=0;layer<=layer_count;layer++){
        int high=layer<layer_count?(scene==&campaign_scene?map_fg_depth(a->map_id,(unsigned)layer):scene->layers[layer].depth_y):32767;
        if(a->map_id==FOREST_MAP)forest_props_draw(low,high);
        if(campaign_state&&a->map_id==BOSS_MAP&&BOSS_DOOR_Y>=low&&BOSS_DOOR_Y<high)boss_door_draw(campaign_state);
        if(campaign_state&&a->map_id==BOSS_MAP){
            unsigned phase=campaign_state->phase,t=campaign_state->timer;
            if(phase==BOSS_INTRO&&185>=low&&185<high)boss_actor_picture(BOSS_ACTOR_CARRIED,t,437,185,0,185);
            if(phase<=BOSS_PRINCESS&&BOSS_PRINCESS_Y>=low&&BOSS_PRINCESS_Y<high)boss_actor_picture(BOSS_ACTOR_POST,0,BOSS_PRINCESS_X,BOSS_PRINCESS_Y,0,BOSS_PRINCESS_Y);
            if(phase==BOSS_PRINCESS&&BOSS_PRINCESS_Y>=low&&BOSS_PRINCESS_Y<high)scissors_draw(a->tick,BOSS_PRINCESS_X+28,BOSS_PRINCESS_Y-96,BOSS_PRINCESS_Y);
            if(phase==BOSS_RELEASE&&BOSS_PRINCESS_Y>=low&&BOSS_PRINCESS_Y<high)boss_actor_picture(BOSS_ACTOR_PRINCESS,t,BOSS_PRINCESS_X,BOSS_PRINCESS_Y,0,BOSS_PRINCESS_Y);
            if(phase==BOSS_DUEL&&BOSS_PRINCESS_Y>=low&&BOSS_PRINCESS_Y<high)boss_actor_picture(BOSS_ACTOR_PRINCESS,32,BOSS_PRINCESS_X,BOSS_PRINCESS_Y,0,BOSS_PRINCESS_Y);
            if(phase>=BOSS_KISS&&BOSS_PRINCESS_Y>=low&&BOSS_PRINCESS_Y<high)boss_actor_picture(t<65?BOSS_ACTOR_PRINCESS:BOSS_ACTOR_KISS,t<65?32:t-65,BOSS_PRINCESS_X,BOSS_PRINCESS_Y,0,BOSS_PRINCESS_Y);
            const Fighter *m=&campaign_state->war.machine;
            if((campaign_state->war.flags&BOSS_CHEST)&&m->y>=low&&m->y<high)boss_actor_picture(BOSS_ACTOR_CHEST_OPEN,phase==BOSS_TREASURE?t:320,m->x+m->facing*30,m->y,0,m->y);
            for(unsigned i=0;i<BOSS_GOLD_COUNT;i++){
                BossGold gold;if(!campaign_boss_gold(campaign_state,i,&gold)||gold.y<low||gold.y>=high)continue;
                const ArtFrame *f=&loot_frames[gold.kind==DROP_GEM?LOOT_FRAME_GEM:gold.kind-1];
                int height=campaign_boss_gold_height(&gold);
                raster_sprite(&view,loot_pixels+f->offset,loot_palette,gold.x+f->x,gold.y-height+f->y,f->w,f->h,0,16,f->h,gold.y);
            }
        }
        if(campaign_state&&a->map_id==THIEVES_MAP)for(unsigned i=0;i<THIEVES_BAG_COUNT;i++){
            int bx,by;
            if(!campaign_thieves_bag(campaign_state,i,&bx,&by)||by<low||by>=high)continue;
            const ArtFrame *f=&loot_frames[DROP_BAG-1];
            raster_rect(&view,bx-4,by-1,8,2,RGB(8,12,12),by);
            raster_sprite(&view,loot_pixels+f->offset,loot_palette,bx+f->x,by+f->y,f->w,f->h,0,16,f->h,by);
        }
        if(campaign_state)for(int d=0;d<CAMPAIGN_DROPS;d++){
            const CampaignDrop *item=&campaign_state->drops[d];if(item->y<low||item->y>=high||!item->kind||!campaign_drop_visible(campaign_state,(unsigned)d))continue;
            int height=campaign_drop_height(campaign_state,(unsigned)d);
            if(item->kind==DROP_WEAPON){
                {const WeaponIcon *icon=weapon_icon(item->value);if(icon)raster_sprite(&view,icon->pixels,icon->palette,item->x-16,item->y-28,32,32,0,16,32,item->y);}
                continue;
            }
            if(item->kind==DROP_BOW){
                /* Source item 11 shows the bow's item-select picture; it bounces in like other loot. */
                const SelectorFrame *bf=&selector_frames[ITEM_BOW-1];uint8_t bow[SELECTOR_MAX_PIXELS];
                if(selector_decode(ITEM_BOW-1,bow)){
                    if(height)raster_rect(&view,item->x-4,item->y-1,8,2,RGB(8,12,12),item->y);
                    raster_sprite(&view,bow,selector_palette,item->x-bf->w/2,item->y-height-bf->h,bf->w,bf->h,0,16,bf->h,item->y);
                }
                continue;
            }
            if(item->kind==DROP_CROC&&render_map!=FOREST_MAP)continue;
            if(item->kind==DROP_CROC){
                const ForestCrocFrame *cf=&forest_croc_frames[0];int flip=item->value&1,cx=item->x+(flip?-(cf->x+cf->w-1):cf->x),cy=item->y+cf->y;
                raster_rect(&view,item->x-24,item->y-2,48,4,RGB(8,12,12),item->y-1);
                if(sprite_visible(cx,cy,cf->w,cf->h)&&forest_croc_decode(0,world_foreground))raster_sprite(&view,world_foreground,forest_croc_palette,cx,cy,cf->w,cf->h,flip,16,cf->h,item->y);
                continue;
            }
            if(item->kind==DROP_PET){draw_pet(item->value&31,item->x,item->y,1,a->tick,0);continue;}
            const ArtFrame *f=&loot_frames[item->kind==DROP_FOOD?loot_food_frame(item->value):item->kind==DROP_GEM?LOOT_FRAME_GEM:item->kind-1];
            if(height)raster_rect(&view,item->x-4,item->y-1,8,2,RGB(8,12,12),item->y);
            raster_sprite(&view,loot_pixels+f->offset,loot_palette,item->x+f->x,item->y-height+f->y,f->w,f->h,0,16,f->h,item->y);
        }

        if(campaign_state&&a->map_id==HUB_FROG){
            static const int racks[4][2]={{0,0},{547,248},{971,249},{571,248}};
            for(unsigned slot=1;slot<4;slot++)if(racks[slot][1]>=low&&racks[slot][1]<high&&((a->p[0].ready&&weapon_set_has(&campaign_state->owned_weapons[0],slot))||(a->p[1].ready&&weapon_set_has(&campaign_state->owned_weapons[1],slot)))&&weapon_icon(slot))raster_sprite(&view,weapon_icon(slot)->pixels,weapon_icon(slot)->palette,racks[slot][0]-16,racks[slot][1]-28,32,32,0,16,32,racks[slot][1]);
        }
        if(campaign_state)for(unsigned h=0;h<2;h++)if(campaign_state->pets[h].equipped&&a->p[h].ready){
            const PetFollower *f=&campaign_state->pet_followers[h];
            if(f->y>=low&&f->y<high)draw_pet(campaign_state->pets[h].equipped,f->x,f->y,f->facing,a->tick,f->love_timer);
        }
        while(drawn<actor_count){
            int player=order[drawn];const Fighter *actor=actors[player];
            if(!actor->ready){drawn++;continue;}
            if(player>=2&&player<9&&!(player==8&&campaign_state&&campaign_state->war.machine.ready)&&!campaign_npc_visible(actor)){drawn++;continue;}
            if(actor->y>=high)break;
            if(a->map_id==BOSS_MAP&&campaign_state&&player<2&&player==(campaign_state->group&1)&&campaign_state->phase>=BOSS_KISS&&campaign_state->timer>=65){
                boss_actor_picture(13+actor->knight_id,64+(campaign_state->timer-65)%22,actor->x,actor->y-actor->z,actor->facing<0,actor->y);drawn++;continue;
            }
            if(a->map_id==5&&player<2&&actor->x>=1136&&actor->y>=131&&actor->y<=174){drawn++;continue;}
            if(a->map_id==5&&(player<2||player==13+BARRACKS_ACTOR_COUNT||(player>=14+BARRACKS_ACTOR_COUNT&&player<22+BARRACKS_ACTOR_COUNT))){
                barracks_foreground_actor(actor);
                if(player>=14+BARRACKS_ACTOR_COUNT&&player<22+BARRACKS_ACTOR_COUNT){
                    foreground_door=barracks_nearwall_departing(actor->x);
                    view.foreground_reject=(foreground_door||foreground_rail)?barracks_foreground_reject:NULL;
                }
            }
            if(player<2){running_dust(a,actor);effects_ready_view(&view,a,player);}
            if(player>=13){
                unsigned i=(unsigned)player-13,frame=barracks_frames[i];
                if(i==BARRACKS_ACTOR_COUNT+9){
                    const BarracksDoorFrame *f=&barracks_door_frames[frame];
                    if(f->x<camera.x+camera.width&&f->x+f->w>camera.x&&f->y<camera.y+camera.height&&f->y+f->h>camera.y&&barracks_door_decode(frame,world_foreground))raster_sprite(&view,world_foreground,barracks_door_palette,f->x,f->y,f->w,f->h,0,16,f->h,175);
                }else if(i>BARRACKS_ACTOR_COUNT){
                    const BarracksActorFrame *f=&barracks_exit_frames[frame];
                    int x=actor->x+f->x,y=actor->y+f->y;
                    if(x<camera.x+camera.width&&x+f->w>camera.x&&y<camera.y+camera.height&&y+f->h>camera.y&&barracks_exit_decode(frame,world_foreground))raster_sprite(&view,world_foreground,barracks_actor_palette,x,y,f->w,f->h,0,16,f->h,actor->y);
                }else if(frame>=BARRACKS_DRINKER_FIRST&&frame<BARRACKS_DRINKER_FIRST+BARRACKS_DRINKER_COUNT&&barracks_dancing()){
                    unsigned n=i-9u; /* the nine grey knights: four styles, each a few frames out of step with its neighbour */
                    dance_sprite(DANCE_GRAY,n&3u,n*5u,actor->x,actor->y,(int)(n&1u),actor->y);
                }else{
                const BarracksActorFrame *f=&barracks_actor_frames[frame];
                int absolute=i==BARRACKS_ACTOR_COUNT||frame>=BARRACKS_FURNITURE_FIRST;
                int x=absolute?f->x:actor->x+f->x,y=absolute?f->y:actor->y+f->y;
                if(i==BARRACKS_ACTOR_COUNT){x=actor->x-f->w/2;y=actor->y-f->h;}
                if(x<camera.x+camera.width&&x+f->w>camera.x&&y<camera.y+camera.height&&y+f->h>camera.y&&barracks_actor_decode(frame,world_foreground))raster_sprite(&view,world_foreground,barracks_actor_palette,x,y,f->w,f->h,0,16,f->h,actor->y);
                }
            }else if(player==12){
                unsigned age=(uint16_t)(a->tick-campaign_state->break_tick),frame=campaign_state->broken&1?2+age/2:campaign_state->break_tick&&age<4?1:0;
                if(!(campaign_state->broken&1)||age<14){
                    const CampaignBreakableFrame *f=&campaign_breakable_frames[frame];
                    if(campaign_breakable_decode(frame,world_foreground))raster_sprite(&view,world_foreground,campaign_breakable_palette,917+f->x,333+f->y,f->w,f->h,0,16,f->h,333);
                }
                if((campaign_state->broken&1)&&age<26)for(int piece=0;piece<5;piece++){
                    const CampaignBreakableFrame *f=&campaign_breakable_frames[9+piece];
                    int x=917+(piece-2)*(int)age/2,y=333-18-(int)(age*(26-age))/12;
                    if(campaign_breakable_decode(9+piece,world_foreground))raster_sprite(&view,world_foreground,campaign_breakable_palette,x+f->x,y+f->y,f->w,f->h,0,16,f->h,333);
                }
            }else if(player==8&&campaign_state&&campaign_state->war.machine.ready){if(a->map_id==BOSS_MAP)boss_actor_picture(actor->attack_kind,actor->animation,actor->x,actor->y,actor->facing<0,actor->y);else machine_actor(actor);}
            else knight(pixels,actor,player);
            view.foreground_reject=NULL;
            if(player>=2&&player<9)effects_status_view(&view,actor);
            drawn++;
        }
        if(a->map_id==THIEVES_MAP&&campaign_state)thieves_scene_draw(a,campaign_state,low,high);
        if(a->map_id==DEER_MAP&&campaign_state)deer_scene_draw(a,campaign_state,low,high);
        if(a->map_id==FLOOD_MAP&&campaign_state)flood_scene_draw(campaign_state,low,high);
        if(a->map_id==WAR_MAP){
            for(unsigned i=0;i<2;i++){
                const Projectile *p=&a->shots[i];if(!p->life||p->owner!=8||p->y<low||p->y>=high)continue;
                int age=48-p->life;
                if(age<40){
                    /* Aimed ball: the red X on the ground shows where it will come down; the ring closes in as it falls. */
                    int mx=p->vz,my=p->y,blink=(age>26&&(age&2));
                    if(!blink)for(int k=-9;k<=9;k++){raster_rect(&view,mx+k,my+k/2,2,2,RGB(31,3,3),my+1);raster_rect(&view,mx+k,my-k/2,2,2,RGB(31,3,3),my+1);}
                    int rx=ARENA_CANNON_RADIUS_X*(40-age)/40+6,ry=ARENA_CANNON_RADIUS_Y*(40-age)/40+3;
                    raster_rect(&view,mx-rx,my,2,1,RGB(26,6,5),my);raster_rect(&view,mx+rx-1,my,2,1,RGB(26,6,5),my);
                    raster_rect(&view,mx,my-ry,1,1,RGB(26,6,5),my);raster_rect(&view,mx,my+ry,1,1,RGB(26,6,5),my);
                    cannon_picture(0,p->x,p->y-p->z,p->vx<0,p->y);
                }else{
                    /* The blast: a bright burst with a ring that grows to the damage radius. */
                    int e=age-40;int r=ARENA_CANNON_RADIUS_X*(e+1)/8,h=ARENA_CANNON_RADIUS_Y*(e+1)/8;
                    uint16_t core=e<3?RGB(31,31,26):RGB(31,22,6),edge=RGB(29,10,3);
                    raster_rect(&view,p->x-r/2,p->y-h/2-4,r,h,core,p->y);
                    raster_rect(&view,p->x-r,p->y-h,2*r,1,edge,p->y);raster_rect(&view,p->x-r,p->y+h,2*r,1,edge,p->y);
                    raster_rect(&view,p->x-r,p->y-h,1,2*h,edge,p->y);raster_rect(&view,p->x+r-1,p->y-h,1,2*h,edge,p->y);
                    if(e<3)stage_shake=2;
                }
            }
            if(campaign_state&&phase==WAR_HORSE_CINEMA){
                unsigned tick=campaign_state->timer;int depth=a->p[0].y;
                if(depth>=low&&depth<high){
                    if(tick>=113&&tick<128){int age=tick-113,height=150-age*(age-1)*5/4;if(height<0)height=0;cannon_picture(1,a->p[0].x+(128-(int)tick)*10,depth-height,0,depth);}
                    if(tick>=128&&tick<148){unsigned frame=2+(tick-128)/4;cannon_picture(frame,a->p[0].x,depth,0,depth);cannon_picture(frame,a->p[0].x,depth,1,depth);}
                }
            }
        }
        effects_render_view(&view,a,low,high);low=high;
        if(scene==&campaign_scene&&layer<layer_count){
            CampaignWorldLayer converted;
            const CampaignRouteLayer *route=map_fg_layer_or_null(a->map_id,(unsigned)layer);
            const CampaignWorldLayer *f;
            if(route){converted=(CampaignWorldLayer){.x=route->x,.y=route->y,.w=route->w,.h=route->h,.depth=route->depth};f=&converted;}else f=&campaign_world_fg_layers[layer];
            int overlap=0;
#ifdef RENDER_DISABLE_FOREGROUND_CULL
            overlap=1;
#else
            /* Static foreground is already in terrain. Only redraw where a
               previous actor/effect could have covered this source layer. */
            for(int yy=0;yy<192&&!overlap;yy++){
                if(view.y[yy]<f->y||view.y[yy]>=f->y+f->h)continue;
                int left=view.dirty_left[yy],right=view.dirty_right[yy];
                if(left<right&&view.x[left]<f->x+f->w&&view.x[right-1]>=f->x)overlap=1;
            }
#endif
            if(route&&campaign_state&&(a->map_id==THIEVES_MAP||a->map_id==FOREST_MAP)){
                int bi=campaign_break_find(a->map_id,route->source,route->depth,route->x+route->w/2);
                if(bi>=0&&campaign_break_cut(campaign_state,(unsigned)bi))continue; /* already covered before the actors were drawn */
            }
            if(route&&campaign_state&&a->map_id==WAR_MAP){
                unsigned bit=route->source==303?1:route->source==172?2:route->source==316&&route->placement>=26&&route->placement<=32?1u<<(route->placement-24):0;
                if(bit&&(campaign_state->broken&bit)){
                    for(unsigned i=0;i<WAR_ALTERNATE_COUNT;i++){
                        const CampaignRouteLayer *alt=&war_alternate_layers[i];if(alt->source!=route->source||alt->depth!=route->depth)continue;
                        if(sprite_visible(alt->x,alt->y,alt->w,alt->h)&&war_alternate_decode(i,world_foreground))raster_sprite(&view,world_foreground,scene->palette,alt->x,alt->y,alt->w,alt->h,0,16,alt->h,32767);
                    }
                    continue;
                }
            }
            if(route&&route->source==161&&campaign_state&&a->map_id==WAR_MAP){
                unsigned frame=route->depth==261?77:route->depth==282?75:route->depth==301?76:route->depth==315?78:0;
                int broken=frame&&((phase==WAR_MACHINE_INTRO&&(unsigned)campaign_state->timer+1>=frame)||phase==WAR_MACHINE_FIGHT||phase==WAR_MACHINE_DEATH||(campaign_state->war.flags&WAR_FLAG_MACHINE_DONE));
                if(broken){
                    /* Source sprite160 plays 32 frames after cinema_wall.gotoAndStop(2): burst, flying planks, then nothing remains. */
                    unsigned age=phase==WAR_MACHINE_INTRO?(unsigned)campaign_state->timer+1-frame:999;
                    unsigned key=age<3?1:age<9?2:age<15?3:age<24?4:0;
                    for(unsigned i=0;key&&i<WAR_ALTERNATE_COUNT;i++){
                        const CampaignRouteLayer *alt=&war_alternate_layers[i];if(alt->depth!=route->depth||alt->dynamic!=key)continue;
                        if(sprite_visible(alt->x,alt->y,alt->w,alt->h)&&war_alternate_decode(i,world_foreground))raster_sprite(&view,world_foreground,scene->palette,alt->x,alt->y,alt->w,alt->h,0,16,alt->h,32767);
                    }
                    continue;
                }
            }
            if(route&&route->source==73&&a->map_id==8){
                unsigned frame=(a->tick%27)/3;const ArtFrame *smith=&hub_ambient_frames[frame];
                int x=381+smith->x,y=300+smith->y;
                if(sprite_visible(x,y,smith->w,smith->h)){
                    _Static_assert(HUB_AMBIENT_PIXELS_MAX<=sizeof(enemy_cache),"Smith uses idle hub enemy slot");
                    int key=-300-(int)frame;
                    if(knight_cache_id[2]!=key){if(!hub_ambient_decode(frame,(uint8_t*)enemy_cache))continue;knight_cache_id[2]=key;knight_cache_id[3]=-1;}
                    raster_sprite(&view,(uint8_t*)enemy_cache,hub_ambient_palette,x,y,smith->w,smith->h,0,16,smith->h,32767);
                }
                continue;
            }
            if(route&&route->source==133&&campaign_state&&a->map_id==WAR_MAP){gate_draw(campaign_state,a,f,layer,scene->palette);continue;}
            if(route&&route->dynamic)overlap=1;
            const uint8_t *fgpix=NULL;
            if(overlap&&f->x<camera.x+camera.width&&f->x+f->w>camera.x&&f->y<camera.y+camera.height&&f->y+f->h>camera.y&&(fgpix=fg_cached(a->map_id,(unsigned)layer,(unsigned)(f->w*f->h)))!=NULL){
#ifdef RENDER_DISABLE_FOREGROUND_CULL
                raster_sprite(&view,fgpix,scene->palette,f->x,f->y,f->w,f->h,0,16,f->h,32767);
#else
                if(route&&route->dynamic)raster_sprite(&view,fgpix,scene->palette,f->x,f->y,f->w,f->h,0,16,f->h,32767);
                else raster_foreground(&view,fgpix,scene->palette,f->x,f->y,f->w,f->h);
#endif
            }
        }
    }
    foreground(pixels,a);
    campaign_feedback(a,pixels,scene);
    for(int player=0;player<2;player++){
        const Fighter *p=&a->p[player];
        if(!p->ready||!p->equip_timer||p->bow_equipped||p->hp<=0)continue;
        if(campaign_state&&(campaign_potion_equipped(campaign_state,player)||campaign_state->shovel_equipped[player])){
            unsigned age=p->equip_timer<30?30-p->equip_timer:0;
            unsigned id=4+(campaign_potion_equipped(campaign_state,player)?0:8)+(age<8?age:7);
            const SelectorFrame *f=&selector_frames[id];uint8_t decoded[SELECTOR_MAX_PIXELS];
            if(selector_decode(id,decoded))raster_sprite(&view,decoded,selector_palette,p->x+f->x,p->y-p->z-63+f->y,f->w,f->h,0,16,f->h,32767);
            continue;
        }
        int width=menu_text_width("NO ITEM"),x=(p->x-camera.x)*256/camera.width-width/2;
        int y=(p->y-p->z-63-camera.y)*192/camera.height-7;
        if(x<3)x=3;
        if(x>253-width)x=253-width;
        if(y<3)y=3;
        if(y>174)y=174;
        /* Opaque plate gives font antialiasing the same backing on both renderers. */
        rect(pixels,x-2,y-2,width+4,18,RGB(3,4,6));
        for(int row=y-2;row<y+16;row++){
            if(view.dirty_left[row]>x-2)view.dirty_left[row]=x-2;
            if(view.dirty_right[row]<x+width+2)view.dirty_right[row]=x+width+2;
        }
        menu_text(pixels,x,y,"NO ITEM",RGB(31,29,20));
    }
    if(campaign_state&&a->map_id>=FOREST_MAP&&campaign_state->phase==WAR_COMPLETE)tally_draw(pixels,a,campaign_state);else tally_on=0;
    if(banner_title){ /* A plate over the top screen, where the player is looking (level-up and new combo notices). */
        int w1=menu_text_width(banner_title),w2=banner_keys?menu_text_width(banner_keys):0,w=(w1>w2?w1:w2)+12,x=(256-w)/2,h=banner_keys?36:22;
        rect(pixels,x,24,w,h,RGB(3,4,6));rect(pixels,x+1,25,w-2,h-2,RGB(29,27,21));
        for(int row=24;row<24+h;row++){if(view.dirty_left[row]>x)view.dirty_left[row]=(uint16_t)x;if(view.dirty_right[row]<x+w)view.dirty_right[row]=(uint16_t)(x+w);}
        menu_text(pixels,(256-w1)/2,28,banner_title,RGB(5,4,2));
        if(banner_keys)menu_text(pixels,(256-w2)/2,44,banner_keys,RGB(20,3,2));
    }
    if(a->countdown||a->winner||(a->running&&a->tick<110))
        for(int y=65;y<90;y++){
            view.dirty_left[y]=0;view.dirty_right[y]=256;
            /* Font antialiasing needs the scenery behind transparent pixels. */
            for(int x=0;x<256;x++)if(!(pixels[y*256+x]&0x8000))
                pixels[y*256+x]=scene_color(scene,view.x[x],view.y[y]);
        }
    if(a->countdown){const char *label=a->countdown>60?"3":a->countdown>30?"2":"1";menu_text(pixels,123,75,label,RGB(31,28,10));}
    else if(a->running&&!campaign_state&&a->tick<110)menu_text(pixels,106,75,"FIGHT!",RGB(31,28,10));
    if(a->winner){char result[48];
        if(campaign_enemy)snprintf(result,sizeof result,a->winner==1?(a->map_id==BOSS_MAP?"PRINCESS RESCUED":"LEVEL CLEARED"):"CAMPAIGN FAILED");
        else if(a->winner<=2)snprintf(result,sizeof result,"%s WINS",arena_knight_name(a->p[a->winner-1].knight_id));else snprintf(result,sizeof result,"DRAW");
        const char *label=result;int x=(256-menu_text_width(label))/2;rect(pixels,0,65,256,23,RGB(3,4,6));menu_text(pixels,x,69,label,RGB(31,28,10));}
}

void arena_render_banner(const char *title,const char *keys){banner_title=title;banner_keys=keys;}
const ArenaCamera *arena_render_camera(void){return &camera;}
const ArenaCamera *arena_render_background_camera(void){return &background_camera;}
void arena_upload_background(void *destination,uint16_t *palette,unsigned map){
    if(world_surface!=(uint16_t *)destination){world_surface=destination;terrain_map=-1;world_invalidate();}
    world_hw_palette=palette;
    const Scene *scene=scene_for(map);
    if(scene==&campaign_scene)world_invalidate(); /* arena_render streams the new map using its new camera. */
}

/* Called at vertical blank, before presenting the prepared camera/actor page.
   Never decode assets or draw sprites while copying the committed terrain. */
int arena_present_background(void){
    terrain_bytes_uploaded=0;
    if(world_map<3||!world_ready||!world_dirty||!world_surface)return 0;
    unsigned columns=0;for(unsigned bits=world_columns;bits;bits>>=1)columns+=bits&1;
    if(world_dirty==2||columns>4){
#ifdef ARM9
        DC_FlushRange(world_staging,sizeof(world_staging));
        dmaCopy(world_staging,world_surface,sizeof(world_staging));
#else
        memcpy(world_surface,world_staging,sizeof(world_staging));
#endif
        terrain_bytes_uploaded=sizeof(world_staging);
    }else{
        /* A horizontal crossing changes only32pixels, not the entire512wide
           ring. CPU halfword stores preserve VRAM's write granularity and
           avoid318 tiny DMA setups/cache flushes for each entering column. */
        volatile uint16_t *out=world_surface;
        for(unsigned slot=0;slot<16;slot++)if(world_columns&(1u<<slot)){
            for(unsigned y=0;y<WORLD_HEIGHT;y++){
                unsigned at=y*256+slot*16;
                for(unsigned x=0;x<16;x++)out[at+x]=world_staging[at+x];
            }
            terrain_bytes_uploaded+=32*WORLD_HEIGHT;
        }
    }
    if(world_hw_palette)memcpy(world_hw_palette,campaign_scene.palette,512);
    world_dirty=0;world_columns=0;return 1;
}

const uint16_t *arena_dirty_left(void){return view.dirty_left;}
const uint16_t *arena_dirty_right(void){return view.dirty_right;}

void arena_render_selection(uint16_t *pixels,unsigned map){
    lobby_loaded=0;
    const Scene *scene=scene_for(map);
    for(int y=0;y<192;y++)for(int x=0;x<256;x++)pixels[y*256+x]=scene_color(scene,x*WORLD_WIDTH/256,y*WORLD_HEIGHT/192);
    rect(pixels,0,0,256,25,RGB(3,4,6));rect(pixels,0,148,256,44,RGB(3,4,6));
    menu_text(pixels,12,5,"CHOOSE ARENA",RGB(31,27,10));
    const char *name=stage_name(map);menu_text(pixels,(256-menu_text_width(name))/2,153,name,RGB(31,31,31));
    menu_text(pixels,12,173,"Left/Right     A select     B back",RGB(31,27,10));
}

void arena_render_knight_selection(uint16_t *pixels,unsigned id){
    if(id>=KNIGHT_COLOR_COUNT)id=0;
    arena_render_selection(pixels,0);
    rect(pixels,0,0,256,28,RGB(3,4,6));menu_text(pixels,22,5,"CHOOSE YOUR KNIGHT",RGB(31,27,10));
    const ArtFrame *f=&knight_frames[id*KNIGHT_FRAMES_PER_COLOR];
    const uint8_t *data=frame_pixels(id*KNIGHT_FRAMES_PER_COLOR,0,0);
    for(int y=0;y<f->h;y++)for(int x=0;x<f->w;x++){
        unsigned c=data[y*f->w+x];if(!c)continue;
        rect(pixels,128+(f->x+x)*2,138+(f->y+y)*2,2,2,knight_palette[c]);
    }
    const char *names[]={"BLUE KNIGHT","ORANGE KNIGHT","RED KNIGHT","GREEN KNIGHT"};
    rect(pixels,0,148,256,44,RGB(3,4,6));
    menu_text(pixels,(256-menu_text_width(names[id]))/2,151,names[id],RGB(31,31,31));
    menu_text(pixels,12,173,"Left/Right     A select     B back",RGB(31,27,10));
}

/* Character select / waiting room: the original multiplayer lobby panels (LOBBY.swf, tools/convert_lobby_art.py): gold frame,
   colour-background knight portrait, up/down chevrons on your own panel, ready badge and the name and level on the plate. */
/* The lobby bitmaps (nitrofiles/lobby.bin) are read into the scenery workspace when the lobby is drawn: no level scenery is
   alive while a menu is up. Anything that renders scenery again drops them and the terrain caches they overwrote. */
#ifdef ARM9
#define LOBBY_FILE "nitro:/lobby.bin"
#else
#define LOBBY_FILE "nitrofiles/lobby.bin"
#endif
_Static_assert(LOBBY_PIXELS_SIZE<=sizeof(scenery_scratch.campaign.terrain),"Lobby bitmaps fit the terrain staging area");
static const uint8_t *lobby_bytes(void){
    uint8_t *at=(uint8_t *)scenery_scratch.campaign.terrain;
    if(lobby_loaded)return at;
    FILE *f=fopen(LOBBY_FILE,"rb");if(!f)return NULL;
    unsigned got=(unsigned)fread(at,1,LOBBY_PIXELS_SIZE,f);fclose(f);
    if(got!=LOBBY_PIXELS_SIZE)return NULL;
    world_invalidate();mask_map=terrain_map=world_map=-1;world_dirty=0;lobby_loaded=1;return at;
}
static void lobby_blit_clip(uint16_t *pixels,unsigned id,int x,int y,int clip0,int clip1){
    if(!lobby_decode(lobby_bytes(),id,world_foreground))return;
    const LobbyArtFrame *f=&lobby_frames[id];
    for(int yy=0;yy<f->h;yy++){int py=y+yy;if(py<0||py>=192)continue;
        for(int xx=0;xx<f->w;xx++){unsigned c=world_foreground[yy*f->w+xx];int px=x+xx;if(c&&px>=clip0&&px<clip1&&px>=0&&px<256)pixels[py*256+px]=lobby_palette[c];}}
}
static void lobby_blit(uint16_t *pixels,unsigned id,int x,int y){lobby_blit_clip(pixels,id,x,y,0,256);}
void arena_render_lobby(uint16_t *pixels,const ArenaLobby *l){
    for(int y=0;y<192;y++)for(int x=0;x<256;x++)pixels[y*256+x]=RGB(14+(y>>5),10+(y>>6),7);
    static const char *names[]={"BLUE","ORANGE","RED","GREEN"};
    for(unsigned s=0;s<2;s++){
        int x=38+(int)s*96,y=4,k=l->knight[s];
        rect(pixels,x+LOBBY_WINDOW_X,y+LOBBY_WINDOW_Y,LOBBY_WINDOW_W,LOBBY_WINDOW_H,RGB(11,11,12));
        if(k>=0&&k<4)lobby_blit_clip(pixels,LOBBY_ART_PORTRAIT_BLUE+(unsigned)k,x+LOBBY_PORTRAIT_X,y+LOBBY_PORTRAIT_Y,x+3,x+81);
        else menu_text(pixels,x+LOBBY_WINDOW_X+(LOBBY_WINDOW_W-menu_text_width("WAITING"))/2,y+LOBBY_WINDOW_Y+LOBBY_WINDOW_H/2-4,l->room?"WAITING":"EMPTY",RGB(24,24,24));
        lobby_blit(pixels,LOBBY_ART_PANEL,x,y);
        if(s==l->own&&k>=0)lobby_blit(pixels,LOBBY_ART_CHEVRONS,x+(84-32)/2,y+LOBBY_WINDOW_Y+(LOBBY_WINDOW_H-115)/2);
        unsigned badge=k<0?LOBBY_ART_BADGE_X:l->ready[s]?LOBBY_ART_BADGE_OK:LOBBY_ART_BADGE_WAIT;
        lobby_blit(pixels,badge,x+LOBBY_BADGE_CX-13,y+LOBBY_BADGE_CY-12);
        char t[16];
        if(k>=0&&k<4){
            const char *n=names[k];menu_text(pixels,x+8+(70-menu_text_width(n))/2,y+166,n,RGB(5,3,1));
            snprintf(t,sizeof t,l->level[s]?"%u":"--",l->level[s]);menu_text(pixels,x+63-menu_text_width(t)/2,y+148,t,RGB(5,3,1));
        }
        snprintf(t,sizeof t,"P%u",s+1);menu_text(pixels,x+10,y+4,t,RGB(5,3,1));
    }
}

void arena_render_movie(uint16_t *pixels,unsigned map,unsigned kind,unsigned tick){
    worldmap_cached=0;
    /* Movies carry scenery exported with their own source camera. A gameplay
       camera cannot align the throne, crystal or princesses correctly. */
    (void)map;cinematic_set_workspace((uint8_t *)&scenery_scratch+49152,sizeof(scenery_scratch)-49152);
    cinematic_draw(pixels,kind,tick);
}

/* The map reuses the existing 134 KiB scenery allocation as an indexed tile
   cache. Decompression happens once per entry, not each animated map frame. */
_Static_assert(672*192<=sizeof(arena_mask),"World map reuses scenery RAM");
void arena_render_world_map(uint16_t *pixels,const Campaign *c,const Arena *a){
    static const char *names[]={"BLACKSMITH","HOME CASTLE","CASTLE KEEP","BARBARIAN WAR","BARBARIAN BOSS","FOREST ENTRANCE","THIEVES' FOREST","MILL RUN","FLOODED CASTLE"};
    unsigned selected=c->group<CAMPAIGN_WORLD_NODES?c->group:1;
    int wx,wy;campaign_world_position(c,&wx,&wy);
    /* The 192 row window starts at map row 32, or at row 0 near the northern nodes (Mill Run lies above row 64). */
    static int worldmap_top=-1;int top=wy<80?0:((wy-80)/64)*64;if(top>256)top=256; /* the 448 row map is shown through a 192 row window that moves in steps of 64 rows */
    if(worldmap_top!=top)worldmap_cached=0;
    if(!worldmap_cached){
        /* Indexed map pixels replace both gameplay terrain and mask storage. */
        world_invalidate();mask_map=terrain_map=world_map=-1;world_dirty=0;
        uint8_t tile[32*64];
        for(int strip=0;strip<21;strip++)for(int row=0;row<192;row+=64){
            if(!original_worldmap_decode_rows(strip,top+row,64,tile))memset(tile,0,sizeof(tile));
            for(int y=0;y<64;y++)memcpy(arena_mask+(row+y)*672+strip*32,tile+y*32,32);
        }
        worldmap_cached=1;worldmap_top=top;
    }
    int x0=wx-128;if(x0<0)x0=0;if(x0>416)x0=416;
    for(int y=0;y<192;y++)for(int x=0;x<256;x++)pixels[y*256+x]=original_worldmap_palette[arena_mask[y*672+x+x0]]|0x8000;
    int moving=campaign_world_travelling(c),facing=c->group>=c->spawned?1:-1;
    int coop=a->p[1].ready;
    for(unsigned h=0;h<2;h++){
        if(!a->p[h].ready)continue;
        unsigned knight=a->p[h].knight_id<KNIGHT_COLOR_COUNT?a->p[h].knight_id:0;
        unsigned pose=moving?POSE_WALK:POSE_IDLE;
        unsigned frame=knight*KNIGHT_FRAMES_PER_COLOR+pose_start[pose]+(a->tick/4)%pose_count[pose];
        const ArtFrame *f=&knight_frames[frame];const uint8_t *data=frame_pixels(frame,h,0);
        int px=wx-x0+(coop?(h?9:-9):0),py=wy-top+(h?3:0);
        /* Half-sized original walk/idle frames keep the path and node visible. */
        for(int y=0;y<f->h;y+=2)for(int x=0;x<f->w;x+=2){
            unsigned color=data[y*f->w+x];if(!color)continue;
            int dx=px+facing*(f->x+x)/2,dy=py+(f->y+y)/2;
            if(dx>=0&&dx<256&&dy>=0&&dy<171)pixels[dy*256+dx]=knight_palette[color];
        }
    }
    rect(pixels,0,171,256,21,RGB(3,5,8));
    if(selected==8&&!(c->completed&64)){menu_text(pixels,6,178,"FLOODED CASTLE",RGB(31,29,20));menu_text(pixels,130,178,"CLEAR MILL RUN",RGB(25,23,18));}
    else if(selected==7&&!(c->completed&32)){menu_text(pixels,6,178,"MILL RUN",RGB(31,29,20));menu_text(pixels,105,178,"CLEAR THIEVES FIRST",RGB(25,23,18));}
    else if(selected==6&&!(c->completed&16)){menu_text(pixels,6,178,"THIEVES",RGB(31,29,20));menu_text(pixels,105,178,"CLEAR FOREST FIRST",RGB(25,23,18));}
    else if(selected==5&&!(c->completed&4)){menu_text(pixels,6,178,"FOREST",RGB(31,29,20));menu_text(pixels,105,178,"CLEAR BOSS FIRST",RGB(25,23,18));}
    else if(selected==4&&!(c->completed&2)){menu_text(pixels,6,178,"BOSS",RGB(31,29,20));menu_text(pixels,105,178,"CLEAR WAR FIRST",RGB(25,23,18));}
    else if(selected==3&&!(c->completed&1)){menu_text(pixels,6,178,"WAR",RGB(31,29,20));menu_text(pixels,94,178,"CLEAR KEEP FIRST",RGB(25,23,18));}
    else menu_text(pixels,14,178,names[selected],RGB(31,29,20));
}
