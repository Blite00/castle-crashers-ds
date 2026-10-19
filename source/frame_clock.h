#ifndef FRAME_CLOCK_H
#define FRAME_CLOCK_H
#include <stdint.h>
/* One simulation/render step per two physical display refreshes. After an
   overrun resume from this refresh, never alternate catch-up and idle frames. */
static inline int frame_clock_due(uint32_t now,uint32_t *last){
    if((uint32_t)(now-*last)<2)return 0;
    *last=now;return 1;
}
static inline int frame_clock_present_due(uint32_t now,uint32_t target){
    return (int32_t)(now-target)>=0;
}
#endif
