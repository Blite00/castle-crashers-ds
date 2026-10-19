#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "ui_skin.h"
#include "menu.h"
#include <string.h>
void ui_skin_box(uint16_t*p,int x,int y,int w,int h,uint16_t c){
 int x1=x+w,y1=y+h;if(x<0)x=0;if(y<0)y=0;if(x1>256)x1=256;if(y1>192)y1=192;
 for(int yy=y;yy<y1;yy++)for(int xx=x;xx<x1;xx++)p[yy*256+xx]=c;
}
void ui_skin_background(uint16_t*p){
 for(int y=0;y<192;y++)for(int x=0;x<256;x++){
  unsigned grain=((unsigned)x*13u+(unsigned)y*7u)&31u;
  p[y*256+x]=y<59?UI_RGB(5,4,3):y>=175?UI_RGB(5,4,3):grain==0?UI_RGB(26,23,16):UI_PAPER;
 }
 ui_skin_box(p,0,59,256,2,UI_INK);ui_skin_box(p,0,61,256,1,UI_GOLD);
 ui_skin_box(p,0,174,256,1,UI_GOLD);
}
void ui_skin_panel(uint16_t*p,int x,int y,int w,int h){
 ui_skin_box(p,x+2,y,w-4,h,UI_INK);ui_skin_box(p,x,y+2,w,h-4,UI_INK);
 ui_skin_box(p,x+2,y+2,w-4,h-4,UI_LIGHT);
 ui_skin_box(p,x+3,y+3,w-6,1,UI_RGB(31,30,26));
 ui_skin_box(p,x+3,y+h-4,w-6,1,UI_RGB(23,19,12));
}
void ui_skin_tile(uint16_t*p,int x,int y,int selected){
 ui_skin_panel(p,x,y,52,52);
 if(selected){ui_skin_box(p,x+2,y+2,48,2,UI_GOLD);ui_skin_box(p,x+2,y+48,48,2,UI_GOLD);ui_skin_box(p,x+2,y+4,2,44,UI_GOLD);ui_skin_box(p,x+48,y+4,2,44,UI_GOLD);}
}
void ui_skin_center(uint16_t*p,int y,const char*s,uint16_t c){int x=(256-menu_text_width(s))/2;if(x<4)x=4;menu_text(p,x,y,s,c);}
void ui_skin_text(uint16_t*p,int x,int y,int width,const char*s,uint16_t c){
 char out[64];size_t n=strlen(s);if(n>=sizeof(out))n=sizeof(out)-1;memcpy(out,s,n);out[n]=0;
 if(menu_text_width(out)>width){while(n&&menu_text_width(out)+menu_text_width("...")>width)out[--n]=0;if(n+3<sizeof(out))strcat(out,"...");}
 menu_text(p,x,y,out,c);
}
