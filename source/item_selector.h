#ifndef ITEM_SELECTOR_H
#define ITEM_SELECTOR_H
#include <stdint.h>
/* Stable inventory ids; more owned items do not add more touch boxes. */
enum {ITEM_NONE,ITEM_POTION,ITEM_SHOVEL,ITEM_BOW,ITEM_SANDWICH,ITEM_COUNT};
static inline unsigned item_selector_count(unsigned mask){unsigned n=0;for(unsigned i=1;i<ITEM_COUNT;i++)if(mask&(1u<<i))n++;return n;}
static inline unsigned item_selector_current(unsigned mask,unsigned current){if(current>0&&current<ITEM_COUNT&&(mask&(1u<<current)))return current;for(unsigned i=1;i<ITEM_COUNT;i++)if(mask&(1u<<i))return i;return 0;}
static inline unsigned item_selector_move(unsigned mask,unsigned current,int direction){current=item_selector_current(mask,current);if(!current)return 0;for(unsigned n=0;n<ITEM_COUNT;n++){current=direction<0?(current+ITEM_COUNT-1)%ITEM_COUNT:(current+1)%ITEM_COUNT;if(current&&(mask&(1u<<current)))return current;}return 0;}
/* -1 outside,0 equip,1 previous,2 next. */
static inline int item_selector_hit(int x,int y){if(y<118||y>=170)return -1;if(x>=64&&x<116)return 0;if(x>=30&&x<58)return 1;if(x>=122&&x<150)return 2;return -1;}
void item_selector_draw(uint16_t *pixels,unsigned mask,unsigned current,unsigned equipped,unsigned quantity);
#endif
