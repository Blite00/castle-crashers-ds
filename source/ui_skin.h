#ifndef UI_SKIN_H
#define UI_SKIN_H
#include <stdint.h>
#define UI_RGB(r,g,b) (0x8000u|(r)|((g)<<5)|((b)<<10))
#define UI_INK UI_RGB(5,4,2)
#define UI_PAPER UI_RGB(28,25,18)
#define UI_LIGHT UI_RGB(31,29,23)
#define UI_GOLD UI_RGB(29,21,5)
void ui_skin_background(uint16_t *pixels);
void ui_skin_box(uint16_t *pixels,int x,int y,int w,int h,uint16_t color);
void ui_skin_panel(uint16_t *pixels,int x,int y,int w,int h);
void ui_skin_tile(uint16_t *pixels,int x,int y,int selected);
void ui_skin_center(uint16_t *pixels,int y,const char *text,uint16_t color);
void ui_skin_text(uint16_t *pixels,int x,int y,int width,const char *text,uint16_t color);
#endif
