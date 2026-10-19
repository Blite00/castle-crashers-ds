#include "effects.h"
#include "magic_fx.h"
#include <string.h>
#include "item_art.h"
static int equip_frame(const Fighter *p){int age=30-p->equip_timer;return 1+(age<8?age:7);}
static void equip_world(uint16_t *pixels,const Fighter *p,int width,int height){
    if(!p->equip_timer||!p->bow_equipped||p->hp<=0)return;
    const ItemFrame *f=&item_frames[equip_frame(p)];
    for(int y=0;y<f->h;y++)for(int x=0;x<f->w;x++){
        unsigned c=item_pixels[f->offset+y*f->w+x];
        int dx=p->x+f->x+x,dy=p->y-p->z-63+f->y+y;
        if(c&&dx>=0&&dx<width&&dy>=0&&dy<height)pixels[dy*width+dx]=item_palette[c];
    }
}
static int arrow_frame(const Projectile *p){
    int v=p->vz;return v>=3?0:v>=1?1:v>=-2?2:v>=-5?3:v>=-9?4:v>=-13?5:v>=-19?6:7;
}
/* Original FX clips, run-length packed in generated/magic_fx.h and decoded on demand into a small cache. */
#define MAGIC_SLOTS 3
static uint8_t magic_cache[MAGIC_SLOTS][MAGIC_FRAME_PIXELS_MAX];
static int magic_cached[MAGIC_SLOTS]={-1,-1,-1};
static unsigned magic_next;
static int magic_id(int group,unsigned n){const MagicGroup *g=&magic_groups[group];return g->first+(int)(n%g->count);}
static const uint8_t *magic_pixels_for(int id){
    for(int i=0;i<MAGIC_SLOTS;i++)if(magic_cached[i]==id)return magic_cache[i];
    unsigned slot=magic_next++%MAGIC_SLOTS;magic_cached[slot]=-1;
    if(!magic_decode((unsigned)id,magic_cache[slot]))return 0;
    magic_cached[slot]=id;return magic_cache[slot];
}
static int ready_group(unsigned id){static const int groups[]={MFX_READY_ICE,MFX_READY_FIRE,MFX_READY_LIGHTNING,MFX_READY_POISON};return groups[id<4?id:0];}
static int splash_group(unsigned id){static const int groups[]={MFX_SPLASH_ICE,MFX_SPLASH_FIRE,MFX_SPLASH_LIGHTNING,MFX_SPLASH_POISON};return groups[id<4?id:0];}
static int bolt_group(int kind,int down){
    int g=kind==SHOT_LIGHTNING?MFX_BOLT_LIGHTNING:kind==SHOT_POISON?MFX_BOLT_POISON:kind==SHOT_FIRE?MFX_BOLT_FIRE:MFX_BOLT_ICE;
    return down?g+(MFX_BOLT_POISON_DOWN-MFX_BOLT_POISON):g;
}
static int impact_group(int kind){return kind==SHOT_LIGHTNING?MFX_IMPACT_LIGHTNING:kind==SHOT_POISON?MFX_IMPACT_POISON:kind==SHOT_FIRE?MFX_IMPACT_FIRE:MFX_IMPACT_ICE;}
/* Frame id of a projectile: arrows by pitch, bolts by clock. */
static int shot_frame(const Arena *a,const Projectile *p){
    if(p->kind==SHOT_ARROW){
        int orange=p->owner>=0&&p->owner<2&&a->p[p->owner].knight_id==KNIGHT_ORANGE;
        return magic_id(orange?MFX_ARROW_ORANGE:MFX_ARROW_BLUE,(unsigned)arrow_frame(p));
    }
    return magic_id(bolt_group(p->kind,p->vz<0),(unsigned)(a->tick/2));
}
/* The source clip fades out and stops. For a held-button indicator, sustain
   its visible middle poses instead of repeatedly restarting that fade. */
static unsigned ready_frame(uint32_t tick){unsigned phase=(tick/3)%6;return phase<4?phase:6-phase;}

