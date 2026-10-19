#include "camera.h"
#include "stage.h"
static int min(int a,int b){return a<b?a:b;}
static int max(int a,int b){return a>b?a:b;}
static int clamp(int a,int lo,int hi){return min(max(a,lo),hi);}
void camera_clamp_world(ArenaCamera *c,const Arena *a){
    int w=stage_world_width(a->map_id),h=stage_world_height(a->map_id);
    c->x=clamp(c->x,stage_camera_min_x(a->map_id),max(stage_camera_min_x(a->map_id),w-c->width));
    c->y=clamp(c->y,0,max(0,h-c->height));
}
void camera_limit_horizontal(ArenaCamera *c,const Arena *a,int left,int right){
    int w=stage_world_width(a->map_id);
    left=clamp(left,0,w);right=clamp(right,left,w);
    /* Co-op zoom can exceed a short encounter. Preserve zoom and its right
       boundary, extending left rather than cropping the far-right fighter. */
    int hi=max(0,right-c->width),lo=min(left,hi);
    c->x=clamp(c->x,lo,hi);
    camera_clamp_world(c,a);
}
/* Sub-unit easing: the camera follows a quarter of the remaining distance in 1/16 world units and only the rounded value
   is shown, so a fighter walking 1-2 units per tick no longer makes the picture (and the background) step unevenly. The
   vertical axis also has a dead band: small up/down walking inside the frame does not move the view at all. */
static int shake_jx,shake_jy;
static int fix_x,fix_y,shown_x=-1,shown_y=-1;
static int follow(int *fix,int *shown,int current,int target){
    if(current!=*shown)*fix=current*16;
    int d=target*16-*fix;
    *fix+=d/4;if(d>-8&&d<8)*fix=target*16;
    *shown=(*fix+8)>>4;return *shown;
}
#define DEAD_BAND_Y 18
void camera_update(ArenaCamera *c,const Arena *a){
    if(c->ready){c->x-=shake_jx;c->y-=shake_jy;}shake_jx=shake_jy=0;
    int world_width=stage_world_width(a->map_id),world_height=stage_world_height(a->map_id);
    Arena visible=*a;
    if(a->map_id>=3){
        /* A fallen partner must not pin the camera hundreds of units behind. */
        if(visible.p[0].hp<=0&&visible.p[1].ready&&visible.p[1].hp>0)visible.p[0]=visible.p[1];
        if(visible.p[1].hp<=0&&visible.p[0].ready&&visible.p[0].hp>0)visible.p[1]=visible.p[0];
    }
    if(!visible.p[0].ready&&visible.p[1].ready)visible.p[0]=visible.p[1];
    if(!visible.p[1].ready)visible.p[1]=visible.p[0];
    a=&visible;
    /* Work in world units. Include swords, heads, jump height and floor shadows. */
    int left=min(a->p[0].x,a->p[1].x)-32;
    int right=max(a->p[0].x,a->p[1].x)+32;
    /* Frame the fighters' ground positions so jumps and slams do not bob the picture; the head is kept in view below. */
    int top=min(a->p[0].y,a->p[1].y)-56;
    int head=min(a->p[0].y-a->p[0].z,a->p[1].y-a->p[1].z)-56;
    int bottom=max(a->p[0].y,a->p[1].y)+8;
    int wanted=max(288,max(right-left,(bottom-min(top,head))*4/3+4));
    wanted=clamp((wanted+3)&~3,288,WORLD_WIDTH);
    int reset=!c->ready||c->round!=a->round;
    /* Widen immediately to prevent clipping; ease back in at four units/tick. */
    if(reset||wanted>c->width)c->width=wanted;
    else c->width=max(wanted,c->width-4);
    c->height=c->width*3/4;
    int x=(left+right-c->width)/2,y=(top+bottom-c->height)/2;
    if(!reset){
        int dy=y-c->y;y=dy>DEAD_BAND_Y?y-DEAD_BAND_Y:dy<-DEAD_BAND_Y?y+DEAD_BAND_Y:c->y;
        x=follow(&fix_x,&shown_x,c->x,x);y=follow(&fix_y,&shown_y,c->y,y);
    }
    /* Following may lag, but never enough to hide either fighter. */
    c->x=clamp(clamp(x,right-c->width,left),0,world_width-c->width);
    c->y=clamp(clamp(y,bottom-c->height,top),0,world_height-c->height);
    c->ready=1;c->round=a->round;
    if(head<c->y)c->y=head; /* Only a head leaving the top of the view moves the camera for a jump. */
    camera_clamp_world(c,a);
    if(stage_shake){const int s=stage_shake;shake_jx=(a->tick&1)?s:-s;shake_jy=(a->tick&2)?s/2+1:-(s/2+1);c->x+=shake_jx;c->y+=shake_jy;stage_shake=0;}
}
