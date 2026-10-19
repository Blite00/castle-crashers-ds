#include "campaign_link.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static CampaignRequest loaded(uint32_t sequence,unsigned bank,unsigned map,unsigned phase){CampaignRequest r={.magic=CAMPAIGN_REQUEST_MAGIC,.version=CAMPAIGN_VERSION,.sequence=sequence,.operation=CAMPAIGN_REQUEST_LOADED,.slot=bank,.reserved0={map,phase}};return r;}
/* Model only main's matching-scene release rule; all wire validation,
   ordering and acknowledgements run the real campaign_link implementation. */
static void deliver(CampaignRequestState*s,const CampaignRequest*r,Campaign*c,Arena*a,unsigned stamp,int*waiting){if(campaign_request_apply(s,r,c,a)&&r->operation==CAMPAIGN_REQUEST_LOADED&&(((unsigned)r->reserved0[0]<<8)|r->reserved0[1])==stamp)*waiting=0;}
int main(void){
 _Static_assert(sizeof(CampaignRequest)==64&&sizeof(CampaignRequest)==sizeof(InputPacket),"DS reply is the 64-byte input packet");_Static_assert(offsetof(CampaignRequest,character)==16,"header is 16 bytes");
 Campaign c={0};Arena a;arena_init(&a);campaign_start(&c,&a,1);CampaignRequestState state={0};int waiting=1;
 CampaignRequest r=loaded(1,1,5,0);assert(campaign_request_valid(&r));deliver(&state,&r,&c,&a,5u<<8,&waiting);assert(!waiting&&c.menu_ack[1]==1);
 waiting=1;deliver(&state,&r,&c,&a,(5u<<8)|CAMPAIGN_WORLD_MAP,&waiting);assert(waiting&&c.menu_ack[1]==1); /* Duplicate cannot release new transition. */
 r=loaded(3,3,5,CAMPAIGN_WORLD_MAP);deliver(&state,&r,&c,&a,(5u<<8)|CAMPAIGN_WORLD_MAP,&waiting);assert(!waiting&&c.menu_ack[1]==3);
 waiting=1;r=loaded(2,1,5,0);deliver(&state,&r,&c,&a,6u<<8,&waiting);assert(waiting&&state.sequence==3&&c.menu_ack[1]==3); /* reordered older datagram */
 r=loaded(4,1,5,0);deliver(&state,&r,&c,&a,6u<<8,&waiting);assert(waiting&&c.menu_ack[1]==4); /* newer but obsolete scene is acked, never releases current */
 r=loaded(5,2,6,0);deliver(&state,&r,&c,&a,6u<<8,&waiting);assert(!waiting&&c.menu_ack[1]==5);
 r=loaded(6,3,6,CAMPAIGN_WORLD_MAP);assert(campaign_request_valid(&r)); /* World map uses hubs even when map retainsWar. */
 for(unsigned map=3;map<=11;map++){
  unsigned bank=map<=5?1:map<=7?2:map==11?4:3;r=loaded(10+map,bank,map,0);assert(campaign_request_valid(&r));
  r.slot=(bank+1)%5;assert(!campaign_request_valid(&r));
  r=loaded(10+map,3,map,CAMPAIGN_WORLD_MAP);assert(campaign_request_valid(&r));r.slot=1;assert(!campaign_request_valid(&r));
 }
 const CampaignRequest good=loaded(30,2,6,0);r=good;r.reserved0[0]=2;assert(!campaign_request_valid(&r));r=good;r.reserved0[0]=12;assert(!campaign_request_valid(&r));r=good;r.reserved0[1]=1;assert(!campaign_request_valid(&r));r=good;r.slot=5;assert(!campaign_request_valid(&r));
 r=good;r.operation=CAMPAIGN_REQUEST_STAT;assert(!campaign_request_valid(&r)); /* scene fields reserved forOP5 only */
 state=(CampaignRequestState){.sequence=UINT32_MAX,.seen=1};r=loaded(0,2,6,0);assert(campaign_request_apply(&state,&r,&c,&a)&&c.menu_ack[1]==0);assert(!campaign_request_apply(&state,&r,&c,&a));r.sequence=UINT32_MAX;assert(!campaign_request_apply(&state,&r,&c,&a));
 /* The first character request must be acknowledged before readiness begins;
    the following loader request has a different acknowledgement nibble. */
 a.running=0;state=(CampaignRequestState){0};c.menu_ack[1]=0;r=(CampaignRequest){.magic=CAMPAIGN_REQUEST_MAGIC,.version=CAMPAIGN_VERSION,.sequence=1,.operation=CAMPAIGN_REQUEST_PROFILE};campaign_character_default(&r.character,1);
 assert(c.menu_ack[1]!=(r.sequence&15));assert(campaign_request_apply(&state,&r,&c,&a)&&c.menu_ack[1]==1);
 a.running=1;r=loaded(2,1,5,0);assert(c.menu_ack[1]!=(r.sequence&15));assert(campaign_request_apply(&state,&r,&c,&a)&&c.menu_ack[1]==2);
 a.running=0;state=(CampaignRequestState){0};r=loaded(1,1,5,0);assert(!campaign_request_apply(&state,&r,&c,&a)&&c.menu_ack[1]==1); /* no premature lobbyready */
 puts("Campaign loading:36-byte requests,scene/bank validation,reordering,duplicates,ack and sequence wrap PASS");return 0;
}