/* Spell hits: a projectile that vanished since the previous tick leaves its original impact burst where it ended (client side only). */
#define IMPACTS 8
#define IMPACT_TICKS 12
typedef struct {int16_t x,y;uint8_t kind,age,facing;} Impact;
static Impact impacts[IMPACTS];
static Projectile last_shots[ARENA_SHOTS];
static uint32_t last_tick;
static uint8_t last_seen;
static void impacts_track(const Arena *arena){
    if(last_seen&&arena->tick==last_tick)return;
    int step=(int)(arena->tick-last_tick),smooth=last_seen&&step>0&&step<=2;
    for(int i=0;i<IMPACTS;i++)if(impacts[i].age){
        int age=impacts[i].age+(smooth?step:IMPACT_TICKS);
        impacts[i].age=(uint8_t)(age>IMPACT_TICKS?0:age);
    }
    if(smooth)for(int s=0;s<ARENA_SHOTS;s++){
        const Projectile *o=&last_shots[s];
        if(o->life<=1||o->kind==SHOT_ARROW||arena->shots[s].life>0)continue;
        for(int i=0;i<IMPACTS;i++)if(!impacts[i].age){impacts[i]=(Impact){(int16_t)o->x,(int16_t)o->y,(uint8_t)o->kind,1,(uint8_t)(o->vx<0)};break;}
    }
    memcpy(last_shots,arena->shots,sizeof last_shots);last_tick=arena->tick;last_seen=1;
}
static int impact_frame_id(const Impact *m){const MagicGroup *g=&magic_groups[impact_group(m->kind)];return g->first+(m->age-1)*g->count/IMPACT_TICKS;}
static int status_id(const Fighter *p,int *yoff){
    if(p->freeze>0){int age=30-p->freeze;*yoff=-22;return age<8?magic_id(MFX_IMPACT_ICE,(unsigned)age*5/8):magic_id(MFX_FROZEN,(unsigned)(age-8)/2);}
    if(p->burn>ARENA_BURN_TICKS-14){*yoff=-20;return magic_id(MFX_BURN,(unsigned)(ARENA_BURN_TICKS-p->burn)*8/14);}
    return -1;
}

static void draw(uint16_t *pixels,int id,int x,int y,int facing,int width,int height) {
    const uint8_t *src=magic_pixels_for(id);if(!src)return;
    const MagicFrame *f=&magic_frames[id];
    for(int yy=0;yy<f->h;yy++)for(int xx=0;xx<f->w;xx++) {
        unsigned c=src[yy*f->w+xx];
        if(!c)continue;
        int dx=x+(facing<0?-(f->x+xx):f->x+xx),dy=y+f->y+yy;
        if(dx>=0&&dx<width&&dy>=0&&dy<height)pixels[dy*width+dx]=magic_palette[c];
    }
}
static void view_effect(RasterView *view,int id,int x,int y,int facing,int depth){
    const uint8_t *src=magic_pixels_for(id);if(!src)return;
    const MagicFrame *f=&magic_frames[id];
    int left=x+(facing<0?-(f->x+f->w-1):f->x);
    raster_sprite(view,src,magic_palette,left,y+f->y,f->w,f->h,facing<0,16,f->h,depth);
}
static int splash_id(const Fighter *p){
    int age=12-p->cooldown;
    if(p->attack_kind!=HIT_SPLASH||p->cooldown<=0||age<6||age>11)return -1;
    return magic_id(splash_group(p->knight_id),(unsigned)(age-6)*magic_groups[splash_group(p->knight_id)].count/6);
}

void effects_render_depth(uint16_t *pixels,const Arena *arena,int width,int height,int low,int high) {
    impacts_track(arena);
    for(int i=0;i<ARENA_SHOTS;i++) {
        const Projectile *p=&arena->shots[i];
        if(arena->map_id==6&&p->owner==8)continue;
        if(p->life>0&&(p->owner>=0&&p->owner<(arena->map_id>=3?9:2)&&(p->owner>=2||arena->p[p->owner].ready))&&p->y>=low&&p->y<high)draw(pixels,shot_frame(arena,p),p->x,p->y-p->z,p->vx<0?-1:1,width,height);
    }
    for(int i=0;i<IMPACTS;i++)if(impacts[i].age&&impacts[i].y>=low&&impacts[i].y<high)draw(pixels,impact_frame_id(&impacts[i]),impacts[i].x,impacts[i].y-14,impacts[i].facing?-1:1,width,height);
    for(int i=0;i<2;i++) {
        const Fighter *p=&arena->p[i];
        if(!p->ready||p->y<low||p->y>=high)continue;
        equip_world(pixels,p,width,height);
        if((p->previous&MAGIC)&&p->hp>0&&!arena->clash_timer)
            draw(pixels,magic_id(ready_group(p->knight_id),ready_frame(arena->tick)),p->x,p->y-p->z,p->facing,width,height);
        int sid=splash_id(p);if(sid>=0)draw(pixels,sid,p->x+p->facing*40,p->y,p->facing,width,height);
        int yoff,id=status_id(p,&yoff);
        if(id>=0)draw(pixels,id,p->x,p->y-p->z+yoff,1,width,height);
    }
}

