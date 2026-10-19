#include "frame_clock.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    uint32_t last=0;unsigned count=0;
    for(uint32_t now=1;now<=36000;now++){int due=frame_clock_due(now,&last);assert(due==(now%2==0));count+=due;}
    assert(count==18000);
    assert(frame_clock_due(36005,&last));assert(!frame_clock_due(36006,&last));assert(frame_clock_due(36007,&last));
    last=0xfffffffeu;assert(!frame_clock_due(0xffffffffu,&last));assert(frame_clock_due(0,&last));
    /* Fast and medium-cost renders share the same two-refresh deadline. */
    assert(!frame_clock_present_due(101,102));assert(frame_clock_present_due(102,102));
    assert(frame_clock_present_due(103,102)); /* an overrun is never held longer */
    assert(!frame_clock_present_due(0xffffffffu,0));assert(frame_clock_present_due(0,0));
    puts("PASS: refresh-locked cadence, overrun recovery and counter wrap");
}
