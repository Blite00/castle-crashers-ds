#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "chapter_assets.h"
#include "chapter_asset_manifest.h"
#include "campaign_world.h"
#include "campaign_shop.h"
#include "campaign_barracks.h"
#include "arena_terrain_packed.h"
#include "war_art.h"
#include "bridge_art.h"
#include "war_machine_art.h"
#include "war_live_art.h"
#include "war_cannon_art.h"
#include "thief_art.h"
#include "blacksmith_art.h"
#include "frog_art.h"
#include "ark_art.h"
#include "original_worldmap.h"
#include "boss_art.h"
#include "boss_actor_art.h"
#include "hub_ambient_art.h"
_Static_assert(sizeof(campaign_world_row_data)==CHAPTER_LENGTH_CAMPAIGN_WORLD_ROW_DATA,"Array shape retained");
_Static_assert(sizeof(campaign_shop_row_data)==CHAPTER_LENGTH_CAMPAIGN_SHOP_ROW_DATA,"Shop array shape");
_Static_assert(sizeof(campaign_barracks_row_data)==CHAPTER_LENGTH_CAMPAIGN_BARRACKS_ROW_DATA,"Barracks array shape");
static const uint8_t*const static_initializer=campaign_world_row_data;
static unsigned calls,last,total;
static void progress(unsigned done,unsigned all){assert(done>=last);assert(done<=all);assert(!chapter_assets_ready(0)&&!chapter_assets_ready(1));last=done;total=all;calls++;}
int main(void){
 assert(!chapter_assets_ready(0)&&!chapter_assets_ready(1));assert(((uintptr_t)chapter_asset_buffer&31)==0);
 chapter_assets_set_progress(progress);assert(chapter_assets_load(0));assert(chapter_assets_ready(0));assert(!chapter_assets_ready(1));assert(last==total&&calls>1);assert(arena_terrain_packed_data==chapter_asset_buffer+CHAPTER_OFFSET_ARENA_TERRAIN_PACKED_DATA);
 unsigned saved=calls;assert(chapter_assets_load(0));assert(calls==saved);last=0;assert(chapter_assets_load(1));assert(chapter_assets_ready(1)&&!chapter_assets_ready(0));assert(total==985812);assert(static_initializer==chapter_asset_buffer+CHAPTER_OFFSET_CAMPAIGN_WORLD_ROW_DATA);
 uint8_t pixels[32*448];assert(campaign_world_decode_strip(0,pixels));assert(campaign_shop_decode_strip(0,pixels));assert(campaign_barracks_decode_strip(0,pixels));
 chapter_assets_set_progress(NULL);
 assert(chapter_assets_load(CHAPTER_WAR));assert(chapter_assets_ready(CHAPTER_WAR));
 assert(campaign_war_decode_strip(20,pixels)&&campaign_bridge_decode_strip(4,pixels));
 assert(war_machine_decode_tile(0,pixels)&&war_live_decode(0,pixels)&&thief_decode_frame(0,pixels)&&war_cannon_decode_tile(0,pixels));
 assert(chapter_assets_load(CHAPTER_HUBS));assert(!chapter_assets_ready(CHAPTER_WAR));
 assert(campaign_blacksmith_decode_strip(2,pixels)&&campaign_frog_decode_strip(10,pixels)&&campaign_ark_decode_strip(9,pixels));
 assert(original_worldmap_decode_rows(3,19,192,pixels));
 assert(hub_ambient_decode(8,pixels));
 assert(chapter_assets_load(CHAPTER_BOSS));assert(campaign_boss_decode_rows(12,100,100,pixels)&&boss_actor_decode_tile(0,pixels));
 const char*bad[]={"build/chapter-test/missing.bin","build/chapter-test/short.bin","build/chapter-test/version.bin","build/chapter-test/wrong-bank.bin","build/chapter-test/size.bin","build/chapter-test/payload.bin","build/chapter-test/extra.bin","build/chapter-test/truncated.bin"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){assert(!chapter_assets_load_path(1,bad[i]));assert(!chapter_assets_ready(0)&&!chapter_assets_ready(1));assert(chapter_assets_error()!=CHAPTER_ASSET_OK);assert(strlen(chapter_assets_error_message()));}
 assert(!chapter_assets_load(CHAPTER_ASSET_BANK_COUNT));assert(!chapter_assets_load_path(0,NULL));assert(chapter_assets_load(0));assert(chapter_assets_load(1));assert(chapter_assets_error()==CHAPTER_ASSET_OK);chapter_assets_invalidate();assert(!chapter_assets_ready(1));
 puts("PASS: aligned chapter banks, zero-read repeat load, all three Keep decoders, static array views, eight load failures and recovery");return 0;
}
