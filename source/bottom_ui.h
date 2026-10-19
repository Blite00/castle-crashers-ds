#ifndef BOTTOM_UI_H
#define BOTTOM_UI_H
#include "campaign.h"
#include "campaign_feedback.h"
typedef struct {
 const Arena *arena;const Campaign *campaign;const CampaignFeedback *feedback;
 unsigned item_cursor,hero,fps,work_ms,save_status;int host,client,practice,difficulty,save_dirty,audio_ok,peer_loading,guest_known;
} BottomUi;
void bottom_ui_base(uint16_t *pixels,const Arena *arena,const Campaign *campaign);
void bottom_ui_game(uint16_t *pixels,const BottomUi *ui);
/* World-map card controls:0 previous,1 enter,2 next;-1 outside. */
int bottom_ui_map_hit(int x,int y);
#endif
