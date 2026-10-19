#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ui_icon_art.h"
int main(void){
 FILE*f=fopen("generated/ui-icon-test-hashes.bin","rb");assert(f);uint8_t out[UI_ICON_PIXELS+2];assert(sizeof(ui_icon_data)+sizeof(ui_icon_palette)==544);
 for(unsigned icon=0;icon<UI_ICON_COUNT;icon++){
  memset(out,0xa5,sizeof(out));assert(ui_icon_decode(icon,out+1));assert(out[0]==0xa5&&out[UI_ICON_PIXELS+1]==0xa5);uint32_t expected,hash=2166136261u;assert(fread(&expected,4,1,f)==1);unsigned visible=0;
  for(unsigned i=0;i<UI_ICON_PIXELS;i++){assert(out[i+1]<16);visible+=out[i+1]!=0;hash=(hash^ui_icon_palette[out[i+1]])*16777619u;}assert(visible>20);assert(hash==expected);
 }
 assert(fgetc(f)==EOF);fclose(f);assert(!ui_icon_decode(UI_ICON_COUNT,out));assert(!ui_icon_decode(0,NULL));puts("PASS:4 original attribute icons, source RGB555 hashes,544B bank and256B decode guards");return 0;
}
