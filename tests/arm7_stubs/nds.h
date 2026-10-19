#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
typedef uint32_t u32;
#define FIFO_USER_01 8
#define FIFO_MAXMOD 3
#define LIBNDS_DEFAULT_TIMER_RTC 3
#define IRQ_VBLANK 1
#define KEY_SELECT 4
#define KEY_START 8
#define KEY_L 512
#define KEY_R 256
extern uint16_t REG_KEYINPUT;
void enableSound(void);void readUserSettings(void);void ledBlink(int);
void touchInit(void);void irqInit(void);void fifoInit(void);
void installSoundFIFO(void);void installSystemFIFO(void);bool isDSiMode(void);
void installCameraFIFO(void);void inputGetAndSend(void);
void fifoSetValue32Handler(int,void (*)(u32,void *),void *);
void setPowerButtonCB(void (*)(void));void initClockIRQTimer(int);
void irqSet(int,void (*)(void));void irqEnable(int);
void fifoSendValue32(int,u32);void swiWaitForVBlank(void);
