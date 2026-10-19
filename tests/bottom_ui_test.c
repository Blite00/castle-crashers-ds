#include "bottom_ui.h"
#include "item_selector.h"
#include "campaign_save.h"
#include "campaign_menu.h"
#include "hud.h"
#include "ui_skin.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "boss_meter_art.h"
static uint16_t guarded[256*192+2],reference[256*192];
static unsigned boss_pixels(const Arena *a,const Campaign *c,int guest){
 BottomUi u={.arena=a,.campaign=c,.hero=guest?1:0,.host=!guest,.client=guest,.audio_ok=1};
 bottom_ui_base(guarded+1,a,c);bottom_ui_game(guarded+1,&u);
 /* Original boss meter: count the lit red bar on its centre row (depleted pixels are dimmed). */
 unsigned n=0;for(int x=BOSS_METER_LEFT+BOSS_METER_FILL_LO;x<=BOSS_METER_LEFT+BOSS_METER_FILL_HI;x++)for(unsigned i=1;i<16;i++)if((BOSS_METER_FILL_MASK>>i&1)&&guarded[1+(BOSS_METER_TOP+BOSS_METER_H/2)*256+x]==boss_meter_palette[i]){n++;break;}
 assert(guarded[0]==0x1234&&guarded[256*192+1]==0x5678);return n;
}
static void boss_ui_checks(void){
 for(unsigned giant=0;giant<2;giant++)for(unsigned coop=0;coop<2;coop++){
  Arena a;Campaign c={0};arena_init(&a);campaign_start(&c,&a,coop);
  a.p[0].ready=1;a.p[1].ready=coop;
  if(giant)campaign_boss_start(&c,&a);else campaign_war_start(&c,&a,WAR_MAP);
  c.phase=giant?BOSS_FIGHT:WAR_MACHINE_FIGHT;a.countdown=0;a.running=1;
  unsigned maximum=coop?450:350;
  c.war.machine=(Fighter){.x=giant?808:3000,.y=290,.ready=1,.hp=maximum,.pose=POSE_IDLE,.facing=-1};
  for(unsigned step=0;step<3;step++){
   c.war.machine.hp=step==0?maximum:step==1?maximum/2:0;
   unsigned bar=BOSS_METER_FILL_HI-BOSS_METER_FILL_LO+1,expected=step==0?bar:step==1?bar/2:0;
   assert(boss_pixels(&a,&c,0)==expected);
   CampaignPacket p;campaign_pack(&p,&c,&a,step+1);assert(campaign_packet_valid(&p));
   Arena guest;Campaign received={0};campaign_unpack(&p,&received,&guest);
   assert(received.phase==c.phase&&received.war.machine.hp==c.war.machine.hp);
   assert(boss_pixels(&guest,&received,1)==expected);
  }
  /* A dead/disconnected partner must not shrink the original encounter max. */
  if(coop){
   c.war.machine.hp=225;a.p[1].ready=0;assert(boss_pixels(&a,&c,0)==(BOSS_METER_FILL_HI-BOSS_METER_FILL_LO+1)/2);
   CampaignPacket p;campaign_pack(&p,&c,&a,4);assert(campaign_packet_valid(&p));
   Arena guest;Campaign received={0};campaign_unpack(&p,&received,&guest);
   assert(boss_pixels(&guest,&received,1)==(BOSS_METER_FILL_HI-BOSS_METER_FILL_LO+1)/2);
  }
  c.war.machine.hp=0;c.phase=giant?BOSS_DEATH:WAR_MACHINE_DEATH;
  assert(boss_pixels(&a,&c,0)==0);
  CampaignPacket death;campaign_pack(&death,&c,&a,5);assert(campaign_packet_valid(&death));
  Arena guest;Campaign received={0};campaign_unpack(&death,&received,&guest);
  assert(received.phase==c.phase&&boss_pixels(&guest,&received,1)==0);
 }
}
static void save(int n){char path[80];snprintf(path,sizeof path,"build/bottom-%02d.ppm",n);FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n256 192\n255\n");for(int i=1;i<=256*192;i++){unsigned v=guarded[i];fputc((v&31)*255/31,f);fputc(((v>>5)&31)*255/31,f);fputc(((v>>10)&31)*255/31,f);}fclose(f);}
int main(void){
 Arena a={0};Campaign c={0};campaign_start(&c,&a,1);a.running=1;a.countdown=0;a.p[0].ready=a.p[1].ready=1;
 c.profiles[0].level=4;c.profiles[0].xp=690;c.profiles[0].gold=250;c.profiles[1].gold=999;
 a.p[0].hp=campaign_health(&c,0);a.p[1].hp=campaign_health(&c,1);a.p[1].knight_id=2;
 c.potions[0]=3;c.tools[0]=0;c.story_flags|=2;
 BottomUi u={.arena=&a,.campaign=&c,.fps=30,.work_ms=12,.host=1,.audio_ok=1,.save_status=1};
 CampaignCharacter fresh;campaign_character_default(&fresh,0);assert(fresh.tools==0);
 assert(!campaign_select_item(&c,0,ITEM_SHOVEL));assert(c.tools[0]==0);
 assert(campaign_item_mask(&c,0)==(1u<<ITEM_POTION));
 unsigned mask=(1u<<ITEM_POTION)|(1u<<ITEM_SHOVEL)|(1u<<ITEM_BOW)|(1u<<ITEM_SANDWICH);
 assert(item_selector_count(mask)==4);
 unsigned cursor=0;for(unsigned i=1;i<=4;i++){cursor=item_selector_current(mask,cursor);assert(cursor==i);cursor=item_selector_move(mask,cursor,1);}assert(cursor==1);
 assert(item_selector_move(mask,1,-1)==4&&item_selector_current(mask&~2u,1)==2);
 assert(!item_selector_current(0,4)&&!item_selector_move(0,1,1));
 assert(item_selector_hit(64,118)==0&&item_selector_hit(115,169)==0&&item_selector_hit(116,130)==-1);
 assert(item_selector_hit(30,130)==1&&item_selector_hit(149,130)==2&&item_selector_hit(90,170)==-1);
 guarded[0]=0x1234;guarded[256*192+1]=0x5678;
 boss_ui_checks();
 assert(bottom_ui_map_hit(7,80)==-1&&bottom_ui_map_hit(248,80)==-1&&bottom_ui_map_hit(50,113)==-1);
 assert(bottom_ui_map_hit(8,64)==0&&bottom_ui_map_hit(33,112)==0&&bottom_ui_map_hit(34,80)==1&&bottom_ui_map_hit(221,80)==1&&bottom_ui_map_hit(222,80)==2);
 assert(campaign_menu_hit(220,70)==6&&campaign_menu_hit(10,90)==0&&campaign_menu_hit(10,169)==3);
 for(int n=0;n<12;n++){
  u.hero=0;u.client=0;u.host=1;u.campaign=&c;c.phase=KEEP_FIGHT;a.map_id=3;c.paused=0;u.peer_loading=0;a.clash_timer=0;
  if(n==1){a.map_id=BOSS_MAP;c.phase=BOSS_FIGHT;c.war.gate_hp=450;c.war.machine.hp=225;}
  if(n==2||n==3){c.phase=CAMPAIGN_WORLD_MAP;c.group=3;c.spawned=3;c.completed=1;if(n==3){u.hero=1;u.client=1;u.host=0;}}
  if(n==4){a.map_id=8;c.phase=KEEP_TRAVEL;}
  if(n==5){a.map_id=9;c.phase=KEEP_TRAVEL;}
  if(n==6){u.hero=1;u.client=1;u.host=0;}
  if(n==7){c.paused=1;}
  if(n==8){a.clash_timer=10;a.clash_score[0]=3;a.clash_score[1]=7;}
  if(n==9){u.peer_loading=1;}
  if(n==10){a.map_id=0;u.campaign=0;u.practice=1;u.host=0;u.difficulty=2;}
  if(n==11){a.p[1].ready=0;a.running=0;}
  bottom_ui_base(guarded+1,&a,u.campaign);memcpy(reference,guarded+1,sizeof reference);
  bottom_ui_game(guarded+1,&u);
  /* Every context retains the meters verbatim, including absent guests. */
  assert(!memcmp(reference,guarded+1,59*256*sizeof(uint16_t)));
  if(n==6)campaign_menu_draw(guarded+1,&c,1,2,2,1,1);
  assert(guarded[0]==0x1234&&guarded[256*192+1]==0x5678);save(n);
 }
 /* Empty potions and missing shovel must remain invisible. */
 a.running=1;u.campaign=&c;u.hero=1;c.potions[1]=0;c.tools[1]=0;
 bottom_ui_base(guarded+1,&a,&c);memcpy(reference,guarded+1,sizeof reference);bottom_ui_game(guarded+1,&u);
 for(int y=118;y<170;y++)for(int x=34;x<154;x++)assert(reference[y*256+x]==guarded[1+y*256+x]);
 /* Four-item layout is a UI fixture only: it does not grant campaign items. */
 for(unsigned i=1;i<=4;i++){bottom_ui_base(guarded+1,&a,&c);item_selector_draw(guarded+1,mask,i,i,3);assert(guarded[0]==0x1234&&guarded[256*192+1]==0x5678);save(11+i);}
 puts("PASS: bottom contexts preserve meters, hidden inventory, touch bounds and framebuffer guards");
}
