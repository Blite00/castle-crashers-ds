# Castle Crashers starting combat audit

Audited 2026-10-04 from the locally extracted original MAIN and PLAYER assets. This is a source audit, not a claim that every move below is already implemented in the DS prototype.

DS mapping used here: **A light, X heavy, B jump, R block, L magic modifier**. A sequence means separate presses during the attack chain window.

## Starting moves and unlocks

| Requirement | DS input | Original result / evidence |
|---|---|---|
| Level1 | A, A, A, A | Four ground light attacks, `punch1_1` through `punch1_4`. Third light tests combo slot0, which is already unlocked at level1. Fourth follows third without another gate. |
| Level1 | X, X, X | Ground heavy chain `punch2_1`, `punch2_2` (or weapon-specific2b), `punch2_3`. All three are ungated. Third attack is a downward ground slam. |
| Level1 | B then repeated A | Air light alternation `punch11_1`, `punch11_2`; no level gate. |
| Level1 | B then X, X, X, then more X | Air heavy starts `slamdown` (group12/1), then `punch2_2` (12/2), then changes to alternating air slashes (group13). No level gate on this chain. |
| Level1 | X near a downed living enemy | Contextual `stomp`, before normal ground-heavy combo dispatch. |
| Level1 | X extremely close to a grabbable enemy | Contextual throw through `f_StompRange` / `fp_ThrowAction`; no level gate. Beefy targets and particular boss restrictions apply. |
| Level2 | B, then B while sufficiently high | `topspin`, combo slot1, requires original `body_y < -75` (over37.5 DS world pixels). This is an airborne spin action, not a universal second vertical jump. |
| Level4 | Running A or X | `dashpunch` / `dashslam`, combo slot2; requires original dashing + truespeed + grounded conditions. |
| Level8 | A, X; or B, A, X | `f_Uppercut`, combo slot3. Aerial behavior also depends on current height. |
| Level16 | A, A, X | `punch5_1`, combo slot4. |
| Level32 | A, A, X, X | Followup `punch5_2`, combo slot5. |
| Level50 | Elemental chain then X, or its extended light followup | From group6/1, heavy gives `punch4_4`; from group6/3, light gives `punch6_4`, both combo slot6. Entry into group6 is independently magic-stat gated. |

Do not call the level8 uppercut or level2 spin a default level1 move. Conversely, restricting the basic ground heavy chain to two attacks omits a genuine starting move.

## How the damaged decompile was checked

Readable source: `castle-arena/assets/main-scripts/scripts/frame_2/DoAction.as`:

- `f_UpdatePlayerAttributes`, lines11350–11380: combo thresholds1,2,4,8,16,32,50. Magic unlocks are separate, based on base magic stat: slots3/0/4/1/2 require1/5/10/15/20 respectively.
- `f_PunchSet1`, line98443; `f_PunchSet2`, line98539.
- `f_TopSpinInit`, line16879; `f_JumpAction`, line16900, dispatches the airborne repeat-jump to it.

FFDec omits custom push operations in these conditions and prints malformed `§§pop()` expressions. The indices above were recovered from **original `assets/extracted/main.xml` DoAction actionBytes**, not guessed from that text. Constant-pool entry696 is `combo_unlocks`; custom actionA2 pushes the member index before GetMember. Offsets in the largest DoAction byte stream:

| Byte offset | Function / condition | Index |
|---|---|---|
|0x453D8|PunchSet1 third light|0 (encoded double0)|
|0x3E8B6|TopSpinInit|1|
|0x45087,0x45B68|PunchSet1/2 dash|2|
|0x4599F,0x45CE4|PunchSet2 air/ground uppercut|3|
|0x45D28|PunchSet2 two-light heavy|4|
|0x45E8A|PunchSet2 group5 followup|5|
|0x45308,0x45DEC|PunchSet1/2 group6 followups|6|

A convenience pcode export now exists at `assets/main-pcode/scripts/frame_2/DoAction.pcode`, but its substituted Jump instructions also hide these custom operands. Raw XML remains the authoritative index evidence.

## Third ground heavy: animation, hitbox, damage

`assets/extracted/player.xml` rootframe29 `punch2_3` places body sprite608 at(1.45,-0.4) source pixels. Its authored scripts:

- `assets/player-scripts/scripts/DefineSprite_608/frame_4/DoAction.as`: ground-slam dust, screen shake(0.3,6), SlamGround sound, and `fp_PunchHit`.
- frame10: clears punching.
- frame12: sets end_attack and stops.

At hitframe4, sprite608 `punch_pt` is sprite18 with center(61,-53.25), scales(2.32135,2.2026672). Sprite18 contains17, which contains rectangular shape16 scaled(0.5961609,0.7898102); shape16 bounds are2080x1570 twips. `f_PunchHit` uses the marker width/height divided by2 about its origin. Therefore the effective source rectangle is approximately143.925x136.566 pixels, centered(62.45,-53.65) after body translation. At half scale: **x[-4.756,67.206], vertical[-60.966,7.316] relative to the foot**, before facing flip and target/depth checks. This is a screen-plane attack box; it does not replace the separate floor-lane collision test.

MAIN `f_MeleeCheckHit`, case2/num3, line16149: `f_Damage(target,punch_pow_medium,attack_type,DMGFLAG_JUGGLE,1,70)`. Positive70 is downward recoil in original body-Y coordinates. Heavy1 uses medium damage; heavy2 uses low. Original base formulas in attributes are low=floor(3+level*.1+strength), medium=floor(5+level*.1+strength*1.15); defense/resistance are applied separately. Do not interpret70 as an upward launch height.

## Contextual stomp and throw

`f_StompRange`, line98228, checks living targets with strict **abs(dx)<60, abs(dy)<20** original units, hence **30,10 DS world pixels**. A target marked `onground` enables stomp. The normal-heavy dispatch reaches this only while the attacker is grounded. The decompiled helper itself misleadingly prints `body_y<0`: pcode contains `Less2; Unknown_70; Not` there, so do not copy the uncorrected AS conditional into C. The surrounding grounded dispatch independently establishes the intended normal-heavy behavior.

A nearby non-grounded target can instead be thrown when grabbable, not beefy, strict abs(dx)<30 original units (15 DS), and attacker bodyY lies between target bodyY-20 and target bodyY+60. Target nohit/airborne eligibility and the level43 boss exception also apply. This is not a general throw on every close enemy.

PLAYER rootframe45 `stomp` places body734. `assets/player-scripts/scripts/DefineSprite_734/frame_9/DoAction.as` calls `f_StompHit`. Unlike the initial selector, the hit scans **all living nearby enemies** (and eligible opposing players), not just the downed target that enabled it. `f_StompCheckHit`, MAIN line100638, uses the same strict feet-distance box; there is no forward-facing requirement. It clears target.nohit, applies **medium damage, DMG_MELEE, DMGFLAG_JUGGLE, horizontal recoil8..12, vertical recoil-9..-11**, then flips the victim. Damage flags and argument order were checked in raw custom-push bytes in function0x4D0AB, not solely the corrupted AS call.

## Prototype adaptations and limits

The DS game runs simulation at30ticks/s with compressed original animation samples, half-scale world geometry, and adapted integer collision/knockback. Exact source attack timing, markers and unlock conditions should guide implementation; copying source recoil numbers directly would not establish equivalent physics. Existing DS defensive recovery, invulnerability and input buffering are adaptations. The source has additional stat-gated magic chains and context rules beyond this bounded default-combo work. R and L retain their shield and magic meanings; this audit does not assign undocumented R+attack combinations or unlock all magic at level1.
