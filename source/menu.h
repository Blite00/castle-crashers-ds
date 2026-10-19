#ifndef CASTLE_MENU_H
#define CASTLE_MENU_H
#include <stdint.h>
/* Selected row is 0..2. Artwork uses the installed game's vignette and font. */
void menu_render_controls(uint16_t *pixels);
void menu_render(uint16_t *pixels, int selected);
void menu_render_multiplayer(uint16_t *pixels, int selected);
void menu_render_loading(uint16_t *pixels,unsigned percent);
void menu_text(uint16_t *pixels, int x, int y, const char *text, uint16_t color);
int menu_text_width(const char *text);
void menu_render_campaign(uint16_t *pixels,int selected);
#endif

