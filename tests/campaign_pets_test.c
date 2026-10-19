#include "campaign.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static int floor_y(int x){for(int y=250;y<448;y++)if(stage_position_open_map(5,x,y))return y;assert(0);return 0;}
static void start(Campaign*c,Arena*a,int coop){memset(c,0,sizeof(*c));arena_init(a);campaign_start(c,a,coop);c->phase=KEEP_BARRACKS_TRAVEL;for(unsigned h=0;h<2;h++){a->p[h].x=155+h*30;a->p[h].y=286;}}
static void tick(Campaign*c,Arena*a){const uint16_t keys[2]={0,0};campaign_step(c,a,keys);}
static CampaignPacket packet(Campaign*c,Arena*a){CampaignPacket p;campaign_pack(&p,c,a,77);static unsigned calls;if(!campaign_packet_valid(&p)){fprintf(stderr,"Invalid pet packet %u map%u xy%d,%d partner%d,%d drops%u:%u age%u\n",calls,a->map_id,a->p[0].x,a->p[0].y,a->p[1].x,a->p[1].y,c->drops[0].kind,c->drops[0].value,c->drop_age[0]);fprintf(stderr,"legal %d,%d drop%d\n",stage_position_open_map(a->map_id,a->p[0].x,a->p[0].y),stage_position_open_map(a->map_id,a->p[1].x,a->p[1].y),stage_position_open_map(a->map_id,c->drops[0].x,c->drops[0].y));assert(0);}calls++;Campaign other={0};Arena replica;campaign_sync_cycle(c,a,&other,&replica,0);CampaignPacket q;campaign_pack(&q,&other,&replica,77);assert(!memcmp(&p,&q,sizeof p));for(unsigned h=0;h<2;h++){assert(c->pets[h].equipped==other.pets[h].equipped);assert(c->tools[h]==other.tools[h]);}return p;}
static void dig_input(Campaign*c,Arena*a){uint16_t keys[2]={BOW|BOW_EDGE,0};campaign_step(c,a,keys);}
static void real_dig(void){
 Campaign c;Arena a;start(&c,&a,0);assert(!campaign_select_item(&c,0,2));dig_input(&c,&a);assert(!c.dig_timer[0]&&!c.pets[0].owned);
 c.tools[0]=1;assert(campaign_select_item(&c,0,2)&&c.shovel_equipped[0]);dig_input(&c,&a);assert(c.dig_timer[0]==2&&a.p[0].pose==POSE_DIG&&a.p[0].animation==0);
 for(int t=2;t<39;t++){tick(&c,&a);assert(!(c.story_flags&128)&&!c.pets[0].equipped);}
 CampaignPacket p=packet(&c,&a);Campaign guest={0};Arena ga;campaign_unpack(&p,&guest,&ga);assert(guest.shovel_equipped[0]&&ga.p[0].pose==POSE_DIG&&ga.p[0].animation==37);
 tick(&c,&a);assert((c.story_flags&128)&&c.pets[0].equipped==24);for(int t=40;t<=76;t++){tick(&c,&a);}assert(!c.dig_timer[0]);
 assert(campaign_select_item(&c,0,2)&&!c.shovel_equipped[0]);
 /* Existing Buddy advances reward creation to original frame34. */
 start(&c,&a,0);c.tools[0]=1;c.pets[0]=(PetProfile){.owned=1u<<24,.equipped=24};assert(campaign_select_item(&c,0,2));dig_input(&c,&a);
 for(int t=2;t<34;t++){tick(&c,&a);assert(!(c.story_flags&128));}tick(&c,&a);assert(c.story_flags&128);
 /* Taking damage cancels before the success frame; full persistent slots do not mark the buried pet claimed. */
 start(&c,&a,0);c.tools[0]=1;assert(campaign_select_item(&c,0,2));dig_input(&c,&a);a.p[0].hp--;a.p[0].hitstun=5;a.p[0].pose=POSE_HURT;tick(&c,&a);assert(!c.dig_timer[0]);for(int t=0;t<80;t++){tick(&c,&a);}assert(!(c.story_flags&128)&&!c.pets[0].owned);
 start(&c,&a,0);c.tools[0]=1;assert(campaign_select_item(&c,0,2));for(unsigned i=0;i<4;i++){assert(campaign_pet_place(&c,17,400,floor_y(400),1));}dig_input(&c,&a);for(int t=2;t<=76;t++){tick(&c,&a);}assert(!(c.story_flags&128)&&!c.pets[0].owned&&!c.dig_timer[0]);
 memset(c.drops,0,sizeof c.drops);dig_input(&c,&a);for(int t=2;t<=39;t++){tick(&c,&a);}assert((c.story_flags&128)&&c.pets[0].equipped==24);
}
int main(void){
 real_dig();Campaign c;Arena a;start(&c,&a,1);assert(!c.pets[0].owned&&!c.tools[0]);assert(!campaign_dig_buddy(&c,&a,0,1));assert(!c.story_flags);
 c.tools[0]=1;assert(!campaign_dig_buddy(&c,&a,0,0));a.p[0].x=180;assert(!campaign_dig_buddy(&c,&a,0,1));a.p[0].x=155;a.p[0].y=296;assert(!campaign_dig_buddy(&c,&a,0,1));a.p[0].y=286;
 for(unsigned i=0;i<4;i++){assert(campaign_pet_place(&c,PET_SNOOT,300+i*30,286,1));}Campaign old=c;assert(!campaign_dig_buddy(&c,&a,0,1));assert(!memcmp(&c,&old,sizeof c));assert(!campaign_pet_place(&c,31,155,286,1));
 memset(c.drops,0,sizeof c.drops);assert(campaign_dig_buddy(&c,&a,0,1));assert(c.story_flags&128);assert(!campaign_dig_buddy(&c,&a,0,1));assert(c.drops[0].kind==DROP_PET&&c.drops[0].value==(24|32));packet(&c,&a);
 tick(&c,&a);assert(c.pets[0].equipped==24&&!c.pets[1].equipped);assert(pets_owned(&c.pets[0],24)&&pets_owned(&c.pets[1],24));assert(!c.drops[0].kind);packet(&c,&a);
 assert(campaign_pet_place(&c,17,a.p[0].x,a.p[0].y,1));tick(&c,&a);assert(c.pets[0].equipped==17&&pets_owned(&c.pets[1],17));assert(c.drops[0].kind==DROP_PET&&c.drops[0].value==24&&c.drop_age[0]==51);packet(&c,&a);
 for(unsigned t=0;t<50;t++){tick(&c,&a);assert(c.pets[0].equipped==17);}tick(&c,&a);assert(c.pets[0].equipped==24);assert(c.drops[0].value==17&&c.drop_age[0]==51);
 a.p[0].x=400;a.p[0].y=floor_y(400);a.p[1].x=c.drops[0].x;a.p[1].y=c.drops[0].y;for(unsigned t=0;t<51;t++)tick(&c,&a);assert(c.pets[1].equipped==17&&c.pets[0].equipped==24);assert(!c.drops[0].kind);packet(&c,&a);
 start(&c,&a,1);a.p[1].hp=0;c.tools[0]=1;assert(campaign_dig_buddy(&c,&a,0,1));tick(&c,&a);assert(c.pets[0].equipped==24&&!c.pets[1].owned);packet(&c,&a);
 start(&c,&a,0);assert(campaign_pet_place(&c,17,400,floor_y(400),0));for(unsigned t=0;t<700;t++)tick(&c,&a);assert(c.drops[0].kind==DROP_PET&&!c.drop_age[0]);a.p[0].x=400;a.p[0].y=floor_y(400);tick(&c,&a);assert(!c.pets[0].equipped);packet(&c,&a);
 c.pets[0].owned=PET_SUPPORTED_MASK;c.pets[0].equipped=17;c.tools[0]=3;c.completed=3;CampaignPacket p=packet(&c,&a),bad=p;
 {CampaignPacket q;campaign_pack(&q,&c,&a,17);assert(campaign_packet_valid(&q));bad=q;bad.actors[0].hp=(int16_t)((bad.actors[0].hp&2047)|(15<<11));bad.actors[1].hp=(int16_t)((bad.actors[1].hp&2047)|(1<<11));assert(!campaign_packet_valid(&bad)); /* Pet id 31 is not registered. */
  campaign_pack(&q,&c,&a,0);assert(campaign_packet_valid(&q));bad=q;bad.actors[0].hp=(int16_t)((bad.actors[0].hp&2047)|(8<<11));bad.actors[1].hp=(int16_t)((bad.actors[1].hp&2047)|(12<<11));assert(!campaign_packet_valid(&bad));} /* Weapon id 200 is not registered. */
 bad=p;bad.profiles[0].knight=(uint8_t)(bad.profiles[0].knight&~16);assert(!campaign_packet_valid(&bad));bad=p;
 bad=p;bad.drops[0].value=31;assert(!campaign_packet_valid(&bad));bad=p;bad.drops[0].value=17|64;assert(!campaign_packet_valid(&bad));bad=p;bad.drops[0].y=(bad.drops[0].y&511)|(52<<9);assert(!campaign_packet_valid(&bad));
 /* The 14-bit drop X packing must retain late-War coordinates and ignore age. */
 start(&c,&a,1);a.map_id=WAR_MAP;c.phase=WAR_TRAVEL;stage_set_map(WAR_MAP);
 for(unsigned h=0;h<2;h++){a.p[h].x=4200+h*30;for(int y=0;y<448;y++)if(stage_position_open_map(WAR_MAP,a.p[h].x,y)){a.p[h].y=y;break;}}
 int wy=300;
 assert(campaign_pet_place(&c,24,4600,wy,1));c.drop_age[0]=51;campaign_pack(&p,&c,&a,78);Campaign restored={0};Arena restored_arena;campaign_unpack(&p,&restored,&restored_arena);
 assert(restored.drops[0].x==4600&&restored.drops[0].y==wy&&restored.drop_age[0]==51&&restored.drops[0].value==(24|32));
 /* Authored floor ends before4600; validate a legal wide coordinate4200. */
 c.drops[0].x=4200;c.drops[0].y=a.p[0].y;campaign_pack(&p,&c,&a,78);
 assert(campaign_packet_valid(&p));packet(&c,&a);bad=p;bad.drops[0].x=(bad.drops[0].x&~16383)|4608;assert(!campaign_packet_valid(&bad));
 puts("Campaign pets: shovel gate, atomic full slots, shared living unlocks, swaps, ignore clock, persistence and radio checks PASS");return 0;
}




