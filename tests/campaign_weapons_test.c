#include "campaign_weapons.h"
#include "arena.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void setup(Arena *a,Fighter enemy[6],Fighter *ally){
 arena_init(a);a->map_id=3;a->running=1;a->p[0].x=100;a->p[0].y=239;a->p[1].ready=0;memset(enemy,0,6*sizeof(*enemy));memset(ally,0,sizeof(*ally));
}
int main(void){
 assert(campaign_weapon_info(2)->source_id==9&&campaign_weapon_info(2)->level==5);
 assert(campaign_weapon_info(4)->source_id==10&&campaign_weapon_info(4)->agility==3&&campaign_weapon_info(4)->strength==-1&&campaign_weapon_info(4)->level==5);
 assert(campaign_weapon_info(5)->source_id==38&&campaign_weapon_info(5)->magic==3&&campaign_weapon_info(5)->level==5&&campaign_weapon_info(6)->source_id==30&&campaign_weapon_info(6)->defense==1&&campaign_weapon_info(6)->magic==1&&campaign_weapon_info(6)->strength==-1&&campaign_weapon_info(7)->source_id==37&&campaign_weapon_info(7)->strength==2&&campaign_weapon_info(7)->agility==-1&&campaign_weapon_info(8)->source_id==42&&campaign_weapon_info(8)->agility==1&&campaign_weapon_info(9)->source_id==54&&campaign_weapon_info(9)->agility==1&&campaign_weapon_info(9)->strength==-1&&campaign_weapon_info(10)->source_id==55&&campaign_weapon_info(10)->defense==3&&campaign_weapon_info(11)->source_id==62&&campaign_weapon_info(11)->magic==-1&&!campaign_weapon_info(12));
 {WeaponSet all=weapon_set_low(63),some=weapon_set_low(15),one=weapon_set_low(1);assert(campaign_weapon_can_equip(&all,5,5)&&!campaign_weapon_can_equip(&some,5,5));campaign_weapon_unlock(&one,5);assert(one.w[0]==33&&campaign_weapon_cycle(&all,5,5,1)==0);
  WeaponSet far={{0,0,0,0}};weapon_set_add(&far,127);weapon_set_add(&far,1);assert(weapon_set_has(&far,127)&&weapon_set_has(&far,1)&&!weapon_set_has(&far,126)&&!weapon_set_has(&far,128)&&!campaign_weapon_can_equip(&far,127,99));}
 assert(campaign_weapon_info(1)->price==32&&campaign_weapon_info(3)->critical_divisor==100);
 {WeaponSet s=weapon_set_low(15);assert(!campaign_weapon_can_equip(&s,2,4)&&campaign_weapon_can_equip(&s,2,5));}
 {WeaponSet s=weapon_set_low(7);assert(campaign_weapon_cycle(&s,1,1,1)==0&&campaign_weapon_cycle(&s,5,1,1)==2);}
 {WeaponSet s=weapon_set_low(1);campaign_weapon_unlock(&s,2);assert(s.w[0]==5&&campaign_weapon_stat(1,-1)==1);}
 assert(campaign_weapon_max_health(1,0)==100&&campaign_weapon_max_health(1,1)==128&&campaign_weapon_max_mana(3)==40);
 Arena a;Fighter e[6],ally;uint16_t input[9]={RIGHT};uint8_t targets[9]={2},levels[2]={1,1},hits[2];
 ArenaCampaignAttributes attributes[2]={{1,1,1,1,0},{1,1,1,1,0}};
 setup(&a,e,&ally);for(int t=0;t<20;t++)arena_group_step_attributes(&a,e,&ally,input,targets,levels,attributes,hits);assert(a.p[0].x==146);
 setup(&a,e,&ally);attributes[0].agility=2;for(int t=0;t<20;t++)arena_group_step_attributes(&a,e,&ally,input,targets,levels,attributes,hits);assert(a.p[0].x==147);
 input[0]=0;levels[0]=5;attributes[0]=(ArenaCampaignAttributes){2,2,1,1,0};setup(&a,e,&ally);
 a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].animation=1;a.p[0].cooldown=8;
 e[0]=(Fighter){.x=135,.y=239,.hp=50,.facing=-1,.ready=1};
 arena_group_step_attributes(&a,e,&ally,input,targets,levels,attributes,hits);assert(e[0].hp==45&&hits[0]==1);
 setup(&a,e,&ally);attributes[0]=(ArenaCampaignAttributes){1,2,1,2,0};
 e[0]=(Fighter){.x=130,.y=239,.hp=50,.facing=-1,.ready=1};a.shots[0]=(Projectile){.x=100,.y=239,.z=25,.vx=12,.life=30,.owner=0,.kind=SHOT_ARROW};
 arena_group_step_attributes(&a,e,&ally,input,targets,levels,attributes,hits);assert(e[0].hp==46);
 unsigned criticals=0;attributes[0]=(ArenaCampaignAttributes){1,1,1,1,100};levels[0]=1;
 for(unsigned t=0;t<1000;t++){
  setup(&a,e,&ally);a.tick=t;a.p[0].pose=POSE_ATTACK;a.p[0].attack_kind=HIT_LIGHT;a.p[0].animation=1;a.p[0].cooldown=8;
  e[0]=(Fighter){.x=135,.y=239,.hp=50,.facing=-1,.ready=1};
  arena_group_step_attributes(&a,e,&ally,input,targets,levels,attributes,hits);assert(e[0].hp==46||e[0].hp==34);criticals+=e[0].hp==34;
 }
 assert(criticals>=2&&criticals<=25);
 puts("PASS: source weapon IDs, ownership/level gates, health, agility movement, strength, arrows and rare criticals");
}
