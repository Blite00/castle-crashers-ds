#include "stage.h"
#include "../generated/war_geometry.h"
#include "../generated/boss_geometry.h"
#include "../generated/blacksmith_geometry.h"
#include "../generated/ark_geometry.h"
#include "../generated/frog_geometry.h"
#include "../generated/bridge_geometry.h"
#include "../generated/forest_geometry.h"
#include "../generated/thieves_geometry.h"
#include "../generated/deer_geometry.h"
#include "../generated/flood_geometry.h"
#include "../generated/arena_collision.h"
#include "../generated/campaign_world_geometry.h"
#include "../generated/campaign_shop_geometry.h"
#include "../generated/campaign_barracks_geometry.h"
#include <stdint.h>
#include <stdlib.h>
typedef struct {int min_x,min_y,max_x,max_y;} StageBounds;
static StageBounds bounds[STAGE_POLYGON_COUNT];
static volatile int bounds_ready;
static unsigned active_map;
void stage_set_map(unsigned map){active_map=map<STAGE_MAP_COUNT?map:0;}
const char *stage_name(unsigned map){
    static const char *names[]={"Castle Arena","King's Arena","Volcano Arena","Home Castle","Castle Keep","Barracks","Barbarian War","Barbarian Bridge","Blacksmith","Animal Ark","Weapon Frog","Barbarian Boss","Forest Entrance","Thieves' Forest","Mill Run","Flooded Castle"};
    return names[map<STAGE_MAP_COUNT?map:0];
}
static void prepare_bounds(void){
    if(bounds_ready)return;
    for(int polygon=0;polygon<STAGE_POLYGON_COUNT;polygon++){
        const StagePolygon *shape=&stage_polygons[polygon];
        StagePoint first=stage_points[shape->start];
        StageBounds b={first.x,first.y,first.x,first.y};
        for(int i=1;i<shape->count;i++){
            StagePoint p=stage_points[shape->start+i];
            if(p.x<b.min_x)b.min_x=p.x;
            if(p.y<b.min_y)b.min_y=p.y;
            if(p.x>b.max_x)b.max_x=p.x;
            if(p.y>b.max_y)b.max_y=p.y;
        }
        bounds[polygon]=b;
    }
    /* Publish only once every entry is initialized. An interrupt that arrives
       during initialization may safely repeat the same immutable calculation. */
    __asm__ volatile("" ::: "memory");
    bounds_ready=1;
}
/* Original BSP rock outlines; a five-unit foot radius is a DS adaptation. */
static int rock_contains(int x,int y,int radius){
    prepare_bounds();
    for(int polygon=0;polygon<STAGE_POLYGON_COUNT;polygon++){
        const StageBounds *b=&bounds[polygon];
        /* Most queries miss all rocks. Avoid edge math (including 64-bit
           multiplies on ARM9) unless the expanded polygon bounds contain us. */
        if(x<b->min_x-radius||x>b->max_x+radius||
           y<b->min_y-radius||y>b->max_y+radius)continue;
        const StagePolygon *shape=&stage_polygons[polygon];int inside=0;
        for(int i=0,j=shape->count-1;i<shape->count;j=i++){
            StagePoint a=stage_points[shape->start+i],b=stage_points[shape->start+j];
            int dx=b.x-a.x,dy=b.y-a.y,px=x-a.x,py=y-a.y;
            if((a.y>y)!=(b.y>y)){
                int64_t cross=(int64_t)px*dy-(int64_t)py*dx;
                if((dy>0&&cross<0)||(dy<0&&cross>0))inside=!inside;
            }
            int len=dx*dx+dy*dy,dot=px*dx+py*dy;
            if(!len||dot<=0){if(px*px+py*py<=radius*radius)return 1;}
            else if(dot>=len){int qx=x-b.x,qy=y-b.y;if(qx*qx+qy*qy<=radius*radius)return 1;}
            else {
                int64_t cross=(int64_t)px*dy-(int64_t)py*dx;
                if(cross*cross<=(int64_t)radius*radius*len)return 1;
            }
        }
        if(inside)return 1;
    }
    return 0;
}
static int barracks_furniture_contains(int x,int y){
    int inside=0;
    for(int i=0;i<CAMPAIGN_BARRACKS_FURNITURE_EDGES;i++){
        const int16_t *e=campaign_barracks_furniture[i];
        if((e[1]>y)!=(e[3]>y)){
            int dy=e[3]-e[1],cross=(x-e[0])*dy-(y-e[1])*(e[2]-e[0]);
            if((dy>0&&cross<0)||(dy<0&&cross>0))inside=!inside;
        }
    }
    return inside;
}
int stage_position_open(int x,int y){
    return stage_position_open_map(active_map,x,y);
}
/* Original BSP46b/47b/49b collision payloads are byte-identical. */
int stage_position_open_map(unsigned map,int x,int y){
    if(map==15)return x>=0&&x<FLOOD_WIDTH&&y>=(int)flood_floor_min(x)&&y<=(int)flood_floor_max(x);
    if(map==14)return x>=0&&x<DEER_GEOMETRY_WIDTH&&y>=(int)deer_floor_min(x)&&y<=(int)deer_floor_max(x);
    if(map==13)return x>=0&&x<THIEVES_WIDTH&&y>=(int)thieves_floor_min(x)&&y<=(int)thieves_floor_max(x);
    if(map==12)return x>=0&&x<2304&&y>=(int)forest_floor_min(x)&&y<=(int)forest_floor_max(x);
    if(map==11)return x>=0&&x<1024&&y>=(int)boss_floor_min(x)&&y<=(int)boss_floor_max(x);
    if(map==8)return x>=0&&x<768&&y>=(int)blacksmith_floor_min(x)&&y<=(int)blacksmith_floor_max(x);
    if(map==9)return x>=0&&x<2816&&y>=(int)ark_floor_min(x)&&y<=(int)ark_floor_max(x);
    if(map==10)return x>=0&&x<3200&&y>=(int)frog_floor_min(x)&&y<=(int)frog_floor_max(x);
    if(map==6)return x>=0&&x<4608&&y>=(int)war_floor_min(x)&&y<=(int)war_floor_max(x);
    if(map==7)return x>=0&&x<1024&&y>=(int)bridge_floor_min(x)&&y<=(int)bridge_floor_max(x);
    if(map==5)return x>=33&&x<CAMPAIGN_BARRACKS_WIDTH-33&&y>=0&&y<CAMPAIGN_BARRACKS_HEIGHT&&y>=campaign_barracks_floor_min[x]&&y<=campaign_barracks_floor_max[x]&&!barracks_furniture_contains(x,y);
    if(map==4)return x>=33&&x<CAMPAIGN_SHOP_WIDTH-33&&y>=0&&y<CAMPAIGN_SHOP_HEIGHT&&y>=campaign_shop_floor_min[x]&&y<=campaign_shop_floor_max[x];
    if(map>=3&&map<STAGE_MAP_COUNT)return x>=33&&x<CAMPAIGN_WORLD_WIDTH-33&&y>=0&&y<CAMPAIGN_WORLD_HEIGHT&&y>=campaign_world_floor_min[x]&&y<=campaign_world_floor_max[x];
    return map<STAGE_MAP_COUNT&&x>=ARENA_MIN_X&&x<=ARENA_MAX_X&&y>=ARENA_MIN_Y&&y<=ARENA_MAX_Y&&(map==3||!rock_contains(x,y,5));
}
/* Frog exit is x152 with a 24-unit interaction radius. Keep feet at least
   32 units inside its art-clamped camera while retaining that exit radius. */
