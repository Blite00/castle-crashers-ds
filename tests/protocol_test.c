#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "protocol.h"

static void state_validation(void) {
    StatePacket s = {.magic=PROTOCOL_MAGIC, .version=PROTOCOL_VERSION};
    arena_init(&s.arena);
    assert(protocol_state_valid(&s));
    assert(!protocol_state_valid(NULL));
    StatePacket good = s;
    s.magic = 0; assert(!protocol_state_valid(&s)); s = good;
    s.version++; assert(!protocol_state_valid(&s)); s = good;
    s.arena.running = 2; assert(!protocol_state_valid(&s)); s = good;
    s.arena.winner = 4; assert(!protocol_state_valid(&s)); s = good;
    s.arena.countdown = 91; assert(!protocol_state_valid(&s)); s = good;
#define BAD(field, value) do { s.arena.p[i].field = value; \
    assert(!protocol_state_valid(&s)); s = good; } while (0)
    for (int i = 0; i < 2; i++) {
        BAD(x, ARENA_MIN_X-1); BAD(x, ARENA_MAX_X+1);
        BAD(y, ARENA_MIN_Y-1); BAD(y, ARENA_MAX_Y+1);
        BAD(z, -1); BAD(z, 81); BAD(vz, -17); BAD(vz, 14);
        BAD(hp, -1); BAD(hp, ARENA_HEALTH+1); BAD(facing, 0); BAD(facing, 2);
        BAD(cooldown, -1); BAD(cooldown, 25); BAD(flash, -1); BAD(flash, 6);
        BAD(pose, -1); BAD(pose, POSE_COUNT);
        BAD(animation, -1); BAD(animation, 30000);
        BAD(mana, -1); BAD(mana, ARENA_MANA+1);
        BAD(combo, -1); BAD(combo, 5); BAD(combo_timer, -1); BAD(combo_timer, 33);
        BAD(attack_kind, -1); BAD(attack_kind, HIT_STOMP+1); BAD(queue_timer, 9);
        BAD(hitstun, -1); BAD(hitstun, 16); BAD(invulnerable, -1); BAD(invulnerable, 31);
        BAD(knockback, -7); BAD(knockback, 7); BAD(guard, -1); BAD(guard, 2);
        BAD(freeze, -1); BAD(freeze, 31); BAD(burn, -1); BAD(burn, ARENA_BURN_TICKS+1);
        BAD(air_spin_used, 3); BAD(knight_id, KNIGHT_COUNT); BAD(ready, 2); BAD(bow_equipped, 2); BAD(equip_timer, 31); BAD(heavy_chain, 4); BAD(float_timer, 12); BAD(stun_chain, 6); BAD(stun_timer, 46);
        BAD(previous, 512); BAD(queued, JUMP); BAD(hit_done, 2);
    }
#undef BAD
    for (int i = 0; i < ARENA_SHOTS; i++) {
        Projectile active = {.x=120,.y=ARENA_MIN_Y,.z=14,.vx=5,.life=48};
        s.arena.shots[i] = active; assert(protocol_state_valid(&s));
#define BAD_SHOT(field, value) do { s.arena.shots[i].field=value; \
    assert(!protocol_state_valid(&s)); s.arena.shots[i]=active; } while (0)
        BAD_SHOT(x, -1); BAD_SHOT(x, WORLD_WIDTH);
        BAD_SHOT(y, ARENA_MIN_Y-1); BAD_SHOT(y, ARENA_MAX_Y+1);
        BAD_SHOT(z, -1); BAD_SHOT(z, 114); BAD_SHOT(vx, 0);
        BAD_SHOT(fraction, -1); BAD_SHOT(fraction, 2); BAD_SHOT(vz, -44); BAD_SHOT(vz, 6);
        BAD_SHOT(life, -1); BAD_SHOT(life, 49);
        BAD_SHOT(owner, -1); BAD_SHOT(owner, 2); BAD_SHOT(kind, -1); BAD_SHOT(kind, SHOT_POISON+1);
