#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>
#include "arena.h"

#define PROTOCOL_MAGIC 0xCA01u
#define PROTOCOL_VERSION 22u
typedef struct {
    uint16_t magic, keys;
    uint16_t knight_id,confirmed;
    uint32_t sequence, attack_count, jump_count, heavy_count, magic_count, bow_count, equip_count;
    uint8_t reserved[28]; /* Room for the guest's profile messages (CampaignRequest), still far below the 227-byte DS reply limit. */
} InputPacket;
typedef struct {
    uint16_t magic, version;
    uint32_t sequence;
    Arena arena;
} StatePacket;
_Static_assert(sizeof(InputPacket) == 64, "input packet layout");
_Static_assert(sizeof(StatePacket) == 8 + sizeof(Arena), "state packet layout");

typedef struct {
    uint32_t sequence, attack_count, jump_count, heavy_count, magic_count, bow_count, equip_count;
    uint16_t keys, pressed,knight_id,confirmed;
    int seen;
} RemoteInput;

/* Wrap-safe comparison; a gap of half the sequence space is ambiguous. */
int protocol_sequence_newer(uint32_t candidate, uint32_t previous);
int protocol_input_valid(const InputPacket *p);
int protocol_state_valid(const StatePacket *p);
/* Caller must exclude interrupts while sharing this state with a callback. */
void protocol_remote_reset(RemoteInput *r);
int protocol_remote_accept(RemoteInput *r, const InputPacket *p);
uint16_t protocol_remote_take(RemoteInput *r);
#endif
