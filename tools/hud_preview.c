#include <stdio.h>
#include <stdint.h>
#include "hud.h"
#include "arena_background.h"
int main(void){uint16_t pixels[256*192];Arena arena={0};
for(int i=0;i<256*192;i++)pixels[i]=arena_palette[arena_pixels[i]];
arena.p[0].hp=ARENA_HEALTH;arena.p[0].mana=ARENA_MANA;arena.p[1].hp=ARENA_HEALTH/2;arena.p[1].mana=ARENA_MANA/2;arena.wins[1]=2;
hud_render(pixels,&arena);FILE *f=fopen("build/hud-preview.ppm","wb");if(!f)return 1;fprintf(f,"P6\n256 192\n255\n");
for(int i=0;i<256*192;i++){unsigned p=pixels[i];fputc((p&31)*255/31,f);fputc(((p>>5)&31)*255/31,f);fputc(((p>>10)&31)*255/31,f);}fclose(f);return 0;}
