#ifndef OVERLAY_COPY_H
#define OVERLAY_COPY_H
typedef struct {unsigned first,end;} OverlayCopy;
/* Coalesce nearby dirty rows, trading at most256 zero/gap bytes for one less
   cache flush and synchronous DMA setup. Returns the completed previous run. */
static inline OverlayCopy overlay_copy_add(OverlayCopy *run,unsigned first,unsigned end){
    OverlayCopy done={0,0};
    if(end<=first)return done;
    if(run->end&&first-run->end>256){done=*run;*run=(OverlayCopy){first,end};}
    else{if(!run->end)run->first=first;run->end=end;}
    return done;
}
#endif