int stage_min_x(void){return active_map==10?168:ARENA_MIN_X;}
int stage_max_x(void){return active_map>=3?stage_world_width(active_map)-34:ARENA_MAX_X;}
int stage_world_width(unsigned map){return map==15?FLOOD_WIDTH:map==14?DEER_GEOMETRY_WIDTH:map==13?THIEVES_WIDTH:map==12?2304:map==11?1024:map==8?768:map==9?2816:map==10?3200:map==6?4608:map==7?1024:map==5?CAMPAIGN_BARRACKS_WIDTH:map==4?CAMPAIGN_SHOP_WIDTH:(map>=3?CAMPAIGN_WORLD_WIDTH:WORLD_WIDTH);}
int stage_world_height(unsigned map){return map>=3?CAMPAIGN_WORLD_HEIGHT:WORLD_HEIGHT;}
/* Frog artwork starts at source x250; x272 clears its irregular left edge.
   Keep the authored exit/collision coordinates intact. */
int stage_camera_min_x(unsigned map){return map==10?136:map==12?56:0;} /* Forest Entrance has no art left of its fence at x 44. */
int stage_min_y(void){return active_map>=3?0:ARENA_MIN_Y;}
int stage_max_y(void){return active_map>=3?CAMPAIGN_WORLD_HEIGHT-1:ARENA_MAX_Y;}
/* Authored solid cliff/riser segments omitted by the outer floor envelope.
   BSP13 indices2,32,88..91,100,104,106 (kinds4/5); BSP2 indices25,23,
   26,109,104,105,117,7,6,4 (kind2). Coordinates use each existing world transform. */
