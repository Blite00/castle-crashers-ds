#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "protocol.h"
#include "stage.h"
#include <string.h>

int protocol_sequence_newer(uint32_t candidate, uint32_t previous) {
    uint32_t distance = candidate - previous;
    return distance != 0 && distance < 0x80000000u;
}

/* Campaign maps (map id 3 up, including CAMP_MAP 16 and its 6450 unit width) share the campaign fighter limits below; nothing differs per level. */
int protocol_input_valid(const InputPacket *p) {
    return p && p->magic == PROTOCOL_MAGIC && p->knight_id < KNIGHT_COUNT && p->confirmed <= 1 && !(p->keys & ~(unsigned)INPUT_MASK);
}

int protocol_state_valid(const StatePacket *p) {
    if (!p || p->magic != PROTOCOL_MAGIC || p->version != PROTOCOL_VERSION)
        return 0;
    const Arena *a = &p->arena;
    if (a->map_id >= STAGE_MAP_COUNT || a->running > 1 || a->winner > 3 || a->countdown > 90 || a->clash_timer > 90 || a->clash_cooldown > 180 ||
        a->clash_score[0] > 10 || a->clash_score[1] > 10 || a->clash_contacts > 2) return 0;
    if(a->running&&!arena_players_ready(a))return 0;
    for (int i = 0; i < 2; i++) {
        const Fighter *f = &a->p[i];
        if (!stage_position_open_map(a->map_id,f->x,f->y) ||
            f->y < (a->map_id>=3?0:ARENA_MIN_Y) || f->y > (a->map_id>=3?stage_world_height(a->map_id)-1:ARENA_MAX_Y) ||
            f->z < 0 || f->z > 80 || f->vz < -16 || f->vz > 13 ||
            f->hp < 0 || f->hp > (a->map_id>=3?1200:ARENA_HEALTH) ||
            (f->facing != -1 && f->facing != 1) ||
            f->cooldown < 0 || f->cooldown > 24 ||
            f->flash < 0 || f->flash > 5 ||
            f->pose < POSE_IDLE || f->pose >= POSE_COUNT ||
            f->animation < 0 || f->animation >= 30000 ||
            f->mana < 0 || f->mana > (a->map_id>=3?170:ARENA_MANA) ||
            f->combo < 0 || f->combo > 4 || f->combo_timer < 0 || f->combo_timer > 32 ||
            f->attack_kind < HIT_NONE || f->attack_kind > HIT_STOMP || f->queue_timer > 8 ||
            f->hitstun < 0 || f->hitstun > 15 || f->invulnerable < 0 || f->invulnerable > 30 ||
            f->knockback < -6 || f->knockback > 6 || f->guard < 0 || f->guard > 1 ||
            f->freeze < 0 || f->freeze > ARENA_FREEZE_TICKS || f->burn < 0 || f->burn > ARENA_BURN_TICKS ||
            f->air_spin_used > 2 || f->knight_id >= KNIGHT_COUNT || f->ready > 1 || f->bow_equipped > 1 || f->equip_timer > 30 || f->heavy_chain > 3 || f->float_timer > 11 || f->stun_chain > 5 || f->stun_timer > 45 || f->hit_done > 1 || (f->previous & ~(unsigned)INPUT_MASK) ||
            (f->queued & ~(ATTACK|HEAVY|MAGIC|BOW))) return 0;
    }
    for (int i = 0; i < ARENA_SHOTS; i++) {
        const Projectile *s = &a->shots[i];
        int cannon=a->map_id==6&&s->owner==8&&s->kind==SHOT_ARROW; /* Ram cannonball: vz carries the landing x, life 48..8 flight, then the blast. */
        if (s->life < 0 || s->life > 48 || s->fraction < 0 || s->fraction > 1) return 0;
        if (cannon?(s->vz<0||s->vz>=stage_world_width(a->map_id)||s->z<0||s->z>300||s->vx<-25||s->vx>25):(s->vz < -43 || s->vz > 5)) return 0;
        if (!s->life) continue; /* Expired projectiles can be offscreen. */
        /* Maps 3 and above, including DEER_MAP (14), share the campaign limits below. */
        if (cannon) {if (s->x<0||s->x>=stage_world_width(a->map_id)||s->y<0||s->y>=stage_world_height(a->map_id)||(s->vx==0&&s->life>8)) return 0; continue;}
        if (s->x < 0 || s->x >= stage_world_width(a->map_id) || s->y < (a->map_id>=3?0:ARENA_MIN_Y) || s->y > (a->map_id>=3?stage_world_height(a->map_id)-1:ARENA_MAX_Y) ||
            s->z < 0 || s->z > 113 || (s->kind==SHOT_ARROW ? (a->map_id>=3?(s->vx<-25||s->vx>25||s->vx==0):(s->vx != -16 && s->vx != 16)) : !(((s->vx==5||s->vx==-5)&&s->vz==0&&s->fraction==0)||((s->vx==13||s->vx==-13)&&s->vz==-9))) ||
            s->owner < 0 || s->owner > (a->map_id>=3?8:1) || s->kind < 0 || s->kind > SHOT_POISON) return 0;
    }
    return 1;
}

void protocol_remote_reset(RemoteInput *r) {
    memset(r, 0, sizeof(*r));
}

int protocol_remote_accept(RemoteInput *r, const InputPacket *p) {
    if (!protocol_input_valid(p) ||
        (r->seen && !protocol_sequence_newer(p->sequence, r->sequence)))
        return 0;
    /* Unsigned subtraction also detects counters wrapping through zero.
       Several taps arriving between ticks coalesce into one action edge. */
    if ((uint32_t)(p->attack_count - r->attack_count)) r->pressed |= ATTACK;
    if ((uint32_t)(p->jump_count - r->jump_count)) r->pressed |= JUMP;
    if ((uint32_t)(p->heavy_count - r->heavy_count)) r->pressed |= HEAVY;
    if ((uint32_t)(p->magic_count - r->magic_count)) r->pressed |= MAGIC;
    if ((uint32_t)(p->bow_count-r->bow_count)) r->pressed |= BOW;
    r->bow_count=p->bow_count;
    if ((uint32_t)(p->equip_count-r->equip_count)) r->pressed |= EQUIP;
    r->equip_count=p->equip_count;
    r->sequence = p->sequence;
    r->attack_count = p->attack_count;
    r->jump_count = p->jump_count;
    r->heavy_count = p->heavy_count;
    r->magic_count = p->magic_count;
    r->keys = p->keys;
    r->knight_id=p->knight_id;r->confirmed=p->confirmed;
    r->seen = 1;
    return 1;
}

uint16_t protocol_remote_take(RemoteInput *r) {
    uint16_t keys = r->keys | r->pressed;
    if(r->pressed&ATTACK)keys|=ATTACK_EDGE;
    if(r->pressed&HEAVY)keys|=HEAVY_EDGE;
    if(r->pressed&JUMP)keys|=JUMP_EDGE;
    if(r->pressed&BOW)keys|=BOW_EDGE;
    if(r->pressed&EQUIP)keys|=EQUIP_EDGE;
    r->pressed = 0;
    return keys;
}
