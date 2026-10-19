#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "barbarian_packed.h"
int main(void){uint8_t old[BARBARIAN_FRAME_PIXELS_MAX],now[BARBARIAN_FRAME_PIXELS_MAX+2];
 for(unsigned id=0;id<304;id++){
  const ArtFrame*f=&barbarian_frames[id];unsigned size=f->w*f->h,p=f->offset,n=0;
  while(n<size){unsigned t=barbarian_pixels[p++],count=(t&127)+1;assert(count<=size-n);if(t&128){memset(old+n,barbarian_pixels[p++],count);}else{memcpy(old+n,barbarian_pixels+p,count);p+=count;}n+=count;}
  memset(now,0xa5,sizeof(now));assert(barbarian_decode_frame(id,now+1));assert(!memcmp(old,now+1,size));assert(now[0]==0xa5);for(unsigned j=size+1;j<sizeof(now);j++)assert(now[j]==0xa5);
 }
 const unsigned hot[][2]={{13,29},{29,42},{57,63},{187,189},{245,265}};
 for(unsigned r=0;r<5;r++)for(unsigned id=hot[r][0];id<hot[r][1];id++)assert(!(barbarian_packed_lengths[barbarian_packed_ids[id]]&32768));
 assert(!barbarian_decode_frame(304,now));assert(!barbarian_decode_frame(0,NULL));puts("PASS: all304 Barbarian frames byte-identical with mixed RLE/Huffman and canaries");return 0;
}

