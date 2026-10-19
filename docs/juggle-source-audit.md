# Aerial light juggling source audit

Source: the locally installed game's MAIN and PLAYER exports. This audit concerns the original first-level humanoids and the DS half-scale, 30-tick simulation. It is not a claim of complete combat parity.

## Contact momentum and float

`assets/main-scripts/scripts/frame_2/DoAction.as`, `f_MeleeImpactSound`, lines15807–15865, contains the contact reaction. Raw actions in `assets/extracted/main.xml` locate its DefineFunction2 name at byte241005 of the enclosing DoAction stream. Relevant offsets below are relative to the function body:

- 0–14: require `hit_impact`.
- 456–470: require `jumping`.
- 471–488: test `punch_num > 3`, custom opcode0x70, then Not/conditional exit.
- 489–504: load `spinning`, custom opcode0x70, then Not/conditional exit.
- 505–515: set `jump_attack=true`.
- 516–568: group12, attack1 sets `speed_jump=-15`; other group12 attacks do not enter that assignment.
- 569–588: groups other than12 call `f_SetFloat(zone,7)`.

**Correction to a preliminary reading:** group12 attack2 skips BOTH the rebound assignment and float7. The float7 call is the ELSE of `punch_group==12`, not the ELSE of `punch_num==1`. Ordinary light group11 qualifies; neither group12 attack should receive generic float7.

Opcode0x70 is a game-specific operation not understood by FFDec. Its logical inversion interpretation is independently supported by `f_CheckDead`: raw `health>0;0x70;Not;If exit` and `CheckHealthPots();0x70;Not;If exit` only make sense as death eligibility at health<=0 with no successful potion. With that interpretation, contact eligibility is `jumping && punch_num<=3 && !spinning`. This is a strongly corroborated bytecode reconstruction, not a recovered definition of the custom opcode from the executable.

For ordinary group11 contact, preserve the attacker's existing vertical velocity and pause it for seven source ticks. There is no generic upward velocity assignment. The DS's former `attacker.vz=4` on every contact was an adaptation that continually renewed upward momentum. `f_SetFloat` at16791 only assigns a timer; `f_Jumping` at16797 decrements that timer instead of advancing vertical motion.

The special group12 attack1 rebound is15source pixels/tick upward, or7.5DS pixels/tick. Integer7 or8 is an explicit rounding approximation. Spinning is excluded from this contact-float branch.

## Victim recoil is separate

`f_MeleeCheckHit`, MAIN around16230, case11 against an ordinary grounded target constructs vertical recoil `-(18+random(3))` and horizontal recoil `random(2)`. The raw case11 branch starts at body offset4522 and calls `f_Damage` with six arguments at4580–4593. Half-scale vertical launch is9–10DS units; the three source outcomes18,19,20 correspond to9,9.5,10.

The generic already-airborne branch around16544 explicitly calls `f_Damage(..., DMGFLAG_JUGGLE,12,-9)`: horizontal6DS units and vertical4.5DS units upward. Integer4 is a conservative rounding approximation; it should not be described as exact4.5. Initial launch and re-hit must not use the same impulse.

Victims use `f_KidToss` rather than the attacker's `f_Jumping`/float timer. The PLAYER `frame_10/PlaceObject2_379_5/CLIPACTIONRECORD onClipEvent(enterFrame).as` invokes `f_KidToss` every tick. There is no source counterpart here for synchronizing a victim's height/velocity or giving it the same six/seven frozen ticks as the attacker.

## Authored light attack events

Clean PLAYER actions provide stronger timing evidence than the damaged MAIN decompile:

| Source pose | Sprite | Hit callback frame | `punching=false` frame | End frame |
|---|---:|---:|---:|---:|
| punch11_1 |690|3|6|8|
| punch11_2 |691|5|7|12|

Paths: `assets/player-scripts/scripts/DefineSprite_690/frame_{3,6,8}/DoAction.as` and `DefineSprite_691/frame_{5,7,12}/DoAction.as`. With zero-based DS animation counters these map to hit2/4 and cancellation5/6. The callback occurs once, not across a three-tick active window. Matching the callback window avoids granting extra late catches. The first light attack has a cancellation marker too.

## Weight and limits

MAIN `f_HumanoidDefaults` at102987 sets baseweight0 and weightplus0. The enemy spawning path at5768 calls that initializer. The first-level barbarian initializer at6010 does not override weight or weightplus, so its values remain0/0. The weightplus1 assignment at11093 belongs to `f_SpawnPlayer`, NOT enemy spawning. Some later enemy types explicitly opt into weightplus1, but this must not be applied to the first-level barbarians. `f_KidToss` at14979–14984 adds `speed_toss_y+current_weight` to height, increments `toss_clock`, and adds weightplus to currentweight every ten ticks. `f_Juggle1Setup` at14742 resets bounces/nohit but does not reset tossclock/currentweight. Standing settings at14699 and the landing branch around15112 reset those values. Therefore accumulating weight survives consecutive airborne hits. The beefy conversion at9947 explicitly sets baseweight4 (two DS units of additional downward displacement per tick). Heavy/beefy behavior additionally has special launch resistance in `f_Juggle1`; it is not appropriate to invent a universal weight category from sprite size.

The DS correction currently omits this accumulating-weight system; ordinary first-level barbarians do not accumulate it in the original either. Preserved attacker momentum, separate target recoil, and authored contact timing address demonstrated mismatches without pretending that omission is exact parity. A hit-count penalty or arbitrary juggle cap is not equivalent to the source's elapsed-airborne-time weight.

A possible future storage reuse is `Fighter.float_timer` while `pose==POSE_AIRHURT`: victims no longer need contact float, so the byte could hold a saturating toss age. This requires preserving the age across airborne hits, clearing it on first launch/landing/recovery, separating hurt physics from ordinary float countdown, and updating `source/protocol.c` validation which currently limits float_timer to11. A saturated byte also bounds the original unbounded timer and must be documented. This is a coordinated change, not a safe isolated field rename. No new field or packet growth is required, but it is not implemented by this audit.
