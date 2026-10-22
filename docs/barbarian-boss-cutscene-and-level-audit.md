# Barbarian Boss (LEVEL7, DS map 11): cutscene and level audit, v0.80

Method: native frames of the renderer (tools/render_preview.c scenarios 64 to 87) with the camera printed every step, the source clip scripts (assets/level7-logic-scripts, sprite 202 and the intro sprite), the previous audit (docs/BARBARIAN_BOSS_AUDIT.md) and the checklist of mismatches seen in earlier levels (docs/new-level-workflow.md). Nothing was run on a DS.

## The intro cutscene: what was wrong
Measured on the renderer, before the fix:
- The camera was the ordinary two-fighter camera fed with made-up framing points. It cut twice (x 180 to 359 at tick 36, x 276 to 110 at tick 297), and its zoom changed on almost every tick (372, 424, 404, 392, 380, 368 ... 332). The background is one affine layer, so every zoom step rescales the whole picture: that is the flicker, and the zoomed-out view (up to 424 units wide for a 256 pixel screen) is why the knights and the little barbarians were hard to read.
- The clip's own screen shakes and sounds (sprite 202: frames 14, 51, 72, 91, 119, 127) were not played at all, except for the music.
- The music started at tick 127, but clip frame 127 is tick 160 (the knock starts the clip at tick 35, clip frame f is tick f + 33), so it began 33 ticks before the door falls.

What the intro does now (render.c `boss_intro_camera`, campaign_boss.c, audio.c):
- One fixed zoom (288 units, the closest the engine allows) for the whole scene. The camera holds on the party and the captives, eases to the gate over 60 ticks (starting at tick 10 so it arrives before the knock), holds for the little barbarians and the falling door, eases onto the boss as it leaves the gate (tick 179) and follows it walking in at its own speed, and over the last 40 ticks eases into the ordinary fight framing, so there is no cut when the fight starts.
- Screen shake: 3 short shakes (0.3 s, magnitude 6) at ticks 47, 105, 124 and the big one (0.8 s, magnitude 15) as the door falls at tick 152, applied on top of the scripted camera.
- Sounds at the same ticks: Ground2 (47, 160), Roar2 (84), Explosion2 (105, 124), Explosion3 (152). Ground2 and Explosion2 are best-guess files (the MAIN sound table is not in the extracted scripts), as for the Mill Run. Orchestra1 (tick 153) has no matching file and is left out.
- The boss music now starts at tick 160 (frame 127 of the gate clip).
- Camera numbers after the fix: x 166 to 578, width 288 until tick 259, then the 40 tick ease to the fight width. No jump larger than 11 units per tick.

## Other cutscenes checked
| Scene | Result |
| --- | --- |
| Boss death, treasure, princess, release, duel, kiss (scenarios 66 to 74) | Zoom stays within 288 to 304, no camera jumps; frames read well. |
| Barracks intro (scenario 37) | Follows the original movie camera guide: zoom 308 to 424 and fast pans are the clip's own. Left alone. |
| War horse ambush, machine intro, bridge (scenarios 80 to 82) | Fixed zoom, no jumps. The war clips have no ScreenShake calls in the source. |
| Thieves' Forest scenes, Mill Run entrance | Shake and sound cues were already ported from their clips (docs/v053-qa.md, v065-qa.md). |
| Flooded Castle angry fish | Added in v0.79 with its three sounds. |
Not covered: whether the Keep intro, abduction and princess pans match their movies frame by frame (they were ported from the original camera guides in earlier versions and no new mismatch showed up in the camera numbers).

## The level against the checklist of earlier mismatches
| Earlier problem | Boss level |
| --- | --- |
| Nothing renders left of spawn | Walkable ground starts at x 116, spawns at 214 and 192; now pinned in tests/level_contract_test.c (228 wall columns). |
| Cliff acting as floor, unreachable ends | Flat arena, flood fill from spawn reaches the princess side (contract test). |
| Camera snaps or stays stale | Intro fixed above. Fight camera: focus on the nearest boss edge, zoom 288 to 304 in the phase scenarios. |
| Enemies appearing mid screen | Reinforcements use safe spawn offsets away from both players (audit v0.39). |
| Death clips cut short | Boss death uses sprite 168 with chest cue 73 after the explosion at 65; reinforcements keep the 44 tick corpse clip. |
| Placeholder names or rewards | Reward audit in BARBARIAN_BOSS_AUDIT.md: 29 gold drops from the source table; the level 7 unlocks that have no counterpart in this build are listed there as intentionally absent. |
| Missing sound or shake cues | The gate scene was the gap, now closed. In the fight itself the boss attacks keep the source damage numbers (punch 20, spikes 10, slam 30, flame 10). |
| Frame drops at chests | Gold uses one indexed table with no extra allocations (audit). |

Known adaptations that remain (all listed with their reason in BARBARIAN_BOSS_AUDIT.md): the boss scheduler is a deterministic 45 tick pursuit because the source AI is unreadable, the kiss approach is grounded and automatic by request, the sword lock uses the shared campaign clash counters.

## Tests
`tests/level_contract_test.c` has a Barbarian Boss row. `tests/campaign_boss_test.c` still passes (intro timing, cue timing, snapshots). The shake and cue tables are covered by the same renderer scenarios and the check suite.
