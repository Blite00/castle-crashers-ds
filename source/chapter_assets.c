#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "chapter_assets.h"
#include "chapter_asset_manifest.h"
#include <stdio.h>
#include <string.h>
#ifdef ARM9
#include <nds.h>
#endif
uint8_t chapter_asset_buffer[CHAPTER_ASSET_CAPACITY] __attribute__((aligned(32)));
static int active_bank=-1;
static enum ChapterAssetError last_error=CHAPTER_ASSET_OK;
static void (*progress_callback)(unsigned,unsigned);
static uint32_t le32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t crc_step(uint32_t crc,const uint8_t*p,unsigned count){
 static const uint32_t table[16]={0,0x1db71064,0x3b6e20c8,0x26d930ac,0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,0xedb88320,0xf00f9344,0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c};
 while(count--){crc^=*p++;crc=(crc>>4)^table[crc&15];crc=(crc>>4)^table[crc&15];}return crc;
}
void chapter_assets_set_progress(void (*callback)(unsigned,unsigned)){progress_callback=callback;}
void chapter_assets_invalidate(void){active_bank=-1;}
int chapter_assets_ready(unsigned bank){return bank<CHAPTER_ASSET_BANK_COUNT&&active_bank==(int)bank;}
void *chapter_assets_audio_workspace(unsigned bytes){
 if(active_bank<0)return NULL;
 unsigned start=(chapter_asset_bank_sizes[active_bank]+31u)&~31u;
 unsigned limit=CHAPTER_ASSET_CAPACITY-(active_bank==CHAPTER_BOSS?CHAPTER_BOSS_TILE_CACHE_BYTES:0);
 return start<=limit&&bytes<=limit-start?chapter_asset_buffer+start:NULL;
}

void *chapter_assets_music_workspace(unsigned bytes){
 void *tail=chapter_assets_audio_workspace(bytes);
 if(tail)return tail;
 if(bytes>CHAPTER_ASSET_CAPACITY)return NULL;
 active_bank=-1; /* Menu screens own the whole buffer; the next chapter reloads its bank. */
 return chapter_asset_buffer;
}

void *chapter_assets_boss_tile_workspace(void){return chapter_assets_ready(CHAPTER_BOSS)?chapter_asset_buffer+CHAPTER_ASSET_CAPACITY-CHAPTER_BOSS_TILE_CACHE_BYTES:NULL;}

enum ChapterAssetError chapter_assets_error(void){return last_error;}
const char*chapter_assets_error_message(void){
 static const char*const messages[]={"Ready","Unknown chapter","Chapter file missing","Chapter version mismatch","Chapter size mismatch","Chapter read failed","Chapter data damaged"};
 return messages[last_error];
}
int chapter_assets_load_path(unsigned bank,const char*path){
 active_bank=-1;last_error=CHAPTER_ASSET_OK;
 if(bank>=CHAPTER_ASSET_BANK_COUNT){last_error=CHAPTER_ASSET_BAD_BANK;return 0;}
 if(!path){last_error=CHAPTER_ASSET_OPEN;return 0;}
 FILE*f=fopen(path,"rb");if(!f){last_error=CHAPTER_ASSET_OPEN;return 0;}
 (void)setvbuf(f,NULL,_IONBF,0);
 uint8_t header[32];unsigned total=chapter_asset_bank_sizes[bank];
 if(fread(header,1,sizeof(header),f)!=sizeof(header)){last_error=CHAPTER_ASSET_READ;goto failed;}
 if(memcmp(header,"CCDSBNK1",8)||le32(header+8)!=1||le32(header+12)!=bank||le32(header+24)||le32(header+28)){last_error=CHAPTER_ASSET_HEADER;goto failed;}
 if(le32(header+16)!=total||total>CHAPTER_ASSET_CAPACITY){last_error=CHAPTER_ASSET_SIZE;goto failed;}
 if(le32(header+20)!=chapter_asset_bank_crcs[bank]){last_error=CHAPTER_ASSET_CHECKSUM;goto failed;}
 uint32_t crc=0xffffffffu;unsigned done=0;
 if(progress_callback)progress_callback(0,total);
 while(done<total){unsigned count=total-done;if(count>16384)count=16384;
  if(fread(chapter_asset_buffer+done,1,count,f)!=count){last_error=CHAPTER_ASSET_READ;goto failed;}
  crc=crc_step(crc,chapter_asset_buffer+done,count);done+=count;if(progress_callback)progress_callback(done,total);
 }
 if(fgetc(f)!=EOF){last_error=CHAPTER_ASSET_SIZE;goto failed;}
 if(ferror(f)){last_error=CHAPTER_ASSET_READ;goto failed;}
 if((crc^0xffffffffu)!=chapter_asset_bank_crcs[bank]){last_error=CHAPTER_ASSET_CHECKSUM;goto failed;}
 fclose(f);
#ifdef ARM9
 DC_FlushRange(chapter_asset_buffer,total);
#endif
 active_bank=(int)bank;return 1;
failed:
 fclose(f);return 0;
}
int chapter_assets_load(unsigned bank){
 static const char*const names[]={
#ifdef ARM9
 "nitro:/chapters/arenas.bin","nitro:/chapters/keep.bin","nitro:/chapters/war.bin","nitro:/chapters/hubs.bin","nitro:/chapters/boss.bin","nitro:/chapters/forest.bin","nitro:/chapters/thieves.bin","nitro:/chapters/deer.bin","nitro:/chapters/flood.bin","nitro:/chapters/camp.bin"
#else
 "nitrofiles/chapters/arenas.bin","nitrofiles/chapters/keep.bin","nitrofiles/chapters/war.bin","nitrofiles/chapters/hubs.bin","nitrofiles/chapters/boss.bin","nitrofiles/chapters/forest.bin","nitrofiles/chapters/thieves.bin","nitrofiles/chapters/deer.bin","nitrofiles/chapters/flood.bin","nitrofiles/chapters/camp.bin"
#endif
 };
 if(chapter_assets_ready(bank)){last_error=CHAPTER_ASSET_OK;return 1;}
 if(bank>=CHAPTER_ASSET_BANK_COUNT){last_error=CHAPTER_ASSET_BAD_BANK;return 0;}
 return chapter_assets_load_path(bank,names[bank]);
}
