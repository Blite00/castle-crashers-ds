#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "menu_art.h"
#include "barracks_doorframe.h"
static uint16_t a[49154],b[49154];
static unsigned char raw[14402];
static unsigned off[105];
int main(void){
 FILE*f=fopen("build/menu_alpha-raw.bin","rb");assert(f);assert(fread(raw,1,6072,f)==6072);fclose(f);f=fopen("build/menu_font_alpha-raw.bin","rb");assert(f);assert(fread(raw+6072,1,8330,f)==8330);fclose(f);
 unsigned p=0;for(unsigned i=0;i<105;i++){const MenuLabel*m=i<10?menu_labels+i:menu_glyphs+i-10;off[i]=p;p+=m->w*m->h;}assert(p==14402);
 for(unsigned id=0;id<105;id++)for(int x=-12;x<270;x+=47)for(int y=-6;y<210;y+=53){
  for(unsigned i=0;i<49154;i++)a[i]=b[i]=(uint16_t)(i*97+123);
  const MenuLabel*m=id<10?menu_labels+id:menu_glyphs+id-10;uint16_t fg=0xed53;
  for(unsigned j=0;j<m->h;j++)for(unsigned i=0;i<m->w;i++){
   int px=x+(int)i,py=y+(int)j;unsigned alpha=raw[off[id]+j*m->w+i];
   if((unsigned)px>=256||(unsigned)py>=192||!alpha)continue;
   unsigned bg=b[1+py*256+px];unsigned r=((bg&31)*(255-alpha)+(fg&31)*alpha+127)/255,g=(((bg>>5)&31)*(255-alpha)+((fg>>5)&31)*alpha+127)/255,bb=(((bg>>10)&31)*(255-alpha)+((fg>>10)&31)*alpha+127)/255;
   b[1+py*256+px]=(uint16_t)(0x8000|r|(g<<5)|(bb<<10));
  }
  menu_draw_alpha(a+1,id<10?id:id-10,id>=10,x,y,fg);assert(!memcmp(a,b,sizeof(a)));
 }
 assert(!barracks_doorframe_pixel(1088,130));assert(!barracks_doorframe_pixel(-1,-1));assert(barracks_doorwall_pixel(1127,100));puts("3150 menu coverage renders exact; source doorframe opening transparent; guards intact");
}
