#ifndef CAMPAIGN_LINK_H
#define CAMPAIGN_LINK_H
#include "campaign_save.h"
#include "protocol.h"
#define CAMPAIGN_REQUEST_MAGIC 0xCA04u
enum { CAMPAIGN_REQUEST_PROFILE=1,CAMPAIGN_REQUEST_EQUIP,CAMPAIGN_REQUEST_STAT,CAMPAIGN_REQUEST_ITEM,CAMPAIGN_REQUEST_LOADED };
typedef struct {uint16_t magic,version;uint32_t sequence;uint8_t operation,slot,reserved0[2];CampaignCharacter character;} CampaignRequest;
_Static_assert(sizeof(CampaignRequest)==sizeof(InputPacket),"Campaign request fits DS reply");
typedef struct {uint32_t sequence;int seen;} CampaignRequestState;
int campaign_request_valid(const CampaignRequest *p);
int campaign_request_apply(CampaignRequestState *state,const CampaignRequest *p,Campaign *c,Arena *a);
#endif
