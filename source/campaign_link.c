#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_link.h"
int campaign_request_valid(const CampaignRequest *p){
    if(!p||p->magic!=CAMPAIGN_REQUEST_MAGIC||p->version!=CAMPAIGN_VERSION||p->operation<1||p->operation>5||p->slot>(p->operation==CAMPAIGN_REQUEST_LOADED?9:3)||(p->operation!=CAMPAIGN_REQUEST_LOADED&&(p->reserved0[0]||p->reserved0[1])))return 0;
    if(p->operation==CAMPAIGN_REQUEST_LOADED&&(p->reserved0[0]<3||p->reserved0[0]>16||(p->reserved0[1]!=0&&p->reserved0[1]!=CAMPAIGN_WORLD_MAP)))return 0;
    if(p->operation==CAMPAIGN_REQUEST_LOADED){unsigned bank=p->reserved0[1]==CAMPAIGN_WORLD_MAP?3:p->reserved0[0]==16?9:p->reserved0[0]==15?8:p->reserved0[0]==14?7:p->reserved0[0]==13?6:p->reserved0[0]==12?5:p->reserved0[0]==11?4:p->reserved0[0]>=8?3:p->reserved0[0]>=6?2:1;if(p->slot!=bank)return 0;}
    if(p->operation!=CAMPAIGN_REQUEST_PROFILE)return 1;
    return campaign_character_valid(&p->character);
}
int campaign_request_apply(CampaignRequestState *state,const CampaignRequest *p,Campaign *c,Arena *a){
    if(!campaign_request_valid(p)|| (state->seen&&!protocol_sequence_newer(p->sequence,state->sequence)))return 0;
    state->seen=1;state->sequence=p->sequence;
    int accepted=0;
    if(p->operation==CAMPAIGN_REQUEST_PROFILE){if(!a->running){campaign_character_put(&p->character,c,1);accepted=1;}}
    else if(a->running&&p->operation==CAMPAIGN_REQUEST_STAT)accepted=campaign_spend(c,a,1,p->slot);
    else if(a->running&&p->operation==CAMPAIGN_REQUEST_ITEM){accepted=campaign_select_item(c,1,p->slot);if(accepted)a->p[1].equip_timer=30;}
    else if(a->running&&p->operation==CAMPAIGN_REQUEST_LOADED)accepted=1;
    /* Acknowledging rejected requests lets the UI recover without retransmits.
       One outstanding request per guest, 4-bit ack carried in unused bow event. */
    c->menu_ack[1]=p->sequence&15;return accepted;
}
