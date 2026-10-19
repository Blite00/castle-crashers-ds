#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "pets.h"
#include <string.h>
#include <stdlib.h>
static const PetInfo snoot={"Snoot",4,0,0,0,0};
static const PetInfo buddy={"Mr Buddy",0,0,0,0,1};
static const PetInfo owlet={"Owlet",0,0,0,0,0};
static const PetInfo troll={"Troll",0,0,0,0,0};
static const PetInfo dino={"Dino",0,0,0,2,0};
const PetInfo *pets_info(unsigned id){return id==PET_SNOOT?&snoot:id==PET_MR_BUDDY?&buddy:id==PET_OWLET?&owlet:id==PET_TROLL?&troll:id==PET_DINO?&dino:0;}
int pets_owned(const PetProfile *p,unsigned id){return p&&pets_info(id)&&!!(p->owned&(UINT64_C(1)<<id));}
int pets_profile_valid(const PetProfile *p){return p&&!(p->owned&~PET_SUPPORTED_MASK)&&(!p->equipped||pets_owned(p,p->equipped));}
int pets_profile_encode(uint8_t out[9],const PetProfile *p){
    if(!out||!pets_profile_valid(p))return 0;
    for(unsigned i=0;i<8;i++)out[i]=(uint8_t)(p->owned>>(8*i));
    out[8]=p->equipped;return 1;
}
int pets_profile_decode(PetProfile *p,const uint8_t in[9]){
    if(!p||!in)return 0;
    PetProfile decoded={0};for(unsigned i=0;i<8;i++)decoded.owned|=(uint64_t)in[i]<<(8*i);decoded.equipped=in[8];
    if(!pets_profile_valid(&decoded))return 0;
    *p=decoded;return 1;
}
int pets_place(PetDrop *drop,unsigned id,int x,int y,int unlock){
    if(!drop||drop->source_id||!pets_info(id)||x<-32768||x>32767||y<-32768||y>32767)return 0;
    *drop=(PetDrop){.x=(int16_t)x,.y=(int16_t)y,.source_id=(uint8_t)id,.unlock=!!unlock};return 1;
}
static PetDrop released(const PetFollower *f,unsigned id){return (PetDrop){.x=f->x,.y=f->y,.source_id=(uint8_t)id,.ignore_timer=PET_IGNORE_TICKS,.z=18};}
int pets_pickup(PetProfile profiles[2],unsigned alive,unsigned hero,PetFollower *f,PetDrop *drop,int x,int y){
    if(!profiles||!f||!drop||hero>=2||!(alive&(1u<<hero))||!pets_info(drop->source_id)||drop->ignore_timer||drop->unlock>1||
       !pets_profile_valid(&profiles[0])||!pets_profile_valid(&profiles[1])||x<-32768||x>32767||y<-32768||y>32767||abs(x-drop->x)>=25||abs(y-drop->y)>=10)return 0;
    PetProfile *p=&profiles[hero];unsigned id=drop->source_id;
    if(!drop->unlock&&!pets_owned(p,id))return 0;
    if(drop->unlock)for(unsigned h=0;h<2;h++)if(alive&(1u<<h))profiles[h].owned|=UINT64_C(1)<<id;
    PetDrop old=p->equipped?released(f,p->equipped):(PetDrop){0};
    *f=(PetFollower){.x=drop->x,.y=drop->y,.love_timer=PET_LOVE_TICKS,.facing=1};
    p->equipped=(uint8_t)id;*drop=old;return 1;
}
int pets_equip_owned(PetProfile *p,PetFollower *f,unsigned id,PetDrop *drop,int x,int y){
    if(!p||!f||!pets_profile_valid(p)||(id&&!pets_owned(p,id))||x<-32753||x>32767||y<-32765||y>32767)return 0;
    if(id==p->equipped)return 1;
    if(p->equipped&&(!drop||drop->source_id))return 0;
    if(p->equipped)*drop=released(f,p->equipped);
    p->equipped=(uint8_t)id;
    *f=id?(PetFollower){.x=(int16_t)(x-15),.y=(int16_t)(y-2),.love_timer=PET_LOVE_TICKS,.facing=1}:(PetFollower){0};return 1;
}
void pets_drop_step(PetDrop *drop){
    if(!drop||!pets_info(drop->source_id))return;
    if(drop->ignore_timer)drop->ignore_timer--;
    if(drop->z<18)drop->z++;
}
static void follow_axis(int16_t *whole,int8_t *fraction,int goal,int tolerance){
    int current=(int)*whole*16+*fraction,delta=goal*16-current;
    if(abs(delta)>tolerance){current+=delta/10;*whole=(int16_t)(current/16);*fraction=(int8_t)(current%16);}
}
void pets_follow_step(PetFollower *f,int x,int y,int facing){
    if(!f)return;
    x=x<-32768?-32768:x>32767?32767:x;y=y<-32768?-32768:y>32767?32767:y;
    int goal=x+(facing<0?25:-25),before=f->x;
    goal=goal<-32768?-32768:goal>32767?32767:goal;
    follow_axis(&f->x,&f->fraction_x,goal,32);follow_axis(&f->y,&f->fraction_y,y,24);
    if(f->x!=before)f->facing=f->x>before?1:-1;
    if(f->love_timer)f->love_timer--;
}
int pets_unearth_buddy(PetDrop *drop,int shovel,int finished,int x,int y){return shovel&&finished&&pets_place(drop,PET_MR_BUDDY,x,y,1);}
