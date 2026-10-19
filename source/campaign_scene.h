#ifndef CAMPAIGN_SCENE_H
#define CAMPAIGN_SCENE_H
#include "arena.h"
/* Keep identity, selected item, health and event counters; discard combat
 * transients so a room transition cannot carry a stun, queued attack or dig. */
static inline void campaign_scene_fighter(Fighter *p){
 *p=(Fighter){.x=p->x,.y=p->y,.hp=p->hp,.mana=p->mana,.facing=1,
  .pose=p->hp?POSE_IDLE:POSE_DEAD,.knight_id=p->knight_id,.ready=p->ready,
  .bow_equipped=p->bow_equipped,.attack_seq=p->attack_seq,.hit_seq=p->hit_seq,
  .block_seq=p->block_seq,.magic_seq=p->magic_seq,.arrow_seq=p->arrow_seq};
}
static inline void campaign_scene_arena(Arena *a){
 a->clash_timer=a->clash_cooldown=a->clash_contacts=0;a->clash_score[0]=a->clash_score[1]=0;a->countdown=0;
}
#endif
