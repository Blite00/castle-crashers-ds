#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "menu.h"
#include "menu_art.h"
#define RGB(r,g,b) (0x8000u|(r)|((g)<<5)|((b)<<10))
static void label(uint16_t *pixels,int id,int x,int y,uint16_t color){menu_draw_alpha(pixels,(unsigned)id,0,x,y,color);}
static void render(uint16_t *pixels,int selected,int multiplayer){
    const uint16_t white=RGB(31,31,31),gold=RGB(31,27,10);
    if(selected<0||selected>(multiplayer?2:3))selected=0;
    menu_draw_background(pixels);
    label(pixels,multiplayer?7:6,13,63,white);
    if(multiplayer)for(int i=0;i<3;i++)label(pixels,i+3,24,82+i*26,i==selected?gold:white);
    else {
        static const char *options[]={"Single Player","Practice","Multiplayer","Controls"};
        for(int i=0;i<4;i++)menu_text(pixels,24,82+i*21,options[i],i==selected?gold:white);
    }
    int cy=87+selected*(multiplayer?26:21);
    for(int dx=0;dx<6;dx++)for(int dy=-3+dx/2;dy<=3-dx/2;dy++)pixels[(cy+dy)*256+12+dx]=gold;
    label(pixels,8,13,174,white);
    if(multiplayer)label(pixels,9,176,174,white);
}
void menu_render(uint16_t *pixels,int selected){render(pixels,selected,0);}
/* Loading screen of the upper display: the title vignette with a progress bar while a level's art is read from the card. */
void menu_render_loading(uint16_t *pixels,unsigned percent){
    const uint16_t white=RGB(31,31,31),gold=RGB(31,27,10),dark=RGB(3,4,6),track=RGB(9,8,6);
    if(percent>100)percent=100;
    menu_draw_background(pixels);
    for(int y=96;y<136;y++)for(int x=0;x<256;x++){uint16_t c=pixels[y*256+x];pixels[y*256+x]=0x8000u|((c&31)>>1)|((((c>>5)&31)>>1)<<5)|((((c>>10)&31)>>1)<<10);} /* darken a band behind the text */
    const char *title="LOADING";menu_text(pixels,(256-menu_text_width(title))/2,98,title,white);
    for(int y=118;y<130;y++)for(int x=28;x<228;x++)pixels[y*256+x]=(y==118||y==129||x==28||x==227)?dark:track;
    for(int y=120;y<128;y++)for(int x=30;x<30+(int)(196*percent/100);x++)pixels[y*256+x]=gold;
    char text[8];text[0]=percent>=100?'1':(char)('0'+percent/100);int n=0;
    if(percent>=100){text[n++]='1';text[n++]='0';text[n++]='0';}else{if(percent>=10)text[n++]=(char)('0'+percent/10);text[n++]=(char)('0'+percent%10);}
    text[n++]='%';text[n]=0;
    menu_text(pixels,(256-menu_text_width(text))/2,137,text,white);
}
void menu_render_multiplayer(uint16_t *pixels,int selected){render(pixels,selected,1);}
void menu_text(uint16_t *pixels,int x,int y,const char *text,uint16_t color){
    int start=x;
    for(;*text;text++){
        unsigned ch=(unsigned char)*text;
        if(ch=='\n'){x=start;y+=15;continue;}
        if(ch<32||ch>126)ch='?';
        const MenuLabel *s=&menu_glyphs[ch-32];
        menu_draw_alpha(pixels,ch-32,1,x,y,color);
        x+=s->w;
    }
}
int menu_text_width(const char *text){
    int width=0;
    for(;*text&&*text!='\n';text++){
        unsigned ch=(unsigned char)*text;if(ch<32||ch>126)ch='?';
        width+=menu_glyphs[ch-32].w;
    }
    return width;
}

void menu_render_controls(uint16_t *pixels){
    menu_draw_background(pixels);
    for(int y=60;y<192;y++)for(int x=0;x<256;x++)pixels[y*256+x]=RGB(4,6,9);
    menu_text(pixels,10,64,"CONTROLS",RGB(31,27,10));
    menu_text(pixels,10,87,"Downed: B + Left/Right rolls",RGB(29,29,28));
    menu_text(pixels,10,104,"B then B: Air spin (Lv2)",RGB(29,29,28));
    menu_text(pixels,10,121,"Close X: Throw; Clash: A/X/B",RGB(29,29,28));
    menu_text(pixels,10,138,"START: Pause / start / rematch",RGB(29,29,28));
    menu_text(pixels,10,155,"SELECT: Leave / main menu",RGB(29,29,28));
    menu_text(pixels,10,174,"A / B: Back",RGB(31,27,10));
}

void menu_render_campaign(uint16_t *pixels,int selected){
    menu_draw_background(pixels);
    menu_text(pixels,13,63,"SINGLE PLAYER",RGB(31,31,31));
    static const char *options[]={"Solo","Host Campaign","Join Campaign"};
    for(int i=0;i<3;i++)menu_text(pixels,24,86+i*26,options[i],i==selected?RGB(31,27,10):RGB(31,31,31));
    menu_text(pixels,13,174,"A select   B back",RGB(31,31,31));
}
