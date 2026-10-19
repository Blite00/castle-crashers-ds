#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_deer.h"
#include "campaign_flood.h"
#include "bottom_ui.h"
#include "campaign_forest.h"
#include "campaign_thieves.h"
#include "ui_skin.h"
#include "item_selector.h"
#include "hud.h"
#include "campaign_menu.h"
#include "stage.h"
#include "bot.h"
#include "menu.h"
#include <stdio.h>
#include <stdlib.h>
void bottom_ui_base(uint16_t *p,const Arena *a,const Campaign *c){
 ui_skin_background(p);
 Arena meters=*a;
 if(c)for(unsigned i=0;i<2;i++){
  meters.p[i].hp=a->p[i].hp*ARENA_HEALTH/campaign_health(c,i);
  meters.p[i].mana=a->p[i].mana*ARENA_MANA/campaign_mana(c,i);
 }
 hud_render(p,&meters);
 if(c)for(unsigned i=0;i<2;i++)if(a->p[i].ready){
  char label[24];hud_render_progress(p,i,c->profiles[i].level,c->profiles[i].xp);
  snprintf(label,sizeof label,"GOLD %u",c->profiles[i].gold);menu_text(p,5+i*128,48,label,UI_GOLD);
  if(c->pickup_timer[i]&&c->pickup_kind[i]!=DROP_WEAPON&&c->pickup_kind[i]!=DROP_POTION){
   int gold=c->pickup_kind[i]<=DROP_BAG||c->pickup_kind[i]==DROP_GEM;
   snprintf(label,sizeof label,"+%u",c->pickup_value[i]);menu_text(p,91+i*128,gold?48:18,label,gold?UI_GOLD:UI_RGB(10,31,10));
  }
 }
}
int bottom_ui_map_hit(int x,int y){
 if(y<64||y>=113||x<8||x>=248)return -1;
 return x<34?0:x>=222?2:1;
}
void bottom_ui_game(uint16_t *p,const BottomUi *u){
 const Arena *a=u->arena;const Campaign *c=u->campaign;unsigned h=u->hero<2?u->hero:0;
 char title[64],detail[80]="",hint[80]="",footer[64];
 snprintf(title,sizeof title,"%s",stage_name(a->map_id));
 int map=c&&c->phase==CAMPAIGN_WORLD_MAP;
 int mother=c&&a->map_id==THIEVES_MAP?campaign_thieves_boss_hp(c):0;
 int boss=c&&((a->map_id==BOSS_MAP&&c->phase==BOSS_FIGHT)||c->phase==WAR_MACHINE_FIGHT||mother>0);
 if(c){
  if(map){
   static const char *names[]={"Blacksmith","Home Castle","Castle Keep","Barbarian War","Barbarian Boss","Forest Entrance","Thieves' Forest","Mill Run","Flooded Castle"};
   unsigned node=c->group<CAMPAIGN_WORLD_NODES?c->group:1;
   int locked=!campaign_world_unlocked(c,node);
   snprintf(title,sizeof title,"%s",names[node]);
   snprintf(detail,sizeof detail,"%s",locked?"Complete the previous level":u->client?"Host chooses the destination":"D-pad or tap arrows to travel");
   snprintf(hint,sizeof hint,"%s",campaign_world_travelling(c)?"Walking to destination...":u->peer_loading?"Guest is loading...":u->client?"Your party travels together":locked?"Destination locked":"A or tap here to enter");
  }else{
   const char *objective=a->map_id==FLOOD_MAP?campaign_flood_objective(c):a->map_id==DEER_MAP?campaign_deer_objective(c):a->map_id==THIEVES_MAP?campaign_thieves_objective(c):a->map_id==FOREST_MAP?campaign_forest_objective(c):a->map_id==BOSS_MAP?campaign_boss_objective(c):a->map_id>=8?campaign_hubs_objective(a->map_id):a->map_id>=6?campaign_war_objective(c,a):campaign_objective(c);
   snprintf(detail,sizeof detail,"%s",c->phase==KEEP_STORE?"Approach a shop board to buy":objective);
   if(c->phase==KEEP_INTRO||c->phase==KEEP_ABDUCTION||c->phase==KEEP_BARRACKS_INTRO)snprintf(hint,sizeof hint,"A: skip cinematic");
   else if(c->revive[0]||c->revive[1])snprintf(hint,sizeof hint,"Reviving - keep holding R");
   else if(!(a->map_id==BOSS_MAP&&c->phase>=BOSS_DUEL)&&a->p[1].ready&&(!a->p[0].hp||!a->p[1].hp))snprintf(hint,sizeof hint,"Hold R beside your fallen ally");
   else if(c->phase==KEEP_FIGHT||c->phase==KEEP_STORE_FIGHT)snprintf(hint,sizeof hint,"Enemies defeated: %u / %u",c->kills,CAMPAIGN_WAVES);
   else if(a->map_id==DEER_MAP)snprintf(hint,sizeof hint,"B: jump logs  D-pad: lane and pace");
   else if(a->map_id==6||a->map_id==7||a->map_id>=FOREST_MAP)snprintf(hint,sizeof hint,"Enemies defeated: %u",c->kills);
   else snprintf(hint,sizeof hint,"START: pause   Tap STATS");
   if(a->p[h].hp&&!c->revive[0]&&!c->revive[1])for(int d=0;d<CAMPAIGN_DROPS;d++){
    const CampaignDrop *item=&c->drops[d];
    if(item->kind==DROP_WEAPON&&abs(a->p[h].x-item->x)<=28&&abs(a->p[h].y-item->y)<=22){
     const CampaignWeapon *w=campaign_weapon_info(item->value);
     if(w){if(c->profiles[h].level<w->level)snprintf(hint,sizeof hint,"%s: needs LV %u (you LV %u)",w->name,w->level,c->profiles[h].level);else snprintf(hint,sizeof hint,"A: %s (LV %u)",w->name,w->level);}
     break;
    }
   }
  }
 }else{
  snprintf(detail,sizeof detail,"%s  -  Round %u",u->host?"Host":u->client?"Guest":"Practice",a->round);
  if(u->practice)snprintf(hint,sizeof hint,"%s bot   START: pause",bot_difficulty_name(u->difficulty));
  else snprintf(hint,sizeof hint,"Local wireless arena");
 }
 if(!a->running){
  snprintf(detail,sizeof detail,"%s",u->host?(a->p[1].ready?"Guest ready":u->guest_known?"Guest choosing a knight":"Waiting for a guest"):"Waiting for host to start");
  snprintf(hint,sizeof hint,"%s",u->host?(arena_players_ready(a)?"START: begin":"Both knights must be ready"):"Your knight is ready");
 }
 if(boss){unsigned maximum=mother?THIEVES_MOTHER_HEALTH:a->map_id==BOSS_MAP?c->war.gate_hp:c->war.gate_hp>151?450:350;snprintf(detail,sizeof detail,"BOSS  %u / %u",mother?(unsigned)mother:(unsigned)c->war.machine.hp,maximum);}
 if(a->clash_timer){
  int group=c&&c->phase!=BOSS_DUEL;
  snprintf(detail,sizeof detail,"%s",group&&a->p[h].pose!=POSE_CLASH?"ALLY WEAPON CLASH":"WEAPON CLASH - MASH A/X/B!");
  snprintf(hint,sizeof hint,group?"KNIGHT %u / 10  ENEMY %u / 10":"P1 %u / 10    P2 %u / 10",a->clash_score[0],a->clash_score[1]);
 }
 if(c&&a->map_id==BOSS_MAP&&campaign_boss_buried_frame(c,h))snprintf(detail,sizeof detail,"WIGGLE RIGHT / LEFT TO ESCAPE");
 if(a->countdown)snprintf(hint,sizeof hint,"Ready... %u",(a->countdown+29)/30);
 if(a->winner){
  if(c)snprintf(detail,sizeof detail,"%s",a->winner==1?"LEVEL CLEARED":"CAMPAIGN FAILED");
  else if(a->winner==3)snprintf(detail,sizeof detail,"DRAW");else snprintf(detail,sizeof detail,"PLAYER %u WINS",a->winner);
  snprintf(hint,sizeof hint,"%s",u->client?"Waiting for host":c?"START: world map":"START: rematch");
 }
 if(u->host&&u->peer_loading)snprintf(hint,sizeof hint,"Waiting for guest to load...");
 if(c&&c->paused)snprintf(hint,sizeof hint,"HOST PAUSED THE GAME");
 if(boss&&(c->paused||u->peer_loading||a->countdown||c->revive[0]||c->revive[1]))snprintf(detail,sizeof detail,"%s",hint);
 ui_skin_panel(p,8,64,240,49);
 ui_skin_text(p,map?35:16,68,map?186:224,title,UI_INK);
 /* While a boss fight is plain, the original meter replaces the "BOSS n / m" line. */
 int meter=boss&&!(c->paused||u->peer_loading||a->countdown||c->revive[0]||c->revive[1]||a->clash_timer||a->winner||(a->map_id==BOSS_MAP&&campaign_boss_buried_frame(c,h)));
 if(!meter)ui_skin_text(p,16,82,224,detail,UI_INK);
 if(!boss)ui_skin_text(p,16,98,224,hint,UI_INK);
 if(map&&!u->client){menu_text(p,17,68,"<",UI_INK);menu_text(p,230,68,">",UI_INK);}
 if(meter){unsigned maximum=mother?THIEVES_MOTHER_HEALTH:a->map_id==BOSS_MAP?c->war.gate_hp:c->war.gate_hp>151?450:350;hud_render_boss(p,mother?(unsigned)mother:(unsigned)c->war.machine.hp,maximum);}
 if(a->running&&!a->winner){
  if(c){campaign_stats_button(p,campaign_points(c,h));item_selector_draw(p,campaign_item_mask(c,h),u->item_cursor,campaign_equipped_item(c,h),c->potions[h]);}
  else hud_render_item(p,&a->p[h]);
 }
 snprintf(footer,sizeof footer,"%u/%u FPS %ums%s%s",u->fps,c&&(c->phase==KEEP_INTRO||c->phase==KEEP_ABDUCTION)?15:30,u->work_ms,c?" | ":"",!c?"":u->save_status==3?"No SD save":u->save_status==4?"Save failed":!u->audio_ok?"Audio missing":u->save_dirty?"Unsaved":"Saved");
 ui_skin_text(p,8,179,240,footer,UI_LIGHT);
}
