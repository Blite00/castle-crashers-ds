#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_feedback.h"
#include <string.h>
void campaign_feedback_reset(CampaignFeedback *s){if(s)memset(s,0,sizeof(*s));}
const char *campaign_feedback_combo_controls(unsigned threshold){
    switch(threshold){case 2:return "B, B";case 4:return "Run + A / X";case 8:return "A, X";case 16:return "A, A, X";case 32:return "A, A, X, X";case 50:return "A,A,A,X (Magic 20)";default:return NULL;}
}
int campaign_feedback_update(CampaignFeedback *s,const Campaign *c,const Arena *a,unsigned localhero,int tick){
    if(!s)return 0;
    if(!c||!a||!a->running){campaign_feedback_reset(s);return 0;}
    for(unsigned h=0;h<2;h++){
        unsigned bit=1u<<h,level=c->profiles[h].level,knight=c->profiles[h].knight;
        if(tick&&s->up[h])s->up[h]--;
        if(tick&&s->notice[h]){s->notice[h]--;if(!s->notice[h])s->combo[h]=0;}
        if(!a->p[h].ready||level<1||level>99){s->seen&=(uint8_t)~bit;s->notice[h]=s->combo[h]=s->up[h]=0;continue;}
        if(!(s->seen&bit)||s->knight[h]!=knight||level<s->level[h]){
            s->notice[h]=s->combo[h]=0;
        }else if(level>s->level[h]){
            static const uint8_t thresholds[6]={2,4,8,16,32,50};
            s->notice[h]=240;s->combo[h]=0;s->up[h]=60;
            for(unsigned i=0;i<6;i++)if(s->level[h]<thresholds[i]&&level>=thresholds[i])s->combo[h]=thresholds[i];
        }
        s->seen|=(uint8_t)bit;s->knight[h]=(uint8_t)knight;s->level[h]=(uint8_t)level;
    }
    /* Defer the entry latch until the local profile is present; a joining client
     * can receive a world-map snapshot before its selected hero is ready. */
    if(localhero>1||!a->p[localhero].ready)return 0;
    int entry=!s->phase_seen||s->phase!=c->phase;
    s->phase=c->phase;s->phase_seen=1;
    return entry&&(c->phase==CAMPAIGN_WORLD_MAP||c->phase==BOSS_COMPLETE)&&campaign_points(c,localhero)>0;
}
