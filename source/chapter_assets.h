#ifndef CHAPTER_ASSETS_H
#define CHAPTER_ASSETS_H
#include <stdint.h>
#define CHAPTER_ASSET_CAPACITY 1040000u
#define CHAPTER_BOSS_TILE_CACHE_BYTES 49152u
enum ChapterAssetBank {CHAPTER_ARENAS=0,CHAPTER_KEEP=1,CHAPTER_WAR=2,CHAPTER_HUBS=3,CHAPTER_BOSS=4,CHAPTER_FOREST=5,CHAPTER_THIEVES=6,CHAPTER_DEER=7,CHAPTER_FLOOD=8,CHAPTER_CAMP=9};
enum ChapterAssetError {CHAPTER_ASSET_OK=0,CHAPTER_ASSET_BAD_BANK,CHAPTER_ASSET_OPEN,CHAPTER_ASSET_HEADER,CHAPTER_ASSET_SIZE,CHAPTER_ASSET_READ,CHAPTER_ASSET_CHECKSUM};
extern uint8_t chapter_asset_buffer[CHAPTER_ASSET_CAPACITY];
int chapter_assets_load(unsigned bank);
int chapter_assets_load_path(unsigned bank,const char*path);
int chapter_assets_ready(unsigned bank);
/* Borrow aligned tail only while this bank remains loaded. Stop DMA before loading another bank. */
void *chapter_assets_audio_workspace(unsigned bytes);
/* Like the audio workspace, but drops the loaded bank when its tail is too small. */
void *chapter_assets_music_workspace(unsigned bytes);
void *chapter_assets_boss_tile_workspace(void);
enum ChapterAssetError chapter_assets_error(void);
const char*chapter_assets_error_message(void);
void chapter_assets_invalidate(void);
void chapter_assets_set_progress(void (*callback)(unsigned loaded,unsigned total));
#endif
