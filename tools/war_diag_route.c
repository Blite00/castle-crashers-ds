#include <stdio.h>
#include <string.h>
#include <time.h>
#include "campaign.h"
#include "campaign_war.h"
#include "stage.h"
#include "render.h"
static unsigned short pixels[49152];
int main(void){Arena a;Campaign c={0};arena_init(&a);campaign_start(&c,&a,0);campaign_war_start(&c,&a,6);memset(c.enemies,0,sizeof c.enemies);memset(&c.ally,0,sizeof c.ally);c.group=5;c.phase=WAR_TRAVEL;a.p[0].x=1706;a.p[0].y=280;unsigned old=0;for(unsigned t=0;t<900;t++){uint16_t k[2]={RIGHT,0};clock_t b=clock();campaign_step(&c,&a,k);double sim=(double)(clock()-b)*1000/CLOCKS_PER_SEC;arena_render_campaign(&c);b=clock();arena_render(&a,pixels);double ren=(double)(clock()-b)*1000/CLOCKS_PER_SEC;int live=0;for(int e=0;e<6;e++)live+=c.enemies[e].ready&&c.enemies[e].hp>0;if(t%30==0||old!=c.phase||sim>10||ren>10)printf("%u phase%u group%u x%d y%d hp%d runners%u mask%u live%d machine%d@%d,%d sim%.2f render%.2f\n",t,c.phase,c.group,a.p[0].x,a.p[0].y,a.p[0].hp,c.war.runner_spawned,c.war.runner_mask,live,c.war.machine.ready,c.war.machine.x,c.war.machine.y,sim,ren);old=c.phase;}return 0;}
