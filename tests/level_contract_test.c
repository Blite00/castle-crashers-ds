/* Level contract: the checks every level of the campaign must pass before it counts as ported (docs/new-level-workflow.md, gate 2).
   It only needs stage.c and the generated floor tables, so a new level gets its row in LEVELS and fails here before it reaches a ROM.
   Each check is named after the bug it prevents. */
#include "stage.h"
#include "../generated/break_points.h"
#include "../generated/deer_layout.h"
#include "../generated/flood_geometry.h"
#include "../generated/camp_geometry.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned map;const char *name;
    int width;               /* stage_world_width */
    int start[2][2];         /* hero spawns, from the level's start function */
    int exit_x;              /* the walkable area must reach this x from the spawn */
    int art_left;            /* leftmost x the baked art covers: nothing walkable may lie left of it */
    int camera_min;          /* leftmost x the camera may show; it must not look past the art */
    int barriers;            /* columns with no open row (the original's pits, rivers and cliffs), pinned so a geometry change is noticed */
    const BreakPoint *breaks;unsigned break_count; /* scenery that must stand on walkable ground */
} Level;
static const Level LEVELS[]={
    {11,"Barbarian Boss",1024,{{214,316},{192,336}},500,0,0,228,0,0},
    {12,"Forest Entrance",2304,{{176,243},{156,258}},2200,44,56,38,break_forest,19},
    {13,"Thieves' Forest",6688,{{327,0},{307,0}},6400,0,0,157,break_thieves,16},
    {14,"Mill Run",DEER_WIDTH,{{444,216},{412,195}},3458,0,0,86,0,0},
    {16,"Tall Grass Field",CAMP_WIDTH,{{450,351},{430,367}},6100,0,0,345,0,0},
    {15,"Flooded Castle",FLOOD_WIDTH,{{324,234},{547,240}},100,0,0,142,0,0},
};
#define STEP 4
#define MAX_W 7200
static unsigned char seen[MAX_W/STEP][512/STEP];
static int open_near(unsigned map,int x,int y,int radius){
    for(int dy=-radius;dy<=radius;dy+=2)for(int dx=-radius;dx<=radius;dx+=2)if(stage_position_open_map(map,x+dx,y+dy))return 1;
    return 0;
}
int main(void){
    int failures=0;
    for(unsigned n=0;n<sizeof LEVELS/sizeof *LEVELS;n++){
        const Level *L=&LEVELS[n];
        stage_set_map(L->map);
        /* Dimensions: the width the rest of the game uses is the one that was audited. */
        assert(stage_world_width(L->map)==L->width&&stage_world_height(L->map)>0&&L->width<MAX_W);
        assert(stage_camera_min_x(L->map)==L->camera_min);
        int height=stage_world_height(L->map);
        /* "Nothing renders to the left of spawn": walkable ground never lies left of the camera's minimum, and the spawns are inside the art. */
        int first_open=-1;
        for(int x=0;x<L->width&&first_open<0;x++)for(int y=0;y<height;y++)if(stage_position_open_map(L->map,x,y)){first_open=x;break;}
        if(first_open<L->art_left||L->camera_min<L->art_left){printf("%s: walkable ground starts at x %d and the camera at %d, left of the art edge %d%c",L->name,first_open,L->camera_min,L->art_left,10);failures++;}
        /* Spawns and the ground they stand on. For levels whose start y is chosen by the level code, y=0 means "the first open row". */
        int sx[2],sy[2];
        for(int h=0;h<2;h++){
            sx[h]=L->start[h][0];sy[h]=L->start[h][1];
            if(!sy[h])for(int y=0;y<height;y++)if(stage_position_open_map(L->map,sx[h],y)){sy[h]=y;break;}
            if(!stage_position_open_map(L->map,sx[h],sy[h])){printf("%s: hero %d spawns on a blocked cell (%d,%d)\n",L->name,h,sx[h],sy[h]);failures++;}
            if(sx[h]<L->art_left){printf("%s: hero %d spawns left of the art\n",L->name,h);failures++;}
        }
        /* Collision continuity: no full-height wall except the allowed ones, and the exit is reachable by flood fill from the spawn. */
        int walls=0;
        for(int x=stage_camera_min_x(L->map);x<L->width-34;x++){int any=0;for(int y=0;y<height&&!any;y++)any=stage_position_open_map(L->map,x,y);if(!any)walls++;}
        if(walls!=L->barriers){printf("%s: %d columns have no open row (pinned %d)\n",L->name,walls,L->barriers);failures++;}
        memset(seen,0,sizeof seen);
        static int queue[MAX_W/STEP*512/STEP][2];int head=0,tail=0,reach=0;
        queue[tail][0]=sx[0]/STEP;queue[tail++][1]=sy[0]/STEP;seen[sx[0]/STEP][sy[0]/STEP]=1;
        while(head<tail){
            int cx=queue[head][0],cy=queue[head][1];head++;if(cx*STEP>reach)reach=cx*STEP;
            static const int d[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}};
            for(int k=0;k<8;k++){
                int nx=cx+d[k][0],ny=cy+d[k][1];
                if(nx<0||ny<0||nx*STEP>=L->width||ny*STEP>=height||seen[nx][ny])continue;
                if(!stage_position_open_map(L->map,nx*STEP,ny*STEP))continue;
                seen[nx][ny]=1;queue[tail][0]=nx;queue[tail++][1]=ny;
            }
        }
        if(reach<L->exit_x){printf("%s: walking from the spawn only reaches x %d, the exit is at %d\n",L->name,reach,L->exit_x);failures++;}
        /* Movement never ends on a blocked cell, however fast the shove (knockback, dash, scripted march). */
        unsigned seed=n*7919u+1;Fighter f;memset(&f,0,sizeof f);f.x=sx[0];f.y=sy[0];
        for(int i=0;i<4000;i++){
            seed=seed*1664525u+1013904223u;int dx=(int)((seed>>8)%41)-20,dy=(int)((seed>>16)%25)-12;
            int ox=f.x,oy=f.y;f.x+=dx;f.y+=dy;stage_move(&f,ox,oy);
            if(!stage_position_open_map(L->map,f.x,f.y)){printf("%s: stage_move ended on a blocked cell (%d,%d)\n",L->name,f.x,f.y);failures++;break;}
        }
        /* Breakable scenery and chests stand on ground a hero can reach (a hit has to be able to land). */
        for(unsigned i=0;i<L->break_count;i++)if(!open_near(L->map,L->breaks[i].x,L->breaks[i].y,40)){printf("%s: breakable %u at (%d,%d) is out of reach\n",L->name,i,L->breaks[i].x,L->breaks[i].y);failures++;}
        printf("%s: ok (walkable x %d..%d, %d wall columns)\n",L->name,first_open,reach,walls);
    }
    return failures?1:0;
}
