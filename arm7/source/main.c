/* Castle Arena ARM7: audio/input first, radio service on explicit request.
 * The initialization sequence follows BlocksDS's standard ARM7 core.
 */
#include <nds.h>
#include <maxmod7.h>
#include <dswifi7.h>
#define RADIO_START 0x43415201u
#define RADIO_READY 0x43415202u
static volatile bool quitting,radio_requested,radio_ready;
static void power_button(void){quitting=true;}
static void request_radio(u32 value,void *context){
    (void)context;
    if(value==RADIO_START)radio_requested=true;
}
static void vblank(void){
    inputGetAndSend();
    if(radio_ready)Wifi_Update();
}
int main(void){
    enableSound();readUserSettings();ledBlink(0);touchInit();
    irqInit();fifoInit();installSoundFIFO();installSystemFIFO();
    if(isDSiMode())installCameraFIFO();
    mmInstall(FIFO_MAXMOD);
    fifoSetValue32Handler(FIFO_USER_01,request_radio,NULL);
    setPowerButtonCB(power_button);
    initClockIRQTimer(LIBNDS_DEFAULT_TIMER_RTC);
    irqSet(IRQ_VBLANK,vblank);irqEnable(IRQ_VBLANK);
    while(!quitting){
        if(radio_requested){
            if(!radio_ready){installWifiFIFO();radio_ready=true;}
            radio_requested=false;
            fifoSendValue32(FIFO_USER_01,RADIO_READY);
        }
        unsigned keys=~REG_KEYINPUT;
        if((keys&(KEY_SELECT|KEY_START|KEY_L|KEY_R))==(KEY_SELECT|KEY_START|KEY_L|KEY_R))quitting=true;
        swiWaitForVBlank();
    }
    return 0;
}
