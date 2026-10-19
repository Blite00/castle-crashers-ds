# LEVEL22 (deer ride and the mill monster) source audit

Step 0 of docs/new-level-workflow.md. Source: `data/levels/level22.pak` (decrypted to `assets/extracted/LEVEL22.swf`), scripts in `assets/level22-scripts`, collision `data/bsps/bsp22.pak`, music `NG36657` (registered as music 6, started at the monster's entrance).

## Route
- LEVEL4 (Thieves' Forest) `f_LeaveLevel2` marks 4, 5 and 46 completed and unlocks **node 22**. MAP links node 4 -> 22 -> 23 -> 102.
- LEVEL22 `f_DockJumpEnd` (when every living player has finished the dock jump): marks 22 completed and visited, unlocks 23 and loads `level23`. Level 23 is the next level after this one (not audited yet; it is where the water and catfish content is expected, to be confirmed from its scripts).

## What kind of level this is
Not a walking level. It is a scripted rail chase on mounts, with no waypoint encounters: `f_Main` only holds a leftover waypoint-1 branch (three barbarians) copied from LEVEL12 that is gated on level 12 being incomplete and the level has no waypoint 1 to trigger it; treat it as dead code, do not port.
- `f_LoadLevelClips`: players, 4 enemy slots (unused), 4 animals, `f_CreateMount3(4)` = four deer mounts (the animal mount type 3). Players start unmounted at `p<N>_start` (walk-to-init) and the deer are placed by `f_MakeMount3`.
- Sprite 234 is the main story clip, sprite 188 the monster/mill clip, sprite 88 the chase loop, sprite 141 the log pattern generator.

## Timeline (frames of sprite 234, then 188 and 88)
1. 234 frame 2: cinematic camera (`f_SetCinematicCamera(guide)`), play. Frame 10: monster clip starts.
2. 188 (mill and monster entrance): explosions and screen shakes at frames 6, 36, 73, 103, 133 (light flash), 175, 187, 196; `s_MillMonster` at 155; deer 2 waits at 196; at 242 the entrance overlay plays "eyeball" and music stops; at 280 to 289 deers 3, 2, 4, 1 turn and play "scared" in that order (280, 283, 285, 289); at 285 the wall breaks (`wallbreak`, entrance overlay "break", big shake 0.5/16, explosion) and music 6 starts; at 80 the pets play "trollscare".
3. 188 frame 305 calls `f_DeerJump`: each living player jumps onto its deer (`temp_speed` = distance/11 over 11 ticks, then `jumptohorse`, `fp_HorseRide`). Frame 316: monster arms switch to frame 2, `camera_chase` starts "chase", pets "follow", and an enterFrame hook calls `f_MonsterMash` every tick.
4. 234 frame 110: mill, woods background and beams play; **`_root.AutoRun = true`** (the deer run by themselves); deer 1..4 `flee`; one of deer 2..4 (random(3)+2) is the pooping deer (`anim_ride deerpoo_walk`, `anim_walk poop`, `flee_poop`) which is used for a gag; HUD wait ends and cinematic camera follows `guide`. Frame 111: guide speed starts at 1, +0.1 per tick, capped at 20 source px/tick; the monster and its two arms move with the guide.
5. **Chase loop (sprite 88)**: scrolling woods, `dust_speed` ramps to 18; `log1/log2/log3` obstacles chosen by sprite 141 from a fixed pattern of 25 frames (log kinds 2..12, second and third logs at x offsets +400, +500, +600, +800 from log1, log1 +300 at frame 21). The loop repeats until sprite 141 frame 26 sets `endloop`; then 88 frame 333 ends the beams and sets `cinema = true`.
6. `f_MonsterMash` (the chasing boss behaviour): every tick, a living player whose x is less than the monster guide x - 75 is "mashed": `monster_mash = true`, jumping, teleported to guide x - 75, punch sound, impact FX at y-50, damage 10 (50 in insane mode), animation `deertoss`. The monster never kills by contact; it keeps flinging the party forward. A dead player has its deer (deerN) continue to run.
7. Finale (88 frames 390 to 563): explosions with shakes of rising strength (0.5, 0.6, 0.7, 0.8, 0.8, 0.9, 0.4), then the monster stops (frames 404 onward the heroes and deer are pushed right at speed up to 18), the woods stop at 437 and `s_MillMonster2` plays; 563 stops.
8. Dock: `f_DockJump` (hook on the dock clip) checks each living player past the dock x: `dockjump`, deer plays `dockjump`, player plays `dockjump`, shadow off; achievement `DeerTrainer` if the player was not mashed (monster_mash false). `f_DockJumpEnd` fades out and changes level when all players have finished.

## Collision and size
Not yet extracted: BSP22 edges and waypoints and the scene origin must be read the way `convert_thieves_geometry.py` did for BSP4. The chase is on a looping scenery band, so the walkable area is the player lane (a few rows) over the scrolling distance; the DS can implement it as a fixed arena with a scrolling background and obstacles instead of a long world.

## Rewards and items
None found in the scripts: no coin bags, chests, weapons or animals are placed (`f_CreateAnimals(4)` is for the deer). The only reward is the level clear (22 completed, 23 unlocked) and the `DeerTrainer` achievement for never being mashed.

## Items still unknown, to fix before coding
1. Deer mount physics: jump height, speed, `deer_walk`, `anim_ride` poses for each knight on the deer (PLAYER clips for `Mount3`; the horse_* clips used for the crocodile are the closest known pattern). The riding animations must be pulled from the original, same as the croc.
2. Obstacle behaviour: log hit boxes, what a log does to a rider (knockdown? jump over?), taken from the log sprites 13/100..140 scripts.
3. BSP22 lane bounds and the size and number of loop repeats before `endloop` (the pattern generator has 25 steps).
4. Exact `f_MonsterMash` position source (`zone.x`) = the guide x, speed ramp as above.
5. Music `NG36657` has to be converted into a chapter bank tail, together with the level's sound effects (`s_MillMonster`, `s_MillMonster2`, explosions, ground thumps).

## DS design (proposal)
Map 14 "Deer chase", its own chapter bank (scene art, deer, rider frames per knight, monster, logs). A fixed-lane arena with a scrolling parallax woods; the camera locks to the guide; the monster is a scripted object. State rides in the existing wire fields (guide speed in `timer`, scene step in `spawned`, mashed bits in `mount_mask`). First milestone: the intro (entrance, scared deer, jump onto deer), then the chase with logs and the mash, then the finale and the dock jump.

## Player count
The DS game is two players only. The source runs four heroes and four deer; this port builds two deer (mount bits 6 and 7 already belong to the two heroes), the guide and the monster mash loop over two heroes, and the HUD and wire carry no third or fourth player.

## Log rules (sprite 132, extracted)
- Each log scrolls left at 18 source px per tick relative to the world; it is reset when it passes x -200 (the pattern in sprite 141 re-places it). Up to three logs (log1..3, each with a front half drawn over the rider).
- Collision is `f_DiagonalCollision(log, hero, 0, 50)`. A log with lane markers (`top`, `bottom`) is a half-blocking log: a hero whose y is above `top` or below `bottom` (outside the gap) is slammed; the gap lane is safe. A log without markers is a full block: a hero is slammed unless it is jumping.
- Slam: pose `deerslam`, pushed back to the log's `hit_x`. (Damage, if any, is in the pose, not in this clip.)
- A deer without a rider avoids logs on its own: shifts 4 px up or down out of the blocked lane, or jumps for full logs.
- The monster (`monster._x`) overruns a log that is behind it: the log explodes (frame 14 `s_Explosion4`, shake 0.7/14) and any hero ahead of the log by less than 75 is moved to log x - 100.
- Background scenery (sprite 259 beams, 13 woods) loops with `endloop` taken from sprite 141 frame 26; sprite 256 randomises an x offset of +-300 per pass (dust and sparks).

## Scene layout (sprite 243 "game", source coordinates, y negative up)
- Start end: mill with the entrance (sprite 88 at x 896), monster 188 at x -885, its arms 190/194 at x -1174/-1018, entrance overlay 208, door1 at x -1153, camera guide 234 at x -788, start markers: p1 (-563,-1019), p2 (-627,-1061), deer1 (-502,-1020), deer2 (-652,-1080) (p3/p4 and deer3/4 are dropped: two players). Edges at x -1396..-1318, topleft (-1146,-1139).
- The chase band: logs log1..3 (sprite 132, 17 frames, with front halves 140), pattern 141, light 135; background woods 13 (241 frames loop) and beams 259 on the bg layers (bg3_c 6, bgwoods 13, bg1_c 15).
- End: the dock at x about 4570..6400 is static art (sprites 17, 19, 21, 23, 25, 27, 29 are the planks, posts and boards), door2 at x 5591, bottomright (5749,-892). Deer jump point is sprite 210 at x 4844 (hook of `f_DockJump`).
- World is therefore about 7800 source px wide (3900 DS) between the edges; the chase scrolls the woods while the guide moves 1..20 source px per tick.

## Deer and rider art (extracted, tools/deer_mount.py and tools/deer_art.py)
- Deer body: `data/game/mount3.pak` (MOUNT3.swf): idle 9, walk 20 (11 frames), poop 29 (3), jump 30 (24), dockjump 31 (74), scared 35, eat 39; shadow sprite 2. 115 frames baked at zoom 0.5 into `assets/deer-mount`.
- Rider on the deer (PLAYER root labels): deer_walk 1032 (11), deerpoo_walk 1033 (3), deertoss 1038 (24), deerslam 1043 (44), deerdie 394 (122, shared with the death clip), runscared 1676 (10), dockjump 1691 (55), jumptohorse 1702 (14). Baked for the blue knight in `assets/deer-blue` (161 frames; the other three costumes use the same call with their name). The rider clips show the rider without the deer in walk; toss, slam and dock include parts of the deer pose, so the body clip and the rider clip are drawn together in the same order as the croc ride block.
- Deer states in game: wait, idle, walk, escape, flee, flee_poop, poop, jump, dockjump, scared, eat (MOUNT3 root labels).

## Static scene bake (done)
`python tools/prepare_campaign_war.py deer` (row `deer`: level 22, scene sprite 243, origin (-1400,-1450), 3900x400) writes `assets/extracted/campaign-deer.xml`; FFDec at zoom 0.5 gives `assets/campaign-deer/1.png` (3900x400). Source SWF must be the normalised `LEVEL22-full.swf` (declared length patched; it has 14 more tags than the raw export, including the `bg0_c` beams layer at root, sprite 261). The static art is only the right end (x 4500..6400 source: the wooden mill wall with wheels, planks and the forest dock); everything else is animated clips: mill 88, monster 188 with arms 190/194, entrance overlay 208, logs 132/140, light 135, intro clip 224, camera guide 234, woods 13, beams 259.

## Status
Ported as map 14 (Mill Run) in v0.65, see docs/v065-qa.md. Ported: intro clip timing and shakes, music at frame 285, deer and rider art, 21 log waves and the three wall kinds, slam, mash toss, log explosions at the monster, finale and dock jump, world-map node. Not ported: entrance overlay (208), monster arms (190, 194), the pooping deer, the DeerTrainer achievement.
