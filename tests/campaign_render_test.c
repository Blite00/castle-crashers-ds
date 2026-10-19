#include "cinematic.h"
#include "render.h"
#include "stage.h"
#include "campaign.h"
#include "barbarian_art.h"
#include "campaign_world.h"
#include "campaign_shop.h"
#include "campaign_barracks.h"
#include "arena_terrain_packed.h"
#include "arena_background.h"
#include "arena_king.h"
#include "arena_volcano.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t vram[512*512/2],palette[256],screen[256*192+2];
static uint8_t strip[CAMPAIGN_WORLD_STRIP_WIDTH*CAMPAIGN_WORLD_HEIGHT],terrain[424*318];
static unsigned visible_hash(void){unsigned hash=2166136261u;for(unsigned i=0;i<sizeof(vram)/sizeof(*vram);i++)hash=(hash^vram[i])*16777619u;for(unsigned i=0;i<256;i++)hash=(hash^palette[i])*16777619u;return hash;}
static void draw(Arena *a){
    screen[256*192]=0x1234;screen[256*192+1]=0x5678;
    unsigned before=visible_hash();arena_render(a,screen);
    if(a->map_id>=3)assert(before==visible_hash()); /* No live terrain or palette writes. */
    const ArenaCamera *sampling=arena_render_camera();
    assert(!(sampling->width&3));
    for(int y=0;y<192;y++)assert(y*sampling->height/192==y*sampling->width/256);
    arena_present_background();
    assert(screen[256*192]==0x1234&&screen[256*192+1]==0x5678);
}
int main(void){
    const unsigned maps[]={5,3,4,0,1,2,5,3,4,5,3};
    const uint16_t *arena_palettes[]={arena_palette,king_palette,volcano_palette};
    Arena a;arena_init(&a);a.p[1].ready=0;
    for(unsigned pass=0;pass<sizeof(maps)/sizeof(*maps);pass++){
        unsigned map=maps[pass];a.map_id=map;arena_render_campaign(NULL);
        memset(vram,0x5a,sizeof(vram));arena_upload_background(vram,palette,map);
        if(map<3)assert(decode_arena_terrain(map,terrain));
        for(int n=0;n<120;n++){
            a.p[0].x=40+n*(stage_world_width(map)-80)/119;
            a.p[0].y=100+(n%3)*140;a.round++;draw(&a);
            const ArenaCamera *world=arena_render_camera(),*bg=arena_render_background_camera();
            assert(bg->y>=0&&bg->y+bg->height<=WORLD_HEIGHT);
            assert(bg->x==(world->x&511));
            for(int x=0;x<world->width;x+=17){
                int wx=world->x+x;
                if(map>=3)assert(map==5?campaign_barracks_decode_strip(wx/32,strip):map==4?campaign_shop_decode_strip(wx/32,strip):campaign_world_decode_strip(wx/32,strip));
                for(int y=0;y<world->height;y+=13){
                    int bx=(bg->x+x)&511,by=bg->y+y;
                    unsigned pair=vram[by*256+bx/2],got=(pair>>((bx&1)*8))&255;
                    unsigned expected=map>=3?strip[(world->y+y)*32+(wx&31)]:terrain[(world->y+y)*424+wx];
                    const uint16_t *expected_palette=map==5?campaign_barracks_palette:map==4?campaign_shop_palette:map==3?campaign_world_palette:arena_palettes[map];
                    assert(got==expected);assert(palette[got]==expected_palette[got]);
                }
            }
            for(int i=320*256;i<512*256;i++)assert(vram[i]==0x5a5a);
        }
    }
    /* Camera pans smoothly to the abduction and returns to the exact player view. */
    Campaign c={0};c.phase=KEEP_TRAVEL;a.map_id=3;a.p[0].x=200;a.p[0].y=240;a.round++;
    arena_render_campaign(&c);arena_upload_background(vram,palette,3);draw(&a);
    ArenaCamera origin=*arena_render_camera();int previous=origin.x;
    c.phase=KEEP_PAN_IN;
    for(int t=0;t<=18;t++){c.timer=t;draw(&a);const ArenaCamera *v=arena_render_camera();assert(v->x>=previous);previous=v->x;}
    int movie_x,movie_y;cinematic_camera(CINEMATIC_PRINCESSES,0,&movie_x,&movie_y);
    assert(arena_render_camera()->x==movie_x&&arena_render_camera()->y==movie_y);
    cinematic_camera(CINEMATIC_PRINCESSES,cinematic_duration(CINEMATIC_PRINCESSES)-1,&movie_x,&movie_y);previous=movie_x;
    c.phase=KEEP_PAN_OUT;
    for(int t=0;t<=18;t++){c.timer=t;draw(&a);const ArenaCamera *v=arena_render_camera();assert(v->x<=previous);previous=v->x;}
    assert(arena_render_camera()->x==origin.x&&arena_render_camera()->y==origin.y&&arena_render_camera()->width==origin.width);
    /* A guest may miss every pan-in snapshot: pan-out must still have a valid view. */
    c.phase=KEEP_TRAVEL;a.p[0].x=1450;a.p[0].y=370;a.round++;draw(&a);
    c.phase=KEEP_PAN_OUT;
    for(int t=0;t<=18;t++){c.timer=t;draw(&a);assert(arena_render_camera()->width>=288&&arena_render_camera()->height>=216);}
    /* Barracks intro framing is deterministic even without earlier radio snapshots. */
    a.map_id=5;a.p[0].x=336;a.p[0].y=284;a.p[1].ready=0;c.phase=KEEP_BARRACKS_INTRO;c.timer=161;
    arena_upload_background(vram,palette,5);draw(&a);ArenaCamera entry=*arena_render_camera();
    assert(entry.x<=1100&&entry.x+entry.width>1100);
    c.timer=250;draw(&a);c.timer=161;draw(&a);
    assert(arena_render_camera()->x==entry.x&&arena_render_camera()->y==entry.y);
    c.timer=486;draw(&a);assert(arena_render_camera()->x<=336&&arena_render_camera()->x+arena_render_camera()->width>336);
    /* Authored vertical pans must reuse terrain rows; one pixel must not
       trigger another complete448-row decode for every visible strip. */
    unsigned partial=0,total=0;
    c.phase=KEEP_BARRACKS_INTRO;a.map_id=5;a.round=5;
    for(int t=0;t<486;t++){
        c.timer=t;a.tick=t;draw(&a);unsigned rows=arena_render_terrain_rows();total+=rows;
        if(rows&&rows<318)partial++;
    }
    assert(partial>20);printf("Barracks scroll: %u partial-row updates, %u output rows over486 frames\n",partial,total);
    /* Ordinary horizontal walking uploads only entering32pixel columns.
       Compare every displayed terrain sample above; retain untouched VRAM and
       reserved actor pages across these partial commits. */
    arena_render_campaign(NULL);a.map_id=3;a.p[0].x=200;a.p[0].y=260;a.round++;
    arena_upload_background(vram,palette,3);draw(&a);
    unsigned narrow=0,bytes=0;
    for(unsigned t=0;t<180;t++){
        a.p[0].x+=2;draw(&a);unsigned copied=arena_render_terrain_upload_bytes();bytes+=copied;
        if(copied){assert(copied<=2*32*WORLD_HEIGHT);narrow++;}
        const ArenaCamera *world=arena_render_camera(),*bg=arena_render_background_camera();
        for(int x=0;x<256;x+=19){
            unsigned wx=world->x+x*world->width/256;assert(campaign_world_decode_strip(wx/32,strip));
            for(int y=0;y<192;y+=23){
                unsigned wy=world->y+y*world->height/192,bx=(bg->x+x*bg->width/256)&511,by=bg->y+y*bg->height/192;
                unsigned pair=vram[by*256+bx/2];assert(((pair>>((bx&1)*8))&255)==strip[wy*32+(wx&31)]);
            }
        }
    }
    assert(narrow>5);assert(!arena_present_background());assert(!arena_render_terrain_upload_bytes());
    printf("Horizontal terrain: %u narrow commits, %u bytes (full copy would be %u)\n",narrow,bytes,narrow*512*WORLD_HEIGHT);
    /* Invisible enemies must not inflate their changing poses; the hero's
       stable cache remains unchanged. Reentering view still decodes normally. */
    c=(Campaign){0};c.phase=KEEP_TRAVEL;arena_render_campaign(&c);draw(&a);
    unsigned decoded=arena_render_sprite_decodes();
    for(unsigned e=0;e<6;e++){c.enemies[e].ready=1;c.enemies[e].hp=100;c.enemies[e].x=3000;c.enemies[e].y=260;c.enemies[e].pose=POSE_WALK;c.enemies[e].animation=(int)e;}
    draw(&a);assert(arena_render_sprite_decodes()==decoded);
    c.enemies[0].x=a.p[0].x+50;draw(&a);assert(arena_render_sprite_decodes()>decoded);
    /* Peasants reuse one idle enemy slot; held animation frames need no decode. */
    c=(Campaign){0};c.phase=KEEP_TRAVEL;c.broken=1;a.map_id=3;a.p[1].ready=0;
    arena_render_campaign(&c);
    uint8_t *enemy_memory=arena_barracks_audio_workspace(6*BARBARIAN_FRAME_PIXELS_MAX);
    assert(enemy_memory);memset(enemy_memory,0xd4,6*BARBARIAN_FRAME_PIXELS_MAX);
    a.p[0].x=980;a.p[0].y=350;a.round++;c.npc_ticks=4;draw(&a);
    decoded=arena_render_sprite_decodes();
    for(unsigned t=0;t<120;t++){
        c.npc_ticks=(uint8_t)(4+t);a.p[0].x=980-(int)t*3;a.round++;draw(&a);
    }
    unsigned npc_decodes=arena_render_sprite_decodes()-decoded;
    assert(npc_decodes<=90&&npc_decodes>10);
    for(unsigned i=0;i<5*BARBARIAN_FRAME_PIXELS_MAX;i++)assert(enemy_memory[i]==0xd4);
    for(unsigned i=5*BARBARIAN_FRAME_PIXELS_MAX+4364;i<6*BARBARIAN_FRAME_PIXELS_MAX;i++)assert(enemy_memory[i]==0xd4);
    /* A busy sixth enemy uses shared scratch instead, pixel-identically. */
    a.p[0].x=900;a.round++;c.npc_ticks=30;draw(&a);
    uint32_t cached_hash=2166136261u;for(unsigned i=0;i<256*192;i++)cached_hash=(cached_hash^screen[i])*16777619u;
    c.enemies[5]=(Fighter){.ready=1,.hp=100,.x=3000,.y=350};a.round++;draw(&a);
    uint32_t fallback_hash=2166136261u;for(unsigned i=0;i<256*192;i++)fallback_hash=(fallback_hash^screen[i])*16777619u;
    assert(cached_hash==fallback_hash);
    /* Audio borrowing invalidates ownership and reloads current NPC poses. */
    c.enemies[5].ready=0;assert(arena_barracks_audio_workspace(6*BARBARIAN_FRAME_PIXELS_MAX));
    memset(enemy_memory,0xd4,6*BARBARIAN_FRAME_PIXELS_MAX);a.round++;draw(&a);
    uint32_t reclaimed_hash=2166136261u;for(unsigned i=0;i<256*192;i++)reclaimed_hash=(reclaimed_hash^screen[i])*16777619u;
    assert(cached_hash==reclaimed_hash);
    printf("Peasant cache: %u decodes over120 frames; guards/fallback/audio reclaim exact\n",npc_decodes);
    /* Baked architecture needs no decode until something overlaps its pixels. */
    arena_init(&a);a.map_id=3;a.p[0].x=840;a.p[0].y=230;
    a.p[0].ready=a.p[1].ready=0;arena_render_campaign(NULL);a.round++;
    unsigned fg_before=arena_render_foreground_decodes();draw(&a);
    assert(arena_render_foreground_decodes()==fg_before);
    a.p[0].ready=1;a.p[0].hp=100;draw(&a);
    assert(arena_render_foreground_decodes()>fg_before);
    printf("Foreground: empty view0 decodes, overlapping hero%u layers\n",arena_render_foreground_decodes()-fg_before);
    /* World boards reuse only inactive enemy buffers. Held states decode
       once; an occupied offscreen sixth slot must render identically. */
    arena_init(&a);a.map_id=4;a.p[0].x=639;a.p[0].y=250;a.p[1].ready=0;
    c=(Campaign){0};c.phase=KEEP_STORE;arena_render_campaign(&c);a.round++;draw(&a);
    decoded=arena_render_sprite_decodes();draw(&a);
    assert(arena_render_sprite_decodes()==decoded);
    a.round++;draw(&a);uint32_t shop_hash=2166136261u;
    for(unsigned i=0;i<256*192;i++)shop_hash=(shop_hash^screen[i])*16777619u;
    enemy_memory=arena_barracks_audio_workspace(6*BARBARIAN_FRAME_PIXELS_MAX);
    assert(enemy_memory);memset(enemy_memory,0xd4,6*BARBARIAN_FRAME_PIXELS_MAX);
    c.enemies[5]=(Fighter){.ready=1,.hp=100,.x=3000,.y=250};a.round++;draw(&a);
    uint32_t shop_fallback=2166136261u;
    for(unsigned i=0;i<256*192;i++)shop_fallback=(shop_fallback^screen[i])*16777619u;
    assert(shop_hash==shop_fallback);
    for(unsigned i=5*BARBARIAN_FRAME_PIXELS_MAX;i<6*BARBARIAN_FRAME_PIXELS_MAX;i++)assert(enemy_memory[i]==0xd4);
    arena_render_campaign(NULL);
    puts("PASS: Keep/shop/arena transitions, palette restoration, ring wrap, vertical rebases, reserved overlay rows and monotonic 18-tick camera pans");
}
