# LEVEL20 (the level after Flooded Castle) source audit

Step 0 of docs/new-level-workflow.md. Source: `data/levels/level20.pak` (decrypted to `assets/extracted/LEVEL20.swf`, length patched in `LEVEL20-full.swf`, XML `level20-full.xml`), scripts `assets/level20-scripts`, collision `bsp20.pak` (`assets/extracted/bsp20.bin`), music `NG90532` (slot 25, level) and `DRUMLIFT` (slot 37, boss and cinematic), rain loop `s_Rain`, bats `s_Bats`. **Name: Tall Grass Field** (not in the extracted scripts; confirmed from the Castle Crashers wiki: the bear boss fight is in Tall Grass Field, Rammy is the orb after the boss, Hawkster is found with the horn outside the huts after the first wave of bears, Burly Bear sits behind a boulder near the end behind a bombable wall; the bears use wind balls and tornadoes). Two players only.

## Route
- LEVEL23 (Flooded Castle) `f_EnemyDie` unlocks node 20. Map links (MAP f_MapInit, worldmap-nodes.json): 22 -> 23 -> 102 -> **20** -> 21. Node 20 sits at source map (584,768); world-map crop (128,736): DS (456,32), so the map crop has to be extended upward before the node can be drawn.
- `f_LeaveLevel` (left door `door1`) and `f_LeaveLevel2` (right door `door2`) only mark visited and exit; `f_LeaveLevel3` (the right door `door3` at the far end) marks **20 completed and visited and unlocks 21**, stops `s_Rain` and `s_Bats`.
- Three portals: `door1` (-4114,-98) leads back, `door2` (5082,-299), `door3` (7626,-567). Player spawn sets per entrance: `p*_1` (-3800,-98), `p*_2` (4789,-285), `p*_3` (7360,-569), `p*_4` (421,-288) used for the boss walk-in, `p*_5` (6154,-566) used for the end cinematic. Edge markers `edge_left` (-4433,-43), `edge_right` (7702,-95).

## Scene
Root: `sky`, parallax `bg3_c`, `bg2_c`, `bg1_c`, `game` (sprite 387 -> 386), `bg0_c`, and a full-screen `rain` clip. Sprite 386 (`game`, 3 frames) is the world: x -4700..8200, y -800..100 source units (12,900 x 900), so 6,450 x 450 DS at half scale (origin (-4700,-800)). Frame 2 holds only the collision edge clips (`n_type` 1/2/3, 400 and 700); frame 3 is empty. Native render at 0.12 zoom: a beach with the king's boat on the far left (x -3800), a long grassy plain with bear huts and rocks (to x ~4800), a stone fortress with a ladder up its side (x 5000..7600, floor at y -570 on the top level) and the exit door at the top right.
- Collision: 86 BSP edges, kinds 2 (floor edge), 3 (kind 3 at (-3799,-139) and (-3765,4): steps), **400** (x 5032..5126 and 7571..7665: wall and ledge clips of the fortress), **700** (x 5424..5500 and 5496..5601: the ladder). Floor extremes x -4151..8110, y -657..4.
- Waypoints WP0..WP21 (x,y): (-3891,-98) (-3593,-157) (-3213,-225) (-2848,-223) (-2369,-218) (-1993,-219) (-731,-209) (-80,-224) (1019,-224) (1463,-224) (2233,-230) (2681,-231) (3084,-259) (3576,-224) (4104,-228) (4554,-228) (5085,-298) (5264,-159) (5668,-582) (6801,-568) (7173,-568) (7624,-564).
- Level functions (`f_Init`): `fp_FunctionLine = f_SideLadder`, `fp_FunctionLineProjectile = f_SideLadderProjectile`: the ladder is a general side-ladder mechanic of MAIN (`f_SideLadder`, line 4078 of main-scripts frame 2). Ladder markers `sl_bot0` (5448,-207), `sl_top0` (5548,-567).
- Level clips: 12 enemy slots, 6 animals, 1 horse, 1 mount (`f_CreateMount2`), 10 bats (`f_CreateBats(10)`), sky, FX 200, rain.