#undef BAD_SHOT
        s.arena.shots[i].life=0;s.arena.shots[i].x=260;
        assert(protocol_state_valid(&s));s=good;
    }
    for(int kind=SHOT_ICE;kind<=SHOT_POISON;kind++)if(kind!=SHOT_ARROW){
        for(int facing=-1;facing<=1;facing+=2){
            s.arena.shots[0]=(Projectile){.x=200,.y=285,.z=50,.fraction=1,.vx=13*facing,.vz=-9,.life=47,.kind=kind};
            assert(protocol_state_valid(&s));
            s.arena.shots[0].vz=-8;assert(!protocol_state_valid(&s));
            s.arena.shots[0].vz=-9;s.arena.shots[0].vx=5;assert(!protocol_state_valid(&s));
        }
    }
    s=good;
    s.arena.running=1;s.arena.p[1].ready=0;assert(!protocol_state_valid(&s));s=good;
    arena_lobby_init(&s.arena);assert(protocol_state_valid(&s));s=good;
    s.arena.map_id=65535;assert(!protocol_state_valid(&s));s=good;
    s.arena.clash_timer=91;assert(!protocol_state_valid(&s));s=good;
    s.arena.clash_score[1]=11;assert(!protocol_state_valid(&s));s=good;
    s.arena.clash_cooldown=181;assert(!protocol_state_valid(&s));s=good;
    s.arena.clash_contacts=3;assert(!protocol_state_valid(&s));s=good;
    for(int slot=0;slot<ARENA_SHOTS;slot++)s.arena.shots[slot]=(Projectile){.x=220,.y=285,.z=113,.vx=16,.life=42,.owner=slot%2,.kind=SHOT_ARROW,.vz=-1,.fraction=1};
    s.arena.p[0].float_timer=s.arena.p[1].float_timer=11;
    assert(protocol_state_valid(&s));s=good;
    arena_start_round(&s.arena);s.arena.countdown=0;s.arena.p[0].z=80;s.arena.p[0].bow_equipped=1;
    for(int tick=0;tick<90;tick++){
        uint16_t keys[2]={tick==0?BOW:0,0};arena_step(&s.arena,keys);
        assert(protocol_state_valid(&s));
    }
    s=good;
    /* Validate actual gameplay snapshots, including rematches and jumps. */
    unsigned rng = 7;
    for (int i = 0; i < 60000; i++) {
        if (!s.arena.running || s.arena.winner) {
            arena_init(&s.arena); s.arena.running = 1;
        }
        rng = rng * 1664525u + 1013904223u;
        uint16_t keys[2] = {((rng >> 12) & 511)|((rng&1)?BOW:0)|((rng&4)?EQUIP:0), ((rng >> 23) & 511)|((rng&2)?BOW:0)|((rng&8)?EQUIP:0)};
        arena_step(&s.arena, keys);
        assert(protocol_state_valid(&s));
    }
}