void effects_render(uint16_t *pixels,const Arena *arena){effects_render_world(pixels,arena,256,192);}

void effects_render_world(uint16_t *pixels,const Arena *arena,int width,int height){effects_render_depth(pixels,arena,width,height,-32768,32767);}

void effects_ready_view(RasterView *view,const Arena *arena,int player){
    const Fighter *p=&arena->p[player];
    if(!p->ready||!(p->previous&MAGIC)||p->hp<=0||arena->clash_timer)return;
    view_effect(view,magic_id(ready_group(p->knight_id),ready_frame(arena->tick)),p->x,p->y-p->z,p->facing,p->y-3);
}
uint8_t effects_levelup[2];
/* Source PLAYER clip30/31: a 60-tick flickering white light pillar over the knight, with the FX 'levelup' star burst
   (FX sprite1292) rising from a ring at the feet. Drawn as flat rects so it costs a few dozen fills. */
static void levelup_view(RasterView *view,const Fighter *p,unsigned left){
    unsigned age=60-left,flick=(age/2)&1;int x=p->x,y=p->y-p->z,depth=p->y+1;
    int rise=(int)(age<12?age*3:36);
    uint16_t pale=0x8000|(28)|(28<<5)|(31<<10),bright=0x8000|31|(31<<5)|(31<<10),gold=0x8000|31|(29<<5)|(14<<10);
    if(left>6){ /* a slim flickering pillar over the knight, as in the source clip but sized for the DS picture */
        for(int i=0;i<5;i++){int h=rise+(i==2?10:i==1||i==3?4:0);
            if(((i&1)^flick)||i==2)raster_rect(view,x-5+i*2,y-h-14,2,h+14,i==2?bright:pale,depth);}
    }
    int rw=6+(int)(age<20?age:20)/2;
    raster_rect(view,x-rw,y-1,2*rw,1,bright,depth);raster_rect(view,x-rw+2,y+1,2*rw-4,1,pale,depth);
    for(unsigned k=0;k<8;k++){
        if(age<k*3||left<=k)continue;
        int t=(int)(age-k*3),dir=k&1?1:-1,sx=x+dir*(2+(int)((k*7)%6)+t/4),sy=y-6-t+t*t/80-(int)((k*5)%5);
        raster_rect(view,sx,sy,k%3?2:1,k%3?2:1,(k%3)?bright:gold,depth);
    }
}
void effects_render_view(RasterView *view,const Arena *arena,int low,int high){
    impacts_track(arena);
    for(int i=0;i<ARENA_SHOTS;i++){
        const Projectile *p=&arena->shots[i];
        if(arena->map_id==6&&p->owner==8)continue;
        if(p->life>0&&(p->owner>=0&&p->owner<(arena->map_id>=3?9:2)&&(p->owner>=2||arena->p[p->owner].ready))&&p->y>=low&&p->y<high)view_effect(view,shot_frame(arena,p),p->x,p->y-p->z,p->vx<0?-1:1,p->y);
    }
    for(int i=0;i<IMPACTS;i++)if(impacts[i].age&&impacts[i].y>=low&&impacts[i].y<high)view_effect(view,impact_frame_id(&impacts[i]),impacts[i].x,impacts[i].y-14,impacts[i].facing?-1:1,impacts[i].y);
    for(int i=0;i<2;i++){
        const Fighter *p=&arena->p[i];if(!p->ready||p->y<low||p->y>=high)continue;
        if(p->equip_timer&&p->bow_equipped&&p->hp>0){
            const ItemFrame *f=&item_frames[equip_frame(p)];
            raster_sprite(view,item_pixels+f->offset,item_palette,p->x+f->x,p->y-p->z-63+f->y,
                f->w,f->h,0,16,f->h,p->y);
        }
        if(effects_levelup[i])levelup_view(view,p,effects_levelup[i]);
        int sid=splash_id(p);if(sid>=0)view_effect(view,sid,p->x+p->facing*40,p->y,p->facing,p->y);
        int yoff,id=status_id(p,&yoff);
        if(id>=0)view_effect(view,id,p->x,p->y-p->z+yoff,1,p->y);
    }
}

void effects_status_view(RasterView *view,const Fighter *p){
    if(!p||!p->ready)return;
    int yoff,id=status_id(p,&yoff);
    if(id>=0)view_effect(view,id,p->x,p->y-p->z+yoff,1,p->y);
}
