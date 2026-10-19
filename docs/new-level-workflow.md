# New level workflow

Goal: a level that matches the original one to one, with none of the defects Forest Entrance and Thieves' Forest shipped with.
The order below is the order that worked; every step has a gate, and a gate that fails stops the next step.

## 0. Source audit (before any code)
Write `docs/<level>-source-audit.md` from the original SWF (FFDec scripts and the level's `f_Main`). It must list, with sprite and frame ids:
- Size, floor/collision payloads (BSP ids), camera leash points (`f_SetLeash`) and the leftmost x with art.
- Every waypoint and its encounters: enemy kind, count, spawn side (`f_MakeEnemy` x/y), delay, co-op only entries.
- Every scripted scene (clip ids, frames where the camera locks and releases, sounds, shake), vines, ledges, rivers, water.
- Every object: breakable grass/trees/chests (`f_RegisterGrass`, `f_SetChest`, item_type, weapon_type), pickups, animals and mounts (`f_MakeMount2`, `f_SetAnimalTimer`), shop or NPC dialogue.
- Every reward: food drops, coins and gems per enemy (`random_below` tables), weapon and pet unlocks, level clear flags, unlock of the next map node.
- The boss: phases, attacks and their tells, health, drops.
Anything not found in the source is written down as "unknown" (the placeholder weapon names 37 and 42 came from skipping this).

## 1. Registration (tools/level_audit.py)
```
python tools/level_audit.py <map id> <NAME_MAP> [bank]
```
Lists every file a level id has to appear in (stage geometry and size, packet validation, chapter bank in both host and guest, music, objective text, render hooks, protocol limits, the contract test) and the ceilings that still name the previous level. It runs inside `check.ps1` for the newest `*_MAP`, so a half-wired level fails the suite. The wire is fixed at 276 bytes: new level state uses the spare fields the way Thieves' Forest does (`broken`, `npc_ticks`, `spawned`, `timer`, `war.gate_hp`, `mount_mask`), never new packet bytes.

## 2. Collision and camera (tests/level_contract_test.c)
Add a row to `LEVELS` (map, size, hero spawns, exit x, art edge, camera minimum, pinned wall columns, breakable list). It checks:
| Bug seen before | Check |
| --- | --- |
| Nothing rendered left of spawn (Forest Entrance) | walkable ground and camera minimum never lie left of the art edge; spawns are inside the art |
| Cliff acting as floor, floating back across the map | stage geometry is pinned (wall columns); ledges are explicit scripted drops, never open floor |
| Unreachable or sealed ends | flood fill from the spawn must reach the exit |
| Knockback, dash or marching ending inside a wall | random shoves through `stage_move` always end on open ground |
| Chests and grass that cannot be hit | every breakable has open ground within 40 units |
Cliff gaps that need a jump or a drop are listed in the audit with their x range and handled by the ledge table (see Thieves' `ledges[]`): jump up needs the hero to be airborne at the lip, a drop is a scripted fall.

## 3. Art import (render parity)
- Export frames with FFDec at `-zoom 0.5`; freeze nested costume sprites with `prepare_frames.freeze` (knights, starter sword, thief variant).
- Everything a level owns goes in its chapter bank (`tools/pack_chapter_assets.py`), not in ROM: backgrounds, props, actors, scene clips, mount frames, level music tail. A bank plus its music must fit `CHAPTER_ASSET_CAPACITY` (1,040,000 bytes); the packer check fails the build if not.
- Resident ROM data is counted by `check_memory_budget.py`; the margin is the only budget (111,812 bytes after v0.63).
- Foreground layers are cached through the ring pool in `render.c` (`FG_CACHE_POOL`); a level with many big layers must keep per-frame decodes low (frame drop at chests and during running thieves was this).
- Check with a native screenshot harness at the spawn, at the first scripted scene, at the leftmost and rightmost camera positions and during the boss.

## 4. Scripting: scenes, camera, spawns
Use the Thieves' Forest pattern, which fixed every transition error:
- Scenes are a table (`campaign_thieves_scenes.h`): waypoint, art clip, freeze window, release tick, sounds, shake. The camera leash is set by the scene and reset when the phase changes; the leash points come from `WPX(wp+1)`, never from the hero position (the "camera snaps back" and "stays in the previous scene" bugs were stale leashes).
- Enemies never appear mid-screen: they spawn off-screen and `march` in; a 180-tick stuck teleport (`place`) rescues idle ones.
- Vine thieves, ledges, rivers and cutscene actors are table entries with their own tests (`scenes_test`, `break_test`, `mount_test`).
- The camera follow is fixed point with a vertical dead band, so a level does not add its own jitter; use `stage_shake` for scripted shake.

## 5. Enemy AI
- Port each enemy from its source class (attack range, tell frames, damage, cooldown) and give every attack a visible tell as the original does; the Troll Mother wind-up (`war.gate_hp`, exclamation mark) is the model.
- Death uses the species' death clip with all its stages (trolls vanishing instantly was a missing clip).
- Add a bot test that plays the level start to end (`campaign_thieves_test`, forest bot): it catches idle or stuck enemies, unreachable spawns and softlocks. The soft-lock guard releases a scene or leash after a timeout.
- AI that is harder than the first levels (chasing boss, water enemies, catfish) gets its own state table in the level file and a unit test per attack, before it is drawn.

## 6. Treasure, rewards, shops
- Food, coins, gems and weapon drops come from the source tables, through `campaign_queue_drop` / `campaign_weapon_place` (never ad hoc): one table per enemy kind and per breakable (grass one in three drops food 2..6; chests 0..n by `campaign_break_open`).
- Every new weapon goes through the weapon pipeline in `docs/adding-content.md` (the pack lives on the SD card); original names come from the source, a placeholder is a defect to fix.
- Shops and NPCs are map hubs: the blacksmith pattern (`campaign_hubs.c`) with the price and level from `f_WeaponStats`. The level-clear flag, the next node's unlock and the level-clear rewards are in the same audit table and a progress test.
- Reward bookkeeping rides the existing wire (`completed`, `broken`, level clears in the rotating byte); a test must fail if a reward can be taken twice or is lost across host and guest.

## 7. Mounts and riding
The crocodile is the pilot (`campaign_mount_step`, `campaign_croc_place`, `DROP_CROC`, rider frames in the forest bank):
- A mount is a drop kind that outlives its rider; mount bits are published in `mount_mask`; heroes use bits 6/7, enemies use their slot bit.
- Rider art is stored with the mount (idle, walk, punches, bow, jump per costume) in the level's bank, drawn by one block in `render.c` that is guarded by `render_map`.
- A new mount (horse, ostrich, camel...) needs: the original mount clip, the horse_* style rider clips per knight, a mount_test (mount, ride, hit, dismount, rider death), and a wire bit assignment.

## 8. Planned hard cases
- Chasing boss: the boss walks behind a scrolling camera; the camera leash advances with the boss, `stage_camera_min_x` is a function of progress (not a constant), the contract test pins the scripted corridor, the bot test must outrun it. Add a "pressure" counter on the wire rather than a position.
- Water level: water is a collision class, not a picture. Extend the floor tables with a water mask (`stage_water_at`), slow or swim physics through `stage_move`, ripples from `water_ripples`, enemy and pet immunities as flags (the river slow with Dino immunity is the model). The contract test must flood-fill both with and without water.
- Catfish boss and bosses of its kind: phase table, one tell per attack, drops and the exit gate in the audit; the Troll Mother/Barbarian boss tests are the templates; its art lives in its own bank so the boss does not cost resident memory.
- Complex collision: keep the original payloads byte-identical (`export_*_geometry.py`), never hand-edit; barriers that were deliberate go in the pinned list. Multi-layer floors (bridges, stairs) use the stair/riser helpers in `stage.c`; add a test row that walks them.
- More complex AI: write the state machine in a table (state, tell, duration, effect), simulate it headless against a dummy hero in a unit test, and only then draw it.

## 9. Gates, in order
1. `tools/level_audit.py` clean.
2. `tests/level_contract_test.c` row passes.
3. Level unit tests (scenes, breakables, mounts, boss, bot) pass; `check.ps1` full pass count.
4. Chapter bank packs and fits with its music; memory budget gate passes.
5. Native screenshot sweep (spawn, every scene, edges, boss, shop).
6. Normal ROM and level-select test ROM built, `docs/vNNN-qa.md` written listing what was verified and what was not (hardware has never been run).

## Player count
Every level is ported for two players. Where the source loops over four heroes, mounts or spawn slots, cut the loop to two before writing any state; the wire only has room for two.

## Lessons from the Mill Run (LEVEL22)
- Read the BSP extents before laying out the world: BSP22 is a 789x128 box, not the level. An AutoRun level scrolls the scene past a small lane, so the DS world is built from the looping clip (tools/deer_scene.py), not from the BSP.
- Normalise the SWF length first (`LEVEL22-full.swf`): the raw export drops tags, including a whole background layer.
- When everything that moves on its own is a function of the timer (guide, monster, logs, scene cues), the guest needs no new wire fields and tests can run the whole level in milliseconds with bots (tests/campaign_deer_test.c).
- Pick the budget first: scene 287 KB + actors 476 KB + music 240 KB + sfx 20 KB must stay under the 1,040,000 byte bank capacity; resident scratch grew by the largest actor frame (monster, 73 KB decoded), so check `tools/check_memory_budget.py` after every art change.
- `tools/level_audit.py` caught the unregistered audio, protocol and bank entries on the first run; run it before the first ROM build.

## Level end
Every level ends on `WAR_COMPLETE`; `campaign_step` now holds that phase for the score sign (`campaign_tally_step`, 170 ticks) before the world map. A new level only has to set `c->phase=WAR_COMPLETE` and its clear bit. The sign reads the xp and gold baselines taken in `campaign_world_enter` and the per-hero kills the combat tick records, so no per-level code is needed; add the new map to the victory music branch in `audio_update_campaign` if its bank has room for the clip.
