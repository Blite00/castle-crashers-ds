#include "hud.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
 if(argc!=3)return 2;unsigned mode=(unsigned)atoi(argv[2]);uint16_t p[256*192];
 for(unsigned i=0;i<256*192;i++)p[i]=0xA4C4;
 Arena a={0};a.map_id=3;a.p[0]=(Fighter){.ready=1,.hp=85,.mana=30,.knight_id=2};a.p[1]=(Fighter){.ready=1,.hp=60,.mana=20,.knight_id=0};
 hud_render(p,&a);hud_render_progress(p,0,2,240);hud_render_progress(p,1,2,260);
 hud_render_boss(p,225,450);hud_render_potions(p,2,1);
 hud_render_level_notice(p,mode==2?4:2,mode==2?"Run + A / X":"B, B",mode?60:140);
 FILE*f=fopen(argv[1],"wb");if(!f)return 1;fprintf(f,"P6\n256 192\n255\n");
 for(unsigned i=0;i<256*192;i++){fputc((p[i]&31)*255/31,f);fputc(((p[i]>>5)&31)*255/31,f);fputc(((p[i]>>10)&31)*255/31,f);}return fclose(f)!=0;
}
