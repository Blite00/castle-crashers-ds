#include "../generated/barracks_rail.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
    assert(stage_position_open(200,185)); /* Behind the central rock. */
    assert(stage_position_open(200,285)); /* In front of it. */
    assert(!stage_position_open(200,230));
    assert(!stage_position_open(385,195)); /* Right-hand rock from original BSP. */
    assert(!stage_position_open(200,ARENA_MIN_Y-1));
    assert(!stage_position_open(ARENA_MAX_X+1,285));
    Fighter p={.x=320,.y=230};stage_move(&p,80,230);
    assert(p.x>80&&p.x<119&&stage_position_open(p.x,p.y));
    p=(Fighter){.x=200,.y=180};stage_move(&p,200,285);
    assert(p.y>261&&stage_position_open(p.x,p.y));
    p=(Fighter){.x=320,.y=285};stage_move(&p,80,285);
    assert(p.x==320&&p.y==285);
    assert(stage_projectile_blocked(80,320,230));
    assert(!stage_projectile_blocked(80,320,285));
    for(unsigned map=0;map<STAGE_ARENA_COUNT;map++){
        stage_set_map(map);
        assert(stage_position_open_map(map,80,239));
        assert(stage_position_open_map(map,344,239));
        assert(!stage_position_open_map(map,200,230));
        Arena round;arena_init(&round);round.map_id=map;arena_start_round(&round);
        assert(round.map_id==map); /* Host choice survives rematches. */
    }
    assert(!stage_position_open_map(STAGE_MAP_COUNT,80,239));
    stage_set_map(3);
    /* Four lanes follow the actual diagonal first staircase, not the back wall. */
    for(int lane=200;lane<=230;lane+=10){
        Fighter walker={.x=410,.y=lane};
        for(int direction=1;direction>=-1;direction-=2){
            for(int step=0;step<180;step++){
                int x=walker.x,y=walker.y;walker.x+=direction;stage_move(&walker,x,y);
                assert(walker.x==x+direction&&abs(walker.y-y)<=1);
                assert(stage_position_open(walker.x,walker.y));
            }
            if(direction==1)assert(walker.y==lane+56);
        }
        assert(walker.x==410&&walker.y==lane);
    }
    /* The lower flat lane beside the stair must not be dragged down with it. */
    Fighter flat={.x=440,.y=330};
    for(int step=0;step<150;step++){int x=flat.x;flat.x++;stage_move(&flat,x,flat.y);assert(flat.y==330);}
    assert(flat.x==590);
    /* The exposed stair side and cliff face separate two valid floor lanes.
       Walking, diagonal knockback and jumping cannot cross their authored edge. */
    for(int z=0;z<=40;z+=40){
        Fighter upper={.x=470,.y=340,.z=z};stage_move(&upper,470,230);assert(upper.y<276);
        Fighter lower={.x=470,.y=210,.z=z};stage_move(&lower,470,330);assert(lower.y>299);
        Fighter cliff={.x=410,.y=350,.z=z};stage_move(&cliff,410,240);assert(cliff.y<272);
        Fighter diagonal={.x=500,.y=350,.z=z};stage_move(&diagonal,470,230);assert(diagonal.y<297);
        Fighter lower_diagonal={.x=440,.y=210,.z=z};stage_move(&lower_diagonal,470,330);assert(lower_diagonal.y>=299);
    }
    assert(stage_segment_blocked(470,230,470,330));
    /* Both co-op heroes independently retain their lanes going down and back up. */
    Fighter heroes[2]={{.x=410,.y=210},{.x=410,.y=230}};
    for(int direction=1;direction>=-1;direction-=2)for(int step=0;step<180;step++){
        for(int hero=0;hero<2;hero++){int x=heroes[hero].x,y=heroes[hero].y;heroes[hero].x+=direction;stage_move(&heroes[hero],x,y);assert(heroes[hero].x==x+direction&&abs(heroes[hero].y-y)<=1);}
        assert(heroes[1].y>heroes[0].y&&heroes[1].y-heroes[0].y<=30);
    }
    assert(heroes[0].y==210&&heroes[1].y==230);
    /* All three source stair surfaces are traversable from both spawn lanes. */
    for(int lane=230;lane<=240;lane+=10){
        Fighter walker={.x=110,.y=lane};
        for(int step=0;step<2300;step++){int x=walker.x,y=walker.y;walker.x++;stage_move(&walker,x,y);assert(walker.x==x+1&&abs(walker.y-y)<=1&&stage_position_open(walker.x,walker.y));}
    }
    /* A real vertical ramp-side wall remains a wall rather than teleporting up. */
    Fighter wall={.x=1923,.y=145};stage_move(&wall,1919,145);
    assert(wall.x<1923&&wall.y==145&&stage_position_open(wall.x,wall.y));
    /* BSP2 staircase front edge likewise cannot be crossed from its upper
       surface into the separate lower barracks floor, even while airborne. */
    stage_set_map(5);
    for(int z=0;z<=40;z+=40){
        Fighter upper={.x=1000,.y=340,.z=z};stage_move(&upper,1000,220);assert(upper.y<245);
        Fighter lower={.x=1000,.y=200,.z=z};stage_move(&lower,1000,280);assert(lower.y>=277);
        Fighter diagonal={.x=1025,.y=340,.z=z};stage_move(&diagonal,1000,220);assert(diagonal.y<228);
        Fighter lower_diagonal={.x=1025,.y=200,.z=z};stage_move(&lower_diagonal,1000,280);assert(lower_diagonal.y>=277);
    }
    /* Push against every interior rail column using real movement. The
       legal upper-side stopping position must remain behind the artwork. */
    for(int x=978;x<=1110;x++){
        Fighter pressed={.x=x,.y=350};
        int start=200-(x-978)*2/3;
        stage_move(&pressed,x,start);
        assert(barracks_rail_behind(pressed.x,pressed.y));
    }
    for(int lane=240;lane<=250;lane+=10){
        Fighter walker={.x=930,.y=lane};
        for(int step=0;step<175;step++){int x=walker.x,y=walker.y;walker.x++;stage_move(&walker,x,y);assert(walker.x==x+1&&abs(walker.y-y)<=1);}
        assert(walker.y<170);
        for(int step=0;step<175;step++){int x=walker.x,y=walker.y;walker.x--;stage_move(&walker,x,y);assert(walker.x==x-1&&abs(walker.y-y)<=1);}
    }
    stage_set_map(0);
    Arena a;arena_init(&a);arena_start_round(&a);a.countdown=0;
    unsigned rng=6;
    for(int i=0;i<60000;i++){
        rng=rng*1664525u+1013904223u;
        uint16_t keys[2]={(rng>>12)&511,(rng>>23)&511};arena_step(&a,keys);
        for(int player=0;player<2;player++)assert(stage_position_open(a.p[player].x,a.p[player].y));
        if(a.winner)arena_start_round(&a);
    }
    puts("PASS: original rock footprints, swept movement, projectile obstruction, diagonal stair lanes/edges/co-op traversal and 60000 legal-position ticks");
}

