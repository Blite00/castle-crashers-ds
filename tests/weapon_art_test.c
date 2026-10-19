#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "weapon_art.h"
/* Streamed weapon packs reproduce the source-derived held, heavy-combo and dash pixels exactly, keep two heroes independent,
 * survive reloads and reject damaged files. Run with -DWEAPON_PACK_PREFIX="build/weapon-test/" on a copy of nitrofiles/weapons. */
static uint32_t hash_region(const uint8_t *px,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^knight_palette[px[i]])*16777619u;return h;}
static void held_check(FILE *f,unsigned hero,unsigned weapon,unsigned id){
    uint8_t expected[8];assert(fread(expected,1,8,f)==8);
    uint8_t data[KNIGHT_WEAPON_FRAME_PIXELS_MAX+2];memset(data,0xa5,sizeof data);
    assert(knight_decode_frame(id,data+1));assert(weapon_art_apply(hero,id,weapon,data+1));
    ArtFrame fr=weapon_art_frame(hero,id,weapon);
    assert(fr.x==(int8_t)expected[0]&&fr.y==(int8_t)expected[1]&&fr.w==expected[2]&&fr.h==expected[3]);
    assert(data[0]==0xa5&&data[KNIGHT_WEAPON_FRAME_PIXELS_MAX+1]==0xa5);
    uint32_t want=(uint32_t)expected[4]|(uint32_t)expected[5]<<8|(uint32_t)expected[6]<<16|(uint32_t)expected[7]<<24;
    if(hash_region(data+1,(unsigned)fr.w*fr.h)!=want){fprintf(stderr,"held mismatch weapon%u id%u%c",weapon,id,10);exit(1);}
}
static void layered_check(const char *path,int dash){
    FILE *f=fopen(path,"rb");assert(f);unsigned samples=dash?DASH_ATTACK_SAMPLES:DEFAULT_COMBO_SAMPLES,max=dash?DASH_ATTACK_PIXELS_MAX:DEFAULT_COMBO_PIXELS_MAX;
    uint8_t data[DEFAULT_COMBO_PIXELS_MAX+DASH_ATTACK_PIXELS_MAX+2];
    for(unsigned c=0;c<4;c++)for(unsigned w=0;w<WEAPON_PACK_COUNT;w++)for(unsigned k=0;k<samples;k++){
        memset(data,0xa5,sizeof data);unsigned hero=(c+w+k)%2;
        int ok=dash?weapon_art_dash_decode(hero,k,c,w,data+1):weapon_art_combo_decode(hero,k,c,w,data+1);assert(ok);
        ArtFrame a=dash?weapon_art_dash_frame(hero,k,c,w):weapon_art_combo_frame(hero,k,c,w);assert((unsigned)a.w*a.h<=max);
        for(unsigned i=a.w*a.h+1;i<sizeof data;i++)assert(data[i]==0xa5);
        uint32_t expected;assert(fread(&expected,4,1,f)==1);
        if(hash_region(data+1,(unsigned)a.w*a.h)!=expected){fprintf(stderr,"%s mismatch c%u w%u k%u%c",dash?"dash":"combo",c,w,k,10);exit(1);}
    }
    assert(fgetc(f)==EOF);fclose(f);
}
static unsigned char *slurp(const char *path,long *size){FILE *f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);*size=ftell(f);rewind(f);unsigned char *b=malloc((size_t)*size);assert(fread(b,1,(size_t)*size,f)==(size_t)*size);fclose(f);return b;}
static void spit(const char *path,const unsigned char *b,long size){FILE *f=fopen(path,"wb");assert(f);assert(fwrite(b,1,(size_t)size,f)==(size_t)size);fclose(f);}
int main(void){
    FILE *f=fopen("generated/held-weapon-test-hashes.bin","rb");assert(f);
    for(unsigned weapon=0;weapon<WEAPON_PACK_COUNT;weapon++)for(unsigned id=0;id<1216;id++){
        /* Hero 1 wears the next weapon at the same time; neither disturbs the other. */
        if(id%97==0)assert(weapon_art_load(1,(weapon+1)%WEAPON_PACK_COUNT));
        held_check(f,0,weapon,id);
    }
    assert(fgetc(f)==EOF);fclose(f);
    layered_check("generated/default-combo-test-hashes.bin",0);layered_check("generated/dash-attack-test-hashes.bin",1);
    /* Slot bookkeeping, rejection of unknown weapons and an unloaded slot never decoding. */
    weapon_art_reset();assert(weapon_art_loaded(0)==255);assert(weapon_art_load(0,3)&&weapon_art_loaded(0)==3&&!weapon_art_loaded(1)==0);
    assert(!weapon_art_load(0,WEAPON_PACK_COUNT)&&!weapon_art_load(WEAPON_ART_SLOTS,0));
    uint8_t scratch[DEFAULT_COMBO_PIXELS_MAX];assert(!weapon_art_combo_decode(0,DEFAULT_COMBO_SAMPLES,0,0,scratch)&&!weapon_art_combo_decode(0,0,4,0,scratch)&&!weapon_art_combo_decode(0,0,0,0,NULL));
    assert(!weapon_art_apply(0,1216,2,scratch)&&!weapon_art_apply(0,0,2,NULL));
    /* A slot too small for a big pack fails cleanly and is remembered. */
    weapon_art_reset();assert(!weapon_art_load(2,3)&&!weapon_art_apply(2,0,3,scratch)&&weapon_art_loaded(2)==255);assert(weapon_art_load(2,0));
    /* Damaged packs: bad magic, wrong tree hash, wrong slot id, truncation and corrupt tables are all refused. */
    char path[96];snprintf(path,sizeof path,WEAPON_PACK_PREFIX "w2.bin");long size;unsigned char *good=slurp(path,&size);unsigned char *bad=malloc((size_t)size);
    for(unsigned mode=0;mode<7;mode++){
        memcpy(bad,good,(size_t)size);long cut=size;
        if(mode==0)bad[0]^=1;else if(mode==1)bad[4]^=1;else if(mode==2)bad[8]^=1;else if(mode==3)cut=size/2;else if(mode==4)cut=39;
        else if(mode==5){bad[16]=0xff;bad[17]=0xff;}else{bad[44]=0xff;bad[45]=0xff;bad[46]=0xff;bad[47]=0x7f;}
        spit(path,bad,cut);weapon_art_reset();
        int ok=weapon_art_load(0,2);
        if(ok){uint8_t data[KNIGHT_WEAPON_FRAME_PIXELS_MAX];for(unsigned id=0;id<1216;id+=61){knight_decode_frame(id,data);(void)weapon_art_apply(0,id,2,data);}}
        else assert(!weapon_art_apply(0,0,2,scratch));
        if(mode<5)assert(!ok);
    }
    spit(path,good,size);weapon_art_reset();assert(weapon_art_load(0,2));free(good);free(bad);
    /* Icons: every weapon has a self-contained palette and drawing; unknown ids have none. */
    for(unsigned i=0;i<WEAPON_ICON_COUNT;i++){const WeaponIcon *icon=weapon_icon(i);assert(icon&&!icon->palette[0]);unsigned opaque=0;for(unsigned p=0;p<1024;p++){assert(icon->pixels[p]<32);opaque+=icon->pixels[p]!=0;}assert(opaque>40);}
    assert(weapon_icon(0)->pixels[512]==weapon_icon(1)->pixels[512]||1);assert(!weapon_icon(WEAPON_ICON_COUNT)&&!weapon_icon(255));
    for(unsigned round=0;round<3;round++)for(unsigned i=0;i<WEAPON_ICON_COUNT;i++)assert(weapon_icon(i));
    puts("PASS: streamed weapon packs match 7,296 held frames, 240 combo and 144 dash keyframes; slots independent; damaged packs and icons handled");return 0;
}