int main(void) {
    assert(protocol_sequence_newer(1, 0));
    assert(!protocol_sequence_newer(1, 1));
    assert(!protocol_sequence_newer(0, 1));
    assert(protocol_sequence_newer(0, UINT32_MAX));
    assert(!protocol_sequence_newer(UINT32_MAX, 0));
    assert(!protocol_sequence_newer(0x80000000u, 0));
    RemoteInput r; protocol_remote_reset(&r);
    InputPacket p = {.magic=PROTOCOL_MAGIC, .sequence=100};
    assert(protocol_input_valid(&p));
    assert(!protocol_input_valid(NULL));
    p.magic=0; assert(!protocol_remote_accept(&r, &p)); p.magic=PROTOCOL_MAGIC;
    p.keys=512; assert(!protocol_remote_accept(&r, &p)); p.keys=0;
    assert(protocol_remote_accept(&r, &p)); assert(protocol_remote_take(&r)==0);
    p.sequence++; p.attack_count=1; p.jump_count=1;
    assert(protocol_remote_accept(&r, &p));
    /* A later released reply must preserve unconsumed taps. */
    p.sequence++; assert(protocol_remote_accept(&r, &p));
    assert(protocol_remote_take(&r)==(ATTACK|JUMP|ATTACK_EDGE|JUMP_EDGE));
    assert(protocol_remote_take(&r)==0);
    p.attack_count=10;
    assert(!protocol_remote_accept(&r, &p));
    p.sequence--; assert(!protocol_remote_accept(&r, &p));
    assert(protocol_remote_take(&r)==0);
    p.sequence+=2; p.keys=LEFT|BLOCK;
    assert(protocol_remote_accept(&r, &p));
    assert(protocol_remote_take(&r)==(LEFT|BLOCK|ATTACK|ATTACK_EDGE));
    assert(protocol_remote_take(&r)==(LEFT|BLOCK));
    /* Reset permits a rebooted guest's lower sequence, including first tap. */
    protocol_remote_reset(&r); p.sequence=0; p.keys=0; p.attack_count=1; p.jump_count=0;
    assert(protocol_remote_accept(&r, &p)); assert(protocol_remote_take(&r)==(ATTACK|ATTACK_EDGE));
    protocol_remote_reset(&r); p.sequence=UINT32_MAX; p.attack_count=UINT32_MAX;
    assert(protocol_remote_accept(&r, &p)); (void)protocol_remote_take(&r);
    p.sequence=0; p.attack_count=0;
    assert(protocol_remote_accept(&r, &p)); assert(protocol_remote_take(&r)==(ATTACK|ATTACK_EDGE));
    assert(protocol_remote_take(&r)==0);
    p.sequence++; p.heavy_count++; p.magic_count++;
    assert(protocol_remote_accept(&r, &p));
    assert(protocol_remote_take(&r)==(128u|256u|HEAVY_EDGE));
    assert(protocol_remote_take(&r)==0);
    p.sequence++;p.bow_count++;p.keys=BOW;
    assert(protocol_remote_accept(&r,&p));assert(protocol_remote_take(&r)==(BOW|BOW_EDGE));
    assert(protocol_remote_take(&r)==BOW);
    p.sequence++;p.bow_count++;
    assert(protocol_remote_accept(&r,&p));assert(protocol_remote_take(&r)==(BOW|BOW_EDGE));
    p.sequence++;p.equip_count++;p.keys=EQUIP;
    assert(protocol_remote_accept(&r,&p));assert(protocol_remote_take(&r)==(EQUIP|EQUIP_EDGE));
    assert(protocol_remote_take(&r)==EQUIP);
    assert(!protocol_remote_accept(&r,&p));assert(protocol_remote_take(&r)==EQUIP);
    p.sequence++;p.equip_count++;
    assert(protocol_remote_accept(&r,&p));assert(protocol_remote_take(&r)==(EQUIP|EQUIP_EDGE));
    p.sequence++;p.knight_id=KNIGHT_GREEN;p.confirmed=1;
    assert(protocol_remote_accept(&r,&p));assert(r.knight_id==KNIGHT_GREEN&&r.confirmed);
    p.sequence--;p.knight_id=KNIGHT_BLUE;p.confirmed=0;
    assert(!protocol_remote_accept(&r,&p));assert(r.knight_id==KNIGHT_GREEN&&r.confirmed);
    p.sequence+=2;p.knight_id=KNIGHT_COUNT;
    assert(!protocol_remote_accept(&r,&p));p.knight_id=KNIGHT_RED;p.confirmed=2;
    assert(!protocol_remote_accept(&r,&p));p.confirmed=0;
    assert(protocol_remote_accept(&r,&p));assert(r.knight_id==KNIGHT_RED&&!r.confirmed);
    /* A release and repress entirely between simulation ticks still spins once. */
    RemoteInput taps;protocol_remote_reset(&taps);
    InputPacket jump={.magic=PROTOCOL_MAGIC,.sequence=1,.keys=JUMP,.jump_count=1};
    Arena jumping;arena_init(&jumping);arena_start_round(&jumping);jumping.countdown=0;
    assert(protocol_remote_accept(&taps,&jump));
    uint16_t jump_keys[2]={protocol_remote_take(&taps),0};arena_step(&jumping,jump_keys);
    jump.sequence++;jump.keys=0;assert(protocol_remote_accept(&taps,&jump));
    jump.sequence++;jump.keys=JUMP;jump.jump_count++;assert(protocol_remote_accept(&taps,&jump));
    jump_keys[0]=protocol_remote_take(&taps);arena_step(&jumping,jump_keys);
    assert(jumping.p[0].air_spin_used==1);
    for(int tick=0;tick<4;tick++){jump_keys[0]=protocol_remote_take(&taps);arena_step(&jumping,jump_keys);}
    assert(jumping.p[0].pose==POSE_AIRSPIN&&jumping.p[0].attack_seq==1);
    assert(!protocol_remote_accept(&taps,&jump));
    for(int tick=0;tick<5;tick++){jump_keys[0]=protocol_remote_take(&taps);arena_step(&jumping,jump_keys);}
    assert(jumping.p[0].attack_seq==1);
    state_validation();
    puts("PASS: packet validation, ordering, wraparound, action edges, lobby reset, 60000 valid snapshots");
}

