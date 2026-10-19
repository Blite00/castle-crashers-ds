#include <stdio.h>
#include <stdint.h>
#include "menu.h"
int main(int argc,char **argv){
    uint16_t pixels[256*192];
    if(argc<2)return 1;
    if(argc>2)menu_render_controls(pixels);else menu_render(pixels,2);
    FILE *f=fopen(argv[1],"wb");if(!f)return 2;
    fprintf(f,"P6\n256 192\n255\n");
    for(int i=0;i<256*192;i++){
        unsigned p=pixels[i];
        fputc((p&31)*255/31,f);fputc(((p>>5)&31)*255/31,f);fputc(((p>>10)&31)*255/31,f);
    }
    fclose(f);return 0;
}