## Waypoint events (`f_Main`)
| WP | Event |
| --- | --- |
| 4 | leash to WP4; `p_game.gotoAndPlay()` (hut scene), kills goal +4 |
| 6 | leash to WP6; bears: level-2 at x+100, level-2 at x+250 (two positions), plus one level-2 per active player; kills +3 (+players) |
| 7 | leash WP7 + 300 right; `p_game.gotoAndPlay()` |
| 9 | leash WP9; two beefy bears arrive flying in with a propeller (`propeller` clip, `propeller_timer` 80 and 60, `speed_toss_x` 13 and 12): the first at WP9 y, the second at camera + 250 and WP9 y + 50 (or y -244 when x < 2600); plus two level-3 bears (x-250 and x-100); kills +4 |
| 14 | leash WP14; level-3 bears from the left: two at WP12 -100 and -200, plus one per player (-(200 + n*50)); kills +2 (+players) |
| 18 | marks the waypoint only |
| 20 | left edge set; `s_Bats`; seven bats from the right (x WP20 + 500, 550, 600, 650, 675, 720, 770; y WP20 -10, +20, -20, +30, 0, -15, +15) |
Not in `f_Main` but in the scene clips: `boss` (sprite 368, at (769,-346)) spawns a bear at `camera right + 150`, y + random(200), level 2, aggressiveness 4, speed +2 whenever fewer than four enemies are active (the boss fight's reinforcement stream); two cannon-like clips (sprite 333 at (3196,-306) and (3262,-268), `shot_timer` 4 and 1, `f_EnemyAttack` of the level fires the eight shots in sequence while the camera is in range 600 to the left); `hawkster` (sprite 342, an animal, at (-1479,-334): states idle, hit, pull, juggle, die); `mud` (sprite 193); `campfire` (265); a `bossdoor` (199) at (4946,-302); two trees registered with `f_RegisterTree` (sprite 51 at 22 and 50); pickups: weapon 54 and weapon 55 (sprite 140 at (-3918,-107) and (831,-266)), weapon 62 at `animal_pt`/`LevelItems` (an animal orb at (425,-363)), a helmet reskin object (`test_reskin_helm20`, `helmetval = 20`, offscreen).

## Enemies
- **Bear** (`f_SpawnBear`, MAIN line 6189): the shared humanoid (`e_human`) with helmet/emblem/shield 20 (the "bear" costume), weapon 29 or 26, health 100 (`f_SetEnemyHealth`), speed 2..4.9, aggressiveness 7, blocks (odds 11), arrows (power 8), punch power 10/12/14/17, dash jump and roll chances, magic power 8 of `f_SetPlayerMagic(helmet 20)`, resists 60/40/40/60/70. Level 2 adds magic (delay 90, wait 60+random(60), air 2), aggressiveness 8, speed +1; level 3 adds splash magic 2, aggressiveness 6, speed +1, health +20; **level 10 is `f_BadDude`** (the boss).
- **Beefy** bears: `f_MakeEnemyBeefy` (MAIN line 9928) on a bear, entering with the propeller clip.
- **Bats**: the level's own `f_SpawnBat` / `f_BatFlyLeft` / `f_HitBat` / `f_BatDie` (same family as Flooded Castle).
- **Boss** (sprite 374 `boss_cam` clip, 276 frames; frame 2 starts `DRUMLIFT`): a level-10 bear spawned at `p_game.boss` position (769,-346) with `flag 4`, speed 6, `weightplus 1`, **magician** with splash magic and weapon magic type 2, punch powers +5, **health +150 per player**, `f_HitReactionSwordLock` as its hit reaction, `fp_DeathAction = f_CloseBossHealth` (restores `NG90532`, kills the reinforcements). The party walks to `p*_4` first (cinematic camera `guide`); frame 121 shows the health meter and releases the party.
- End cinematic: sprite 244 (237 frames, labels `boss_intro` 2, `wait` 12, `follow` 21, `finish` 110) driven by `camera_target` (7096,-560) and `cinema_cam` (373, 170 frames: `boss_intro`, `finish`, `s1` 131, `s2` 266): triggers when `camera_target` reaches the screen's right edge, walks the party to `p*_5` and plays the story with seven characters.

## What this level needs (new engine work)
1. **Wide world** 6,450 x 450 DS with three vertical layers (plain, fortress and the top deck at y -570) joined by one side ladder; BSP edges 400 and 700 are special.
2. **Bear enemy**: a new species of the shared humanoid AI (arrows, magic, blocks) with a bear costume (PLAYER helmet 20 with weapons 26 and 29), levels 2, 3, beefy and propeller entry.
3. **Bats**: reuse the Flooded Castle bat art and AI family, but flying over land (no grab).
4. **Boss** bear with magic, sword-lock and reinforcement stream; cinematic walk-in.
5. **End cinematic** (sprite 244) and the exit door.
6. Scenery props: huts, cannon-clips, trees, campfire, mud, animals (hawkster, animal orb), weapon pickups 54, 55 and 62 (original names needed).
7. World map: the node, the extended crop, the link 102 -> 20 -> 21 and the clear bit.
8. Banks: the scene is as wide as Thieves' Forest; its own bank and music tail (level music `NG90532`, boss `DRUMLIFT`, victory is the usual one) must fit 1,040,000 bytes.

## Unknown (stated, not guessed)
- The level's name; the names of weapons 54, 55, 62 and 26/29; what `p_game.gotoAndPlay()` at WP4 and WP7 starts (it plays frames of sprite 386 beyond frame 1, but those frames only hold collision clips, so the effect is probably an unseen trigger in a clip loaded at run time); sprite 244's story text (no dialogue strings in the scripts); the exact meaning of edge kinds 400 and 700 beyond "ladder and fortress edges".

## Plan
1. Registration (stage, bank, campaign dispatch, map node 10, audio, tests, contract row) with the scene baked and the collision converted; bears using the existing humanoid enemy AI with a bear costume.
2. Waves per the table above, bats, propeller entry, ladder.
3. Boss, cinematic and rewards.
4. Cutscene audit and a full playthrough bot test.
