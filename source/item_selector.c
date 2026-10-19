#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "item_selector.h"
#include "ui_skin.h"
#include "menu.h"
#include "selector_art.h"
#include <stdio.h>
void item_selector_draw(uint16_t *p,unsigned mask,unsigned current,unsigned equipped,unsigned quantity){
 current=item_selector_current(mask,current);if(!current)return;
 ui_skin_tile(p,64,118,current==equipped);
 uint8_t decoded[SELECTOR_MAX_PIXELS];const SelectorFrame *f=&selector_frames[current-1];
 if(selector_decode(current-1,decoded))for(int y=0;y<f->h;y++)for(int x=0;x<f->w;x++){unsigned v=decoded[y*f->w+x];if(v)p[(123+y)*256+90-f->w/2+x]=selector_palette[v];}
 char label[16];const char *names[]={"","POTION","SHOVEL","BOW","FOOD"};
 if(current==ITEM_POTION||current==ITEM_SANDWICH)snprintf(label,sizeof label,"x%u",quantity);else snprintf(label,sizeof label,"%s",names[current]);
 menu_text(p,90-menu_text_width(label)/2,155,label,UI_INK);
 unsigned count=item_selector_count(mask);if(count>1){
  ui_skin_panel(p,30,129,28,24);ui_skin_panel(p,122,129,28,24);
  menu_text(p,40,135,"<",UI_INK);menu_text(p,133,135,">",UI_INK);
  unsigned page=0;for(unsigned i=1;i<=current;i++)if(mask&(1u<<i))page++;
  snprintf(label,sizeof label,"%u/%u",page,count);menu_text(p,34,157,label,UI_INK);
 }
}
