#include "chapter_assets.h"
#include "chapter_audio.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 chapter_assets_invalidate();assert(!chapter_assets_audio_workspace(1));assert(!chapter_assets_boss_tile_workspace());assert(chapter_assets_load(CHAPTER_WAR));
 uint8_t*p=chapter_assets_audio_workspace(CHAPTER_AUDIO_WAR_BYTES);assert(p&&!((uintptr_t)p&31));assert(p>=chapter_asset_buffer&&p+CHAPTER_AUDIO_WAR_BYTES<=chapter_asset_buffer+CHAPTER_ASSET_CAPACITY);
 uint8_t first[32];memcpy(first,chapter_asset_buffer,32);memset(p,0xa5,CHAPTER_AUDIO_WAR_BYTES);assert(!memcmp(first,chapter_asset_buffer,32));assert(!chapter_assets_audio_workspace(CHAPTER_ASSET_CAPACITY));
 assert(chapter_assets_load(CHAPTER_HUBS));assert(chapter_assets_audio_workspace(CHAPTER_AUDIO_WORLDMAP_BYTES));
 assert(chapter_assets_load(CHAPTER_BOSS));assert(chapter_assets_audio_workspace(CHAPTER_AUDIO_BOSS_BYTES));
 assert(chapter_assets_audio_workspace(CHAPTER_AUDIO_BOSS_DUEL_BYTES));assert(chapter_assets_audio_workspace(CHAPTER_AUDIO_BOSS_VICTORY_BYTES));
 /* Audio and the renderer borrow opposite ends of the same bank. Exercise
  * actual writes in both directions, not merely pointer inequality. */
 uint8_t*tiles=chapter_assets_boss_tile_workspace();p=chapter_assets_audio_workspace(CHAPTER_AUDIO_BOSS_DUEL_BYTES);
 assert(tiles==chapter_asset_buffer+CHAPTER_ASSET_CAPACITY-CHAPTER_BOSS_TILE_CACHE_BYTES);
 assert(!((uintptr_t)tiles&31)&&p+CHAPTER_AUDIO_BOSS_DUEL_BYTES<=tiles);
 unsigned remaining=(unsigned)(tiles-p);assert(remaining>=CHAPTER_AUDIO_BOSS_DUEL_BYTES);
 assert(chapter_assets_audio_workspace(remaining)==p);assert(!chapter_assets_audio_workspace(remaining+1));
 memcpy(first,chapter_asset_buffer,32);uint8_t before=p[-1];
 memset(tiles,0x6d,CHAPTER_BOSS_TILE_CACHE_BYTES);memset(p,0xa5,CHAPTER_AUDIO_BOSS_DUEL_BYTES);
 for(unsigned i=0;i<CHAPTER_BOSS_TILE_CACHE_BYTES;i++)assert(tiles[i]==0x6d);
 assert(p[-1]==before&&!memcmp(first,chapter_asset_buffer,32));
 memset(tiles,0x3c,CHAPTER_BOSS_TILE_CACHE_BYTES);
 for(unsigned i=0;i<CHAPTER_AUDIO_BOSS_DUEL_BYTES;i++)assert(p[i]==0xa5);
 /* The maximum valid audio extent must stop immediately before the cache. */
 memset(p,0x96,remaining);for(unsigned i=0;i<CHAPTER_BOSS_TILE_CACHE_BYTES;i++)assert(tiles[i]==0x3c);
 assert(p[-1]==before&&!memcmp(first,chapter_asset_buffer,32));
 assert(chapter_assets_load(CHAPTER_KEEP));assert(!chapter_assets_music_workspace(CHAPTER_ASSET_CAPACITY+1));assert(chapter_assets_load(CHAPTER_KEEP));assert(chapter_assets_music_workspace(307440)==chapter_asset_buffer&&!chapter_assets_ready(CHAPTER_KEEP));assert(chapter_assets_load(CHAPTER_ARENAS));assert((uint8_t*)chapter_assets_music_workspace(231556)>chapter_asset_buffer&&chapter_assets_ready(CHAPTER_ARENAS));assert(chapter_assets_load(CHAPTER_THIEVES));assert(chapter_assets_audio_workspace(CHAPTER_AUDIO_THIEVES_BYTES)&&chapter_assets_audio_workspace(CHAPTER_AUDIO_THIEVES_BOSS_BYTES));
 assert(chapter_assets_load(CHAPTER_KEEP));assert(!chapter_assets_boss_tile_workspace());assert(chapter_assets_audio_workspace(1)&&!chapter_assets_audio_workspace(CHAPTER_ASSET_CAPACITY-852608+1));chapter_assets_invalidate();assert(!chapter_assets_audio_workspace(1));assert(!chapter_assets_boss_tile_workspace());
 puts("Chapter audio: loaded-bank tail capacity,32byte alignment,asset/32KiB boss-cache isolation, exact capacity+1 rejection and invalidation PASS");return 0;
}
