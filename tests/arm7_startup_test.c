#include <assert.h>
#include <stdio.h>
#define main arm7_main
#include "../arm7/source/main.c"
#undef main
uint16_t REG_KEYINPUT=0xffff;
static int waits,installs,updates,acks,inputs,audio_ready,system_ready;
static void (*tick)(void),(*stop)(void),(*request)(u32,void *);
void enableSound(void){} void readUserSettings(void){} void ledBlink(int x){(void)x;}
void touchInit(void){} void irqInit(void){} void fifoInit(void){}
void installSoundFIFO(void){audio_ready=1;}
void installSystemFIFO(void){system_ready=1;}
bool isDSiMode(void){return true;} void installCameraFIFO(void){}
void mmInstall(int x){assert(x==FIFO_MAXMOD&&audio_ready&&system_ready);}
void inputGetAndSend(void){inputs++;}
void fifoSetValue32Handler(int ch,void (*fn)(u32,void *),void *ctx){assert(ch==FIFO_USER_01&&!ctx);request=fn;}
void setPowerButtonCB(void (*fn)(void)){stop=fn;}
void initClockIRQTimer(int x){assert(x==LIBNDS_DEFAULT_TIMER_RTC);}
void irqSet(int x,void (*fn)(void)){assert(x==IRQ_VBLANK);tick=fn;}
void irqEnable(int x){assert(x==IRQ_VBLANK);}
void installWifiFIFO(void){assert(audio_ready&&system_ready);installs++;}
void Wifi_Update(void){assert(installs==1);updates++;}
void fifoSendValue32(int ch,u32 value){assert(ch==FIFO_USER_01&&value==RADIO_READY&&installs==1);acks++;}
void swiWaitForVBlank(void){
    tick();waits++;
    if(waits==1){assert(installs==0&&updates==0&&inputs==1);request(0,NULL);}
    if(waits==2){assert(installs==0&&updates==0);request(RADIO_START,NULL);}
    if(waits==3){assert(installs==1&&acks==1&&updates==1);request(RADIO_START,NULL);}
    if(waits==4){assert(installs==1&&acks==2&&updates==2);stop();}
    assert(waits<=4);
}
int main(void){assert(arm7_main()==0);assert(inputs==4);puts("PASS: ARM7 starts audio/input before radio, ignores unknown requests, acknowledges readiness and installs radio once");return 0;}
