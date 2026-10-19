#ifndef CAMPAIGN_FEEDBACK_H
#define CAMPAIGN_FEEDBACK_H
#include "campaign.h"
/* Local presentation only: no save/network fields. Zero initialization is valid. */
typedef struct {
    uint8_t notice[2],combo[2],up[2]; /* up: ticks left of the 60-tick level-up beam */
    uint8_t knight[2],level[2],seen;
    uint8_t phase,phase_seen;
} CampaignFeedback;
_Static_assert(sizeof(CampaignFeedback)<=64,"Feedback memory budget");
void campaign_feedback_reset(CampaignFeedback *state);
/* Observe authoritative profiles on every update. tick only advances the150-tick
 * display timers; duplicate render/network updates must pass zero. Returns1 once
 * on a safe phase entry if the local ready hero has unspent attribute points.
 * Call reset outside active campaign, including before loading another save. */
int campaign_feedback_update(CampaignFeedback *state,const Campaign *campaign,const Arena *arena,unsigned localhero,int tick);
/* Thresholds 2/4/8/16/32/50; NULL means no newly unlocked supported combo. */
const char *campaign_feedback_combo_controls(unsigned threshold);
#endif