static const int16_t keep_risers[][4]={
    {389,355,417,312},{417,312,421,307},{426,298,470,299},
    {388,307,394,298},{421,307,426,298},{394,298,398,291},
    {398,291,426,246},{470,299,504,299},{426,246,504,299}
};
static const int16_t barracks_risers[][4]={
    {959,272,953,276},{980,258,959,272},{1025,227,980,258},
    {1092,182,1025,227},{1100,177,1092,182},{1100,177,1124,172},{1124,172,1131,170},
    {953,276,1106,278},{1106,278,1150,285},{1150,285,1231,368}
};
static int crosses_riser(int x,int y,int nx,int ny){
    const int16_t (*edges)[4]=active_map==5?barracks_risers:keep_risers;
    unsigned count=active_map==5?sizeof(barracks_risers)/sizeof(*barracks_risers):active_map==3?sizeof(keep_risers)/sizeof(*keep_risers):0;
    for(unsigned i=0;i<count;i++){
        const int16_t *e=edges[i];int dx=e[2]-e[0],dy=e[3]-e[1];
        if((x<e[0]&&nx<e[0]&&x<e[2]&&nx<e[2])||(x>e[0]&&nx>e[0]&&x>e[2]&&nx>e[2])||
           (y<e[1]&&ny<e[1]&&y<e[3]&&ny<e[3])||(y>e[1]&&ny>e[1]&&y>e[3]&&ny>e[3]))continue;
        int before=dx*(y-e[1])-dy*(x-e[0]),after=dx*(ny-e[1])-dy*(nx-e[0]);
        if(!before)continue; /* A source spawn on an edge may move off it. */
        if((before<0&&after<0)||(before>0&&after>0))continue;
        int a=(nx-x)*(e[1]-y)-(ny-y)*(e[0]-x),b=(nx-x)*(e[3]-y)-(ny-y)*(e[2]-x);
        if(!a||!b||(a<0)!=(b<0))return 1;
    }
    return 0;
}
/* Source BSP ramp helper planes form diagonal stair surfaces. The back wall
   can change height beside a flat lane, so its envelope must not drag actors. */
static int stair_contains(const CampaignStair *stair,int x,int y){
    int sign=0;
    for(int i=0;i<4;i++){
        int j=(i+1)&3,dx=stair->x[j]-stair->x[i],dy=stair->y[j]-stair->y[i];
        int cross=dx*(y-stair->y[i])-dy*(x-stair->x[i]);
        /* Two world pixels cover rounded BSP seams without bridging walls. */
        if(abs(cross)<=2*(abs(dx)+abs(dy)))continue;
        int side=cross>0?1:-1;if(sign&&side!=sign)return 0;sign=side;
    }
    return 1;
}
static int stair_rise(int x,int y,int next){
    /* The short shop descent remains aligned with its BSP boundary. */
    if(active_map==4&&x>=0&&x<CAMPAIGN_SHOP_WIDTH&&next>=0&&next<CAMPAIGN_SHOP_WIDTH){
        int rise=campaign_shop_floor_min[next]-campaign_shop_floor_min[x];
        return abs(rise)<=2?rise:0;
    }
    if(active_map==5){
        /* Original BSP2 stair helper100/101 corridor slopes up-right; apply
           only on its surface, never to the lower flat lane below it. */
        static const CampaignStair stair={{935,1068,1090,956},{234,142,178,269},133,-92};
        if(stair_contains(&stair,x,y)||stair_contains(&stair,next,y)){
            int before=(x-935)*-2/3,after=(next-935)*-2/3;return after-before;
        }
        return 0;
    }
    if(active_map!=3)return 0;
    for(int i=0;i<CAMPAIGN_STAIR_COUNT;i++){
        const CampaignStair *stair=&campaign_stairs[i];
        if(!stair_contains(stair,x,y)&&!stair_contains(stair,next,y))continue;
        int before=(x-stair->x[0])*stair->rise_y/stair->rise_x;
        int after=(next-stair->x[0])*stair->rise_y/stair->rise_x;
        return after-before;
    }
    return 0;
}
void stage_move(Fighter *p,int from_x,int from_y){
    int target_x=p->x,target_y=p->y,x=from_x,y=from_y;
    if(active_map==10&&target_x<168)target_x=168;
    /* Unit sweeps prevent fast knockback crossing thin obstacles and permit sliding. */
    while(x!=target_x){int next=x+(target_x>x?1:-1),next_y=y;
        if(next>=stage_min_x()&&next<=stage_max_x())next_y+=stair_rise(x,y,next);
        if(!stage_position_open(next,next_y)||crosses_riser(x,y,next,next_y))break;
        target_y+=next_y-y;x=next;y=next_y;
    }
    while(y!=target_y){int next=y+(target_y>y?1:-1);if(!stage_position_open(x,next)||crosses_riser(x,y,x,next))break;y=next;}
    p->x=x;p->y=y;
}
int stage_segment_blocked(int x,int y,int to_x,int to_y){
    if(active_map>=3)return crosses_riser(x,y,to_x,to_y);
    int dx=abs(to_x-x),sx=x<to_x?1:-1,dy=-abs(to_y-y),sy=y<to_y?1:-1;
    int error=dx+dy;
    for(;;){
        if(rock_contains(x,y,1))return 1;
        if(x==to_x&&y==to_y)return 0;
        int twice=2*error;
        if(twice>=dy){error+=dy;x+=sx;}
        if(twice<=dx){error+=dx;y+=sy;}
    }
}
int stage_projectile_blocked(int from_x,int to_x,int y){return stage_segment_blocked(from_x,y,to_x,y);}

int stage_shake;
