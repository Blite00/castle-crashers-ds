#include "campaign_feedback.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hero(Campaign*c,Arena*a,unsigned h,unsigned k,unsigned l){a->p[h].ready=1;a->p[h].knight_id=k;a->p[h].hp=80;c->profiles[h]=(CampaignProfile){.knight=k,.level=l};}
int main(void){
 Campaign c={0};Arena a={0};CampaignFeedback s={0};a.running=1;c.phase=KEEP_FIGHT;hero(&c,&a,0,0,1);
 assert(!campaign_feedback_update(&s,&c,&a,0,1));assert(!s.notice[0]);
 /* Observe a real damage-awarded XP transition. */
 c.profiles[0].xp=keep_xp_threshold(1);campaign_award_hit(&c,&a,0);assert(c.profiles[0].level==2&&campaign_points(&c,0)>0);
 assert(!campaign_feedback_update(&s,&c,&a,0,1));assert(s.notice[0]==240&&s.combo[0]==2);
 for(unsigned i=0;i<20;i++){assert(!campaign_feedback_update(&s,&c,&a,0,0));}assert(s.notice[0]==240);
 campaign_feedback_update(&s,&c,&a,0,1);assert(s.notice[0]==239);
 c.phase=KEEP_ABDUCTION;assert(!campaign_feedback_update(&s,&c,&a,0,0));
 c.phase=CAMPAIGN_WORLD_MAP;assert(campaign_feedback_update(&s,&c,&a,0,0));assert(!campaign_feedback_update(&s,&c,&a,0,1));
 c.stats[0][0]=2;assert(!campaign_feedback_update(&s,&c,&a,0,0));c.stats[0][0]=0;assert(!campaign_feedback_update(&s,&c,&a,0,0));
 c.phase=BOSS_KISS;assert(!campaign_feedback_update(&s,&c,&a,0,0));c.phase=BOSS_COMPLETE;assert(campaign_feedback_update(&s,&c,&a,0,0));assert(!campaign_feedback_update(&s,&c,&a,0,0));
 /* Guest arrival establishes its profile, but can request the safe menu. */
 campaign_feedback_reset(&s);a.p[1].ready=0;c.phase=CAMPAIGN_WORLD_MAP;assert(!campaign_feedback_update(&s,&c,&a,1,0));
 hero(&c,&a,1,2,8);assert(campaign_feedback_update(&s,&c,&a,1,0));assert(!s.notice[1]&&!s.combo[1]);assert(!campaign_feedback_update(&s,&c,&a,1,0));
 hero(&c,&a,1,3,15);campaign_feedback_update(&s,&c,&a,1,0);assert(!s.notice[1]);
 a.p[1].ready=0;campaign_feedback_update(&s,&c,&a,0,0);hero(&c,&a,1,3,20);campaign_feedback_update(&s,&c,&a,1,0);assert(!s.notice[1]);
 /* Loading a save outside the active campaign cannot invent a level-up. */
 a.running=0;campaign_feedback_update(&s,&c,&a,1,0);a.running=1;hero(&c,&a,1,3,30);assert(campaign_feedback_update(&s,&c,&a,1,0));assert(!s.notice[1]);
 campaign_feedback_reset(&s);c.phase=KEEP_FIGHT;hero(&c,&a,0,1,3);campaign_feedback_update(&s,&c,&a,0,0);
 c.profiles[0].level=4;campaign_feedback_update(&s,&c,&a,0,0);assert(s.combo[0]==4&&s.notice[0]==240);
 c.profiles[0].level=9;campaign_feedback_update(&s,&c,&a,0,0);assert(s.combo[0]==8&&s.notice[0]==240);
 c.profiles[0].level=10;campaign_feedback_update(&s,&c,&a,0,0);assert(!s.combo[0]&&s.notice[0]==240);
 for(unsigned i=0;i<240;i++){campaign_feedback_update(&s,&c,&a,0,1);}assert(!s.notice[0]&&!s.combo[0]);
 campaign_feedback_reset(&s);hero(&c,&a,0,0,1);c.phase=CAMPAIGN_WORLD_MAP;assert(!campaign_feedback_update(&s,&c,&a,0,0));assert(!s.notice[0]);assert(!campaign_feedback_update(&s,&c,&a,2,0));
 assert(!strcmp(campaign_feedback_combo_controls(2),"B, B"));assert(!strcmp(campaign_feedback_combo_controls(4),"Run + A / X"));assert(!strcmp(campaign_feedback_combo_controls(8),"A, X"));assert(!campaign_feedback_combo_controls(3));
 assert(sizeof(s)<=64);printf("Campaign feedback: %zuB, real hit XP, duplicates, joins/loads, combos and safe stats entry PASS\n",sizeof(s));return 0;
}
