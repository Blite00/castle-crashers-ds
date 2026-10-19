#ifndef TEST_NDS_H
#define TEST_NDS_H
#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;
typedef enum {SoundFormat_ADPCM} SoundFormat;
void soundEnable(void);void soundKill(int);void soundSetMasterVolume(int);void soundSetVolume(int,int);void DC_FlushRange(const void*,unsigned);
int soundPlaySampleChannel(int,const void*,SoundFormat,unsigned,unsigned,int,int,bool,unsigned);
#endif
