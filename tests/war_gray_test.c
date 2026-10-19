#include "campaign_combat.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 Arena a;arena_init(&a);a.map_id=4;stage_set_map(4);a.running=1;a.p[0].x=500;a.p[0].y=280;a.p[0].hp=100;a.p[1].ready=0;
 Fighter e[6]={0},ally={0};e[0]=(Fighter){.x=645,.y=280,.hp=1000,.ready=1,.facing=-1};e[5]=(Fighter){.x=615,.y=280,.hp=1000,.ready=1,.facing=1,.knight_id=3};
 CampaignCombatState s={0};uint8_t targets[6]={0},levels[2]={1,1},hits[2];uint16_t keys[2]={0};
 int struck=0;
 for(int i=0;i<180;i++){campaign_combat_step(&s,&a,e,&ally,targets,keys,levels,hits);if(e[0].hp<1000)struck=1;assert(!hits[0]&&!hits[1]);}
 assert(struck&&e[5].attack_seq); /* An enemy-array gray is an ally with working attack AI. */
 e[0].ready=0;unsigned seq=e[5].attack_seq;int hp=a.p[0].hp;
 for(int i=0;i<120;i++)campaign_combat_step(&s,&a,e,&ally,targets,keys,levels,hits);
 assert(e[5].attack_seq==seq&&a.p[0].hp==hp); /* No hostile left: gray never targets a hero. */
 puts("Gray ally team/AI pass");return 0;
}




