# LEVEL23 (Flooded Castle: swimming, logs, the Catfish) source audit

Step 0 of docs/new-level-workflow.md. Source: `data/levels/level23.pak` (decrypted to `assets/extracted/LEVEL23.swf`, length patched in `LEVEL23-full.swf`), scripts in `assets/level23-scripts`, collision `bsp23.pak`, music `007_FLOODED_CASTLE` (slot 27), boss music `015_CORN_BOSS` (slot 32), victory `Waterflame_RaceVictory` (slot 4). Two players only.

## Route
- LEVEL22 `f_DockJumpEnd` unlocks node 23 (done in v0.65). LEVEL23 `f_LeaveLevel2` and the boss death (`f_EnemyDie`, line 801) mark 23 completed and visited and unlock **node 20**, the next level.
- Map links: node 22 -> 23 -> 102 (node 102 then leads to 20).

## What the level is
A side-scrolling water level. The party swims (`f_SwimStand`/`f_SwimWalk` replace the normal stand and walk), can jump onto drifting **logs** and ride them, and fights a fixed set of water enemies before the Catfish boss.
- `f_LoadLevelClips`: 25 enemy slots, 4 animals, FX, and **20 bats** (`f_CreateBats(20)`: scenery).
- **Logs** (`f_LogWaitCheck`, `f_LogFollow`, `f_LogStand`, `f_LogWalk`, `f_LogDrift`): a log within 50 across and 20 deep of a jumping player, with positive jump speed, becomes that player's mount (`horse`; `fp_HorseRide = f_LogRide`, jump speed 12). Riders stand or walk on it with the `logstand` and `logwalk` clips, and the log leaves a wake. Unridden logs drift left at 4 per tick and reset ahead of the camera when they pass x -100.
- **Rocks** (`f_RockSpawn`, `f_RockScroll`): up to four rocks scroll from the right edge at speed -12. A swimmer that is not airborne (`body_y > -20`) within 100 behind a rock and 10 lanes of it is hit: knocked off the log (`f_KnockOff`), pushed to the rock's lane edge, damage 5 (50 in insane), juggle launch.
- **Fat fish** (`f_SpawnFatFish`): up to two (`fish1`, `fish2`), health 60, speed 4, chases a random player, punch power 12, resists melee 50, fire 60, electric 50, poison 60, ice 60. Five hits make it charge. Each hit bleeds for 20 ticks. When killed it drifts left at 12 and counts as a kill once it leaves the screen.
- **Shark fins** (`f_SpawnSharkFin`): up to six fins (`fin1..6`) that enter at speed 40 from either side, with a pause timer and a patrol (`f_SharkFin`).
- **Catfish boss** (`f_Boss`, `f_HitCatFish`, `f_EnemyDie` lines 748-end): not yet read in full; it has its own hit handler (`damage_pow, damage_type, damage_flags, recoil`), a death scene that completes the level, and the boss music.
- Waypoint 1 (when level 23 is not yet completed) spawns three barbarians around WP2: `+100,0`, `+200,+50`, `+250,-30`, kills goal +3, with a leash from WP0 to WP3. Waypoint 2 starts the story clip.

## Needs (new engine work)
1. **Water**: swim poses for all four knights (PLAYER swim clips: `swim_stand`, `swim_walk`, splash and `big_splish` FX, wake FX), a water mask in collision, slower movement, no ground jump.
2. **Log mounts**: the croc mount pattern (campaign_mount_step) extended with drift, jump-on, and the wake, with `logstand` and `logwalk` rider poses from PLAYER.
3. **Scrolling hazards** (rocks, shark fins): deterministic from the timer like the Mill Run logs, so the guest needs no extra wire fields.
4. **Fat fish**: a large enemy with the five-hit charge; reuse the species table.
5. **Catfish boss**: its phases, attacks and tells must be read from `f_Boss` and the CATFISH clips before anything is built.
6. Art: the flooded castle scene, fish, fins, rocks, logs, bats, catfish, swim poses. Budget the chapter bank first (the Mill Run bank is at its limit; this level gets its own bank and music tail).

## Next steps
Read `f_Boss`/`f_HitCatFish`/`f_EnemyDie` and the story clip at waypoint 2; extract BSP23 (lane, waypoints); bake the static scene with `prepare_campaign_war.py` (new `flood` row) and the swim clips; then build in this order: swimming and logs, rocks and fins, fat fish, the boss, the score sign and the world-map node.
