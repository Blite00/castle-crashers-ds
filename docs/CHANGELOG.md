## v0.52 Frame rate, camera, ram, combos, level-up, lobby

See docs/v052-qa.md. ROM: castle-arena-v0.52.nds; ZIP: releases/castle-arena-v0.52.zip.

## v0.51 Thieves' Forest

The next story level after Forest Entrance: the original forest walk with trolls, imps, vine thieves and the Troll Mother, a new world-map node, the Troll club, the Troll and Dino pets and coin bags. See docs/v051-qa.md. ROM: castle-arena-v0.51.nds; ZIP: releases/castle-arena-v0.51.zip.

## v0.50 Scalability foundation

Weapon art streams from the SD card per weapon, the save and network formats hold the whole game's weapons, pets and levels, and docs/adding-content.md says how to add more. See docs/v050-qa.md. ROM: castle-arena-v0.50.nds; ZIP: releases/castle-arena-v0.50.zip.

## v0.49 Owlet fruit and the crocodile mount

Owlet now fetches fruit from the forest's tree and brings it to you, and the first thief of Forest Entrance rides the original crocodile. See docs/v049-qa.md. ROM: castle-arena-v0.49.nds; ZIP: releases/castle-arena-v0.49.zip.

## v0.48 Memory headroom and forest weapons

Menu and arena music now load from NitroFS, freeing about 390 KB beyond the multiplayer reserves. Forest Entrance now has its two weapons: the Leaf blade lies on the road and the Thief sword is a shovel secret. See docs/v048-qa.md. ROM: castle-arena-v0.48.nds; ZIP: releases/castle-arena-v0.48.zip.

## v0.47 Bow, Owlet and map path

Thieves now drop the bow sometimes, as in the original (first chance in Barbarian War); the Owlet pet waits at the end of Forest Entrance; the world-map path to the forest bends through the original junction. See docs/v047-qa.md. ROM: castle-arena-v0.47.nds; ZIP: releases/castle-arena-v0.47.zip.

## v0.46 Forest Entrance

A new world-map node after the Barbarian Boss opens Forest Entrance, the original LEVEL18: the burned road beyond the war, the opening wave with a croc-thief spot, the ambush cutscene (King, litter and the princess-carrying thieves; barbarians drop from the trees), the archer wave, the final thief wave and the bush exit that completes the level and returns to the map. Two-player scaling follows the source. See docs/forest-entrance-source-audit.md. Not yet in: the crocodile mount drawing and Thieves' Forest itself. ROM: castle-arena-v0.46.nds; ZIP: releases/castle-arena-v0.46.zip.

## v0.45 boss meter in the info panel

The original-style boss meter now sits inside the bottom-screen info panel under the boss name, replacing the "BOSS n / m" line and numbers. Pause, countdown, clash, burial and result messages still replace it with their text. ROM: castle-arena-v0.45.nds; ZIP: releases/castle-arena-v0.45.zip.

## v0.44 original boss health meter

The Barbarian Boss and War Machine health meter on the bottom screen now uses the original game's ornate wrought-iron frame, angry-face medallion and glossy red bar (MAIN sprite 312), scaled to fit beside the Stats square; the red portion empties as the boss is hurt. Non-hot logic files are now compiled for size, which restores multiplayer memory headroom to 4,868 bytes beyond the reserves (it was 420 in v0.43). ROM: castle-arena-v0.44.nds; ZIP: releases/castle-arena-v0.44.zip.

## v0.43 Barbarian Boss AI and cut-down hint

The Barbarian Boss scheduler now follows the original `f_BarbarianBoss` bytecode (the decompiler's inverted comparisons were resolved from raw p-code): timers only run while he walks or stands; he walks to his spot beside a hero and punches every 60 ticks; at or below half health he may instead turn and breathe fire (90-tick cooldown); every 90 walking ticks he alternates a shield slam toward a hero in front (reach 225, lane 10) and reverse spikes (reach 250, lane 5). Reinforcements arrive at most once per 60 walking ticks and are capped at 1 above 70% boss health, 2 at or below 70% and 3 at or below 40%. After the chest opens, a snipping scissors icon now floats beside the bound princess until her rope is cut. ROM: castle-arena-v0.43.nds; ZIP: releases/castle-arena-v0.43.zip.

## v0.42 no forced run

Removed the invented "forced run" before the War Machine. The original game never takes control to auto-run the heroes forward past enemies and bomb them; the DS replayed an unused ambush movie (LEVEL11 sprite 363) that nothing in normal play triggers. Players now walk from waypoint 10 to the machine on foot and the machine introduction (walls breaking, machine entering) still plays as in the source. Includes all v0.41 wall, gate and War Machine repairs. ROM: castle-arena-v0.42.nds; ZIP: releases/castle-arena-v0.42.zip.

## v0.41 Barbarian War repairs

The four cinema fence walls the War Machine breaks no longer stay frozen on their first explosion frame. They now play the original break (burst, flying planks, debris), then disappear from the palisade as in the source (sprite 160 is a 32-frame animation that ends empty).

The wooden gate at the end of the level now behaves like the original `f_HitDoor` door: it shows the five damaged door frames at 90/75/50/25 percent and the destroyed frame at zero health, shoves right for four ticks on each melee hit, throws 1/2/2/3/5 original `door_shrapnel` wood chips by damage level, and plays the hit sound. The destroyed door remains as rubble when it opens. Feedback is derived from the synchronized gate health, so no protocol or save change.

The War Machine fight now uses the source leash: heroes and camera are bounded to waypoint 11 .. waypoint 13 + 80 (world 2042..2754), not out to the machine's spawn point (3279). The machine follows the source cycle: ram left until it passes the camera by 90 units, ram right until it passes the camera, track 75..125 units right of a random player with 0.25 acceleration up to speed 2 for 400 ticks (with the source summon timing), then shake for 46 ticks. It no longer chases vertically while tracking. Its three barrels reload every 90 ticks (staggered because the DS has two shot slots), each aimed at a player. Heroes previously could run out to the spawn point; they no longer can.

Not changed: horse mounting in the pre-machine ambush (see docs/v041-qa.md), the second hero/ally AI, and real hardware validation. Protocol 37, arena 22, save 4 and the 276-byte packet are unchanged. ROM: castle-arena-v0.41.nds; ZIP: releases/castle-arena-v0.41.zip.

## v0.40 boss movement

The Barbarian Boss uses its original idle pose when stationary, including at the end of its entrance. Walking follows actual horizontal pursuit or legal half-speed depth movement and uses the source36-tick cycle. The ram likewise idles when its tracking distance is satisfied rather than playing movement in place. Attack movement and ram charge speeds are unchanged.

New input-stepped regressions verify left/right pursuit, depth movement, stationary poses, entrance stop, ram tracking/charges and guest position/pose synchronization. Boss and War gameplay suites pass. Protocol37, save4 and both wireless/runtime memory reserves remain unchanged. Real hardware FPS and physical multiplayer remain unverified. The existing43-second trailer represents v0.39.

ROM: castle-arena-v0.40.nds; ZIP: releases/castle-arena-v0.40.zip.

## v0.39 campaign progression and encounter repairs

Campaign destinations unlock in order: Home Castle → Castle Keep → Barbarian War → Barbarian Boss. Home Castle now has its own clear milestone. Entry is checked by the campaign logic as well as shown on the map; direct entry requests cannot bypass the lock. Blacksmith and its hub rooms remain accessible. Cleared levels can be replayed, and normal continuous transitions still proceed without forcing the map.

Existing later completion implies the earlier stages were cleared, so older saves keep their earned access. Save migration retains characters, stats, gold, equipment and pets. Both consoles must use v0.39: campaign protocol37 carries the additional milestone without enlarging the276-byte snapshot. Arena protocol22 is unchanged.

Barbarian War now keeps the ram within reachable combat bounds and advances it during its introduction. The ambush runs on foot instead of forcing every player onto a horse, and releases control after the landing. This does not yet implement optional horse mounting. The encounter camera stops scrolling before the hero reaches the fight boundary, preventing enemies from occupying an unreachable visible half-screen. Source sleeping animations use the encounter clock. Scripted cameras stay inside loaded artwork; Weapon Frog excludes its transparent left margin.

The boss shield slam now buries a grounded knight: alternate RIGHT, LEFT eight times after the entry animation to escape. Original buried artwork and prompts preserve the selected knight. Boss hits show feedback without interrupting his attacks, his punch/spikes can damage reinforcements, and eligible simultaneous humanoid attacks can trigger a button-mash weapon clash. Health bars use the encounter maximum consistently on host and guest.

The main-screen quick item selector and original overhead feedback from v0.38 remain. ROM: `castle-arena-v0.39.nds`; archive: `releases/castle-arena-v0.39.zip`. Real DS/DSi performance and physical two-console multiplayer remain unverified.

## v0.38 quick item selector

Campaign items now share one square directly on the main bottom screen. Tap the arrows to cycle through owned items and equip immediately; tap the square to use, or press Y. The first available item is equipped automatically. When an item runs out, the selector moves to the next available item. Guest tap-use waits for the host-confirmed selection. Stats remain a separate square.

Fresh characters have no shovel. The v0.37 preview showed a test inventory, not an early unlock; current campaign levels do not award a shovel. It still requires its later-game acquisition, and Mr. Buddy remains buried. The selector supports four item identities in its layout, but this release does not add campaign bow/sandwich acquisition or sandwich effects. Future items must be hooked into actual ownership/use before appearing. Arena bow behavior remains unchanged.

Original PLAYER305 item artwork and PLAYER316 selection pulses now show the potion or owned shovel above the character when equipped; the existing original bow popup remains. Updated preview uses ordinary early-level inventory. The UI retains source font, health/magic/XP, boss bar, map card and stats controls.

Download `castle-arena-v0.38.nds` / `releases/castle-arena-v0.38.zip`. Save and wire formats remain compatible. Use matching builds on both consoles. Physical wireless and real DS/DSi FPS remain unverified.

## v0.37 bottom-screen redesign

The bottom screen now uses a consistent parchment, ink and gold layout with the original font, knight portraits, health/magic/XP artwork, potion icon and four original attribute icons. The shared layout covers campaign, arenas, Blacksmith/Animal Ark/Weapon Frog and the world map. Health remains on the bottom screen. Square item buttons retain their original touch positions and only appear when available; the equipped-weapon menu remains absent. Shops keep their in-world purchase boards.

The world-map card names the destination and shows travel, locked-destination and guest-loading states. Hosts/solo players can tap the left/right sides to walk between destinations and the middle to enter, using the existing travel and entry rules; guests follow their host. Stats retain host-authoritative point spending and waiting feedback. Pause, controls, lobby and selection pages use matching colors. Boss health, pickups, revives, clashes, level/combo notices, save status and FPS information remain available.

Download `castle-arena-v0.37.nds` or `releases/castle-arena-v0.37.zip`. Copy the ROM to the usual SD/flashcart location; retain the existing save. Use the same release on both consoles. This UI-only update keeps campaign protocol36, arena protocol22 and the existing save format. Real DS/DSi performance and physical two-console play are not newly verified.

# Castle Crashers DS - v0.36 Barbarian Boss

This is a separate DS homebrew arena demake in `castle-arena/`. The existing FTL project is untouched. It uses assets extracted from the installed PC game, with a rebuilt C combat simulation and native BlocksDS DSWiFi local multiplayer. It is not the complete original game or an exact arena port yet.

## Release status

v0.36 adds Barbarian Boss after the bridge, with original terrain, boss animation, attack cues, carried princesses, chest and 29 treasure drops. Cut down the red princess after collecting treasure; two players must then duel, and only the winner kisses the princess. A simultaneous knockout restarts the duel. Solo play proceeds directly to the rescue ending. The chosen knight uses original kiss artwork. Boss completion is saved and the unlocked boss can be replayed from the world map.

The world map uses the original FLUTEY music and stronger source-material colors. The blacksmith now animates his hammer with its original sound. Boss introduction, combat, treasure, duel and victory have their source music cues. Connected gameplay continues through the castle, Barbarian War, bridge and boss without forced map returns. Both consoles must use v0.36 for local wireless campaign play.

Native QA covers chapter progression, source artwork and collision bounds, solo/co-op ending state, treasure synchronization, save migration, loading handshakes and rendering equivalence. Real DS/DSi frame rate and physical two-console sessions remain unverified. AI, two-reinforcement scheduling, some cinematic movement and the four-weapon/two-pet inventory are adaptations. The source boss unlocks later destinations and character ID5; those are not implemented here. This is not yet a 1:1 port. Mr. Buddy still requires the later shovel. See docs/BARBARIAN_BOSS_AUDIT.md and docs/PROJECT_CONTINUITY.md.

Arena now supports sprint+A roll and sprint+X dive attacks. Campaign unlocks them at level4; the second-jump spin remains level2. Practice difficulty is selected with Up/Down at knight selection. Original LEVEL/New Combo labels announce level gains and supported unlocks. Unspent skill points open the existing stats menu at world-map entry or boss completion, without interrupting fights.

Audio has been raised across all34 clips with offline compression, paired music output and overlap-aware gain. Modelled idle/normal/busy mixes are louder than the previous build; real-console listening remains unverified.

### Earlier v0.35 changes

v0.35 restores separate Home Castle and Castle Keep map stops. Home Castle starts in the barracks and continues through the throne/courtyard into Castle Keep; the Castle Keep stop enters its encounter/shop section directly. Normal gameplay continues from Home Castle to Castle Keep to Barbarian War and its bridge without a world-map interruption. Clearing Castle Keep also unlocks Barbarian War for map replay. Barbarian Boss remains a labeled, unavailable destination.

START opens Resume, World Map, Sound and Save and Quit. Solo games pause; a campaign host pauses both players while keeping wireless active. Guests can open a local menu and leave the room, but destination and shared pause remain host-controlled. Arena wireless continues playing while its local menu is open.

The original map layout now uses adapted terrain/building colors in 84 compressed tiles. One or two chosen knights animate and walk smoothly between stops with a following camera. Travel is host-authoritative; entry waits until arrival and guest loading is acknowledged. Both consoles must use v0.35.

### Earlier v0.34 changes

v0.34 adds Barbarian War and its bridge continuation, including the War Machine, source encounters, mounted sequence, sleeping enemies and interactive scenery. The separate giant Barbarian Boss stage is not included. Original terrain, actor frames and music are used with bounded chapter banks; the complete Keep stays resident during its rooms and waves.

The original western world map connects the Blacksmith, Castle Keep and Barbarian War. The Blacksmith's doors lead to the Animal Ark and weapon frog. Supported previously collected weapons appear on their stands; swapping leaves the old weapon on the ground. Mr. Buddy remains buried in the barracks and requires the later-game shovel, which this build does not grant early. Owned pets can follow, be exchanged as pickups, and be returned through the Ark's bin.

In local multiplayer the host chooses the map destination. Both screens receive the same selection and travel together; simulation waits for the guest's scene-loaded acknowledgement. Loading has an extended timeout, while obsolete/reordered acknowledgements cannot release the wrong destination. Both consoles must use v0.34. This uses native local wireless without internet.

Save files retain each knight's progress, collected pets, tools and cleared levels, migrating older saves. The original DSi-working boot payloads and both48KiB reserves are preserved. Native source, rendering, save and packet tests pass; sustained DS/DSi performance and physical two-console sessions still need hardware verification. Hub ambient NPCs use static source poses; combat/AI and some cinematic movement remain adaptations. See docs/PROJECT_CONTINUITY.md and docs/next-level-source-audit.md.

### Earlier v0.33 changes

v0.33 removes the invented shared upward rebound from aerial light hits. Contact pauses the attacker for seven ticks without changing its vertical momentum, while the target follows its own recoil. Grounded targets launch higher than airborne re-hits; re-hits retain horizontal separation. Aerial light connects only on its authored hit frame and uses the original follow-up markers. Air heavy has its separate source rebound; spin no longer receives the light-hit pause.

Level-one moves now include **A-A-A-A**, **X-X-X**, alternating airborne lights and the existing aerial heavy chain, close throws, and **X beside a fallen enemy to stomp**. The new third heavy is a downward ground slam. Stomp also hits other nearby opponents on its impact frame. Both use original PLAYER artwork with held keyframes and original move timing, including all four knights and supported held weapons. Existing knight graphics are compressed losslessly to help fund the additions; no new image buffer is allocated.

The source's unlock indices were verified from original bytecode: second-jump spin is level 2; ground/air A-X uppercut is level 8. The old aerial A-X spin shortcut is corrected to uppercut. Later level 4/16/32/50 moves remain outside this first-level build; the existing six-hit escape and other combat adaptations remain. See `docs/default-combat-audit.md` and `docs/juggle-source-audit.md` for exact evidence and limits.

Both consoles need v0.33. Campaign snapshots remain 276 bytes and input replies 36 bytes. The working ARM7 boot payloads and both 48 KiB reserves are preserved, with 292 bytes beyond those reserves. Native tests do not establish real DS frame rate or physical two-console reliability.

### Earlier v0.32 changes

v0.32 restores the original in-world shop boards and shop potion artwork, replacing the bottom-screen shop panel. Item prices, selection and purchase/failure states are attached to the boards; bottom-screen health and inventory remain available.

The princess movie now respects the source doorway's foreground depth. Conversion removes only actor pixels hidden by that original silhouette, with no added runtime overlay. Before the shop, foreground restoration stays within actor/effect damage instead of enlarging redraw areas across adjacent wall strips. A native 1,200-frame comparison reduced foreground decompressions from 7,569 to 1,374 and CPU time from 346 ms to 243 ms; these are desktop measurements, not DS FPS. Finding legal spawn positions also avoids repeated full-height searches.

Enemy loot is rolled when the corpse disappears. Timed items use the source bounce, become collectible on frame 20, blink late in their lifetime, and expire on frame 304. Each item has its own clock. Five source food variants retain the original healing amount. Weapons retain persistent ground swaps. The DS supports four visible drops and queues overflow until space is free, without erasing existing items or blocking enemy reuse; a queued item's lifetime starts when it appears.

Both consoles must use v0.32 because drop ages now travel in the compact campaign snapshot. Host packets remain 276 bytes and replies 36 bytes. The build retains both 48 KiB reserves plus 1,476 bytes of additional headroom, and the original working boot payloads. Real DS/DSi frame rate, jitter and physical two-console play remain unverified.

### Earlier v0.31 changes

v0.31 composes the first barracks intro into the existing hidden image page, then switches complete images at presentation time. Its vertical camera pan no longer rewrites visible background memory. The handoff restores terrain before showing live gameplay; no new image buffer is allocated.

The handrail uses the actual collision boundary for actor depth. The doorway includes its final wall row, and departing cutscene knights follow the original foreground layer regardless of their feet position. Princess cutscene actor decompressions fall from 116 to 8 per playback using a cropped background cache in the same workspace, with identical image checks. Native desktop rendering time fell about 64%; this is not a measured DS frame-rate improvement.

The post-princess enemy group enters before players reach its normal spawn anchors; placement checks both heroes and moves blocked entries to legal nearby space. The shop now displays item prices, selection and capacity/funds messages, purchase confirmation, and the original red potion bottle. Health and item controls stay on the bottom screen.

The final build keeps 3,204 bytes beyond the two 48 KiB reserves; shared item artwork removes a duplicate 4,459 bytes without extra decoding. Both consoles need v0.31. Original boot payloads, multiplayer packet sizes and both 48 KiB reserves are preserved. Real DS/DSi frame rate, flicker and physical two-console play still need verification.

### Earlier v0.30 changes

v0.30 schedules prepared images for a consistent two-refresh presentation deadline. Fast and medium-cost renders no longer alternate between one-refresh and two-refresh presentation latency. Long overruns still present late; this does not claim sustained hardware FPS.

Cinematic scaling and alpha/fade arithmetic no longer invoke per-pixel software division on the DS. The compiled pixel expander was checked, and all 298 reference images remain identical. Cutscene artwork is still sampled at 15 fps; the performance display now counts movie renders and shows that target.

Villagers cache their unchanged poses in an existing unused enemy buffer, falling back safely when the sixth enemy needs it. Tests measure 87 decodes over 120 frames instead of roughly 360, without adding an image buffer. Nearby dirty display rows are batched into fewer transfers; pixel-equivalence tests cover clearing old actors as well as new ones.

New enemy waves retain previous corpses until their independent blink timers expire. If all actor slots are occupied, pending spawns wait for a free slot instead of deleting a corpse. Holding magic no longer resets walking animation or stops running. Running dust is smaller and intermittent, without doubling the artwork's own drift.

Foreground artwork is precomposited into the terrain with the same indexed colors as the previous full redraw. Foreground layers are decoded only when a character/effect overlaps them. All 49 scenes match the previous renderer exactly; the durable comparison also checks culling against forced redraws.

v0.30 checks pass for presentation deadlines, transfer batching, cached villagers, foreground culling, corpse overlap between waves, magic movement, cinematic pixels, save/audio lifetimes and multiplayer packets. Both consoles need v0.30. The build retains the two 48 KiB reserves and 2,020 additional bytes. Real DS/DSi performance remains unverified.

### Earlier v0.29 changes

v0.29 replaces background repainting with per-actor foreground rejection at the barracks rail and doorway. This preserves other characters already in front and avoids scanning a large rectangle around each actor. The doorway uses the original complete near-wall assembly rather than the far jamb and an approximate wall mask.

Cutscenes cache their decoded data in the existing scenery workspace, keeping the first 49,152 bytes separate for audio. Throne decompression falls from 6,564 calls to 63, and princess decompression from 4,301 to 166; all 298 reference images remain identical. Native desktop benchmarks were approximately 5×/1.8× faster respectively, not measured DS FPS. Both multiplayer reserves remain unchanged, with 1,732 bytes beyond the combined 96 KiB reserves.

Running retains momentum through vertical/diagonal turns and uses a gentler 22/18 increase over the existing DS walking speed. This reproduces the original dash-to-true-speed ratio with adapted DS pacing, not exact original world distance. The original dust artwork now provides a running trail, including local co-op synchronization.

Enemy and allied NPC corpses blink using the original PLAYER394 timing and disappear at grounded-death tick 44. Human bodies remain revivable. Held magic sustains visible middle frames rather than repeatedly restarting the source effect's fade-out; this sustained loop is an adaptation because the original clip ends.

Validation covers 49 reference/overlay scenes, preserved foreground actors, source masks, corpse timing, run turns and packet state, dust bounds, sustained magic, full campaign traversal, audio lifetimes and cinematic seeks/resets. Physical DS/DSi FPS and two-console joining remain unverified; vertical terrain rebases can still require a full transfer. Both consoles need v0.29.

### Earlier v0.28 changes


v0.28 aligns background and character sampling during camera zooms, uses display-refresh-based pacing, and uploads only changed background columns during horizontal scrolling. The 180-frame scrolling test transfers 111,936 bytes instead of 1,790,976 bytes for those updates. The main sprite pixel loop runs from ARM9 instruction TCM. These are verified implementation changes, not a promise of sustained hardware FPS.

Hold a movement direction for 30 game ticks to run at the original true-speed multiplier (2.2×). Releasing, reversing, blocking, attacking, jumping or hitting a wall resets the buildup. This hold-to-run trigger is a DS adaptation; the original distinguishes walking, dashing and true speed.

The weapon equipment panel and bottom-screen weapon box are removed. Picking up a weapon equips it and leaves the previous weapon on the ground; shop purchases also leave the old weapon behind. Stats and potions remain available, and the current weapon still persists in the character save. Older save files remain readable.

Enemy archers now use original aiming/release artwork, and the seated barracks NPCs keep their heads throughout their drinking poses. Full player dance routines are still unfinished. XP remains awarded on damaging hits; the bottom screen now uses the original pink progress-meter colors and level-relative fill rather than cumulative XP text.

Validation: native rendering, full-route solo/co-op, running/reset behavior, weapon ground swaps, source NPC animation, XP bar, save recovery and packet-size checks pass. Both consoles need v0.28. Real DS/DSi FPS and physical joining remain unverified. The final memory budget retains two heroes and the unchanged 48 KiB wireless plus 48 KiB runtime reserves, with 2,884 bytes beyond those reserves.


v0.27 fixes the barracks scrolling bottleneck: vertical movement shifts the existing decoded background and loads only newly exposed rows. Independently compressed 64-row chunks replace repeated full 448-row strip inflation. Pixel/buffer checks cover 65,709 partial requests; a 486-tick authored barracks sequence now uses 36 partial updates and 18,584 output rows. This is a decoder-work reduction, not a measured DS frame-rate claim. The bottom screen now shows measured draws per second and peak work time so hardware results can be reported directly.

The barracks door was mistakenly baked closed in the background. It is now an independently depth-sorted original door: the wounded knight bursts through at source frame 161, with the full 32-tick opening sampled at 15 fps. Skipping the scene leaves it open. Existing input-driven solo/co-op staircase/exit tests remain included.

The original doorway frame and adjoining stonework now draw in front of characters passing behind them. Original cliff-edge collision segments prevent walking or jumping through the sides of the first stairs; the diagonal staircase remains traversable by both players.

The near stair handrail also redraws over characters behind it, using its original artwork and the sloping stair edge to decide depth. Characters standing on the floor in front remain visible over the rail; the far rail stays behind the stairs.

Characters now retain XP, level, gold, potions, owned/equipped weapons, allocated stats and completion per knight. Saves use two alternating CRC-checked files at the SD/flashcard root (`castle-crashers-ds.0.sav` and `.1.sav`), with readback verification and recovery from an interrupted newer write. Saving occurs at room/encounter checkpoints, completion, solo pause/inventory and exit. A save failure is displayed; it never claims success. Progress loads after selecting a campaign mode. Replaying begins at the barracks with the saved character, rather than resuming a mid-battle snapshot. Back up both save files together.

Tap STATS to allocate earned points. Weapon switching now happens through ground pickups. D-pad selects, A confirms, B closes; touch also works. The menu pauses solo, while local co-op remains live. Health stays on the lower screen. Four base attributes start at1, can reach25, and follow the source point budget: two per level through20, then one, capped96 allocated points. Weapon modifiers affect combat, health, mana and movement. Equipping defense or allocating points does not heal for free. Guest characters load from their own console; menu requests are host-authoritative, acknowledged, and safe to retransmit without spending twice. Campaign packets remain276 bytes and replies36 bytes. Both consoles require v0.27.

Barbarians now have source-confirmed frontal blocking, bow attacks, three melee choices, different approach distances, and an opposite-side flanking role. The shared two-projectile budget remains. Reaction clocks and some animation mappings are still DS adaptations; this is not execution of the original full ActionScript engine.

Original cinematic and barracks sound cues are embedded in the ROM's NitroFS data, loaded into renderer storage that is inactive during their scenes. No additional persistent audio bank is allocated. This requires the loader to provide access to the ROM file or cartridge. Storage/audio availability and physical wireless timing still require DS/DSi validation.

The full first-level route is present, but this remains a hardware-validation candidate: real sustained FPS, DSi storage, and two-console joining have not been verified on this release. A native 3DS port would offer a better target for closer fidelity; this remains the DS/DSi build and does not use native 3DS hardware.

## Prior releases

v0.27 validation: the native suite passes, including 48 reference/overlay scenes, rail-mask and cliff-edge checks, audio lifetimes, save recovery and two-player packet tests. The DS build leaves 101,060 bytes before runtime allocations: the unchanged 48 KiB wireless and 48 KiB additional runtime reserves, plus 2,756 bytes. This static budget is not a measurement of worst-case runtime heap usage. Both consoles must use v0.28 for multiplayer.

v0.26 translates the original source timelines and weapon selectors into the DS renderer. King/crystal and princess scenes now use 298 frames at 15 fps, original actor placement, corrected gray/barbarian costume selectors, genuine spell transparency, the original camera paths and the source opening fade-in. Player release occurs at the original source frames366/236; post-release holds are not inserted into the movie. The invented King close-up has been removed. Barracks motion now includes115 wounded-knight samples with exact positions and authored blinking, the camera guide, and eight gray knights following their departure paths.

All four knights use source-derived equipped artwork throughout their27 animation states. Sparse weapon changes and lossless cross-color sprite compression keep the extra art within the same memory allocation. Weapon9 is correctly labeled Carrot (the prior Pitchfork label was wrong). Both heroes retain independent equipment and cache invalidation when swapping on the same animation frame.

Validation: all4,864 held-weapon frame combinations match source-derived hardware colors; all298 cinematic images match reference hashes with buffer guards and random seeking; all145 barracks actor frames match the source conversion. The full native check suite and DS build pass, including45 reference/overlay scenes, camera handoff, terrain presentation and packet sizes. This does not establish real DS/DSi frame rate or physical wireless joining. The original game's full script engine and complete cinematic audio cue sheet are not executed by this renderer.

v0.25 adds the original LEVEL2 opening barracks before the King/crystal scene: the 1280Ã—448 room, sampled musicians and wounded-knight entrance, the interrupted party music, free first potion at the trainer, five breakable barrels, and the upstairs exit. The source timeline runs 486 game ticks and can be skipped with A. The room then remains playable for exploration before entering LEVEL13. This is a reconstructed scene using original assets, not execution of the original Flash timeline.

The shop now includes the original Skinny Sword (weapon2, 32 gold), alongside the 12-gold potion. The prices are verified from the original LEVEL19 bytecode: its custom A0/A2/A3 instructions contain Push-format fields that the ordinary decompiler did not recover. The original Carrot pickup is weapon9 and requires level5; the rare barbarian weapon is19. Equipment uses the verified source stat modifiers. The bottom screen shows its original icon, and all knight poses now use the extracted held-weapon selector for the equipped item. The original Skinny Sword selector shares the starter sword artwork; Carrot and Barbarian weapon selectors visibly differ. v0.27 adds persistent character saves and first-level equipment/stat menus; the entire original combat engine is still not ported.

Lossless DEFLATE compression replaces the older scenery codec, recovering 271,061 bytes from existing assets before adding the barracks. Decoded colors and pixels match the previous assets exactly. The bounded decoder allocates no heap or additional image surface. The same two-player packet and character-buffer limits remain in force.

Naming/end-point clarification: Home Castle leads into the one-room Castle Keep shop, then Barbarian War. This build covers the throne/courtyard/rampart route and adjoining shop, stopping before Barbarian War. The opening LEVEL2 barracks now leads into that route. Original LEVEL13 exit code completes13 and unlocks19; LEVEL19 completes19 and unlocks11. Exact source paths are recorded in `generated/campaign-source-endpoint-audit.json`. Earlier notes loosely called the whole opening route Castle Keep.

v0.24 prepares campaign terrain in RAM and copies it at vertical blank before switching to the matching camera and actor page. Rendering no longer overwrites visible campaign terrain or its palette. The lower-screen transfer waits for the next refresh when a terrain copy occurs. Native checks explicitly verify that live terrain/palette remain unchanged until presentation; this addresses the in-place-update cause of flicker, but actual DS/DSi flicker and transfer timing still require hardware testing.

First-staircase movement now follows the original BSP diagonal stair surfaces instead of using the height of the back wall. Tests cover four lanes down/up, vertical approaches, the adjacent flat lane, both heroes and the whole Keep route. Enemies now retreat and reposition up/down while waiting or recovering, then close in for their attack turn. They retain readable windup and one-attacker-per-hero limits; the timing is a DS adaptation rather than the original complete AI scheduler.

v0.23 gives campaign an empty starting inventory. No bow slot appears in campaign. Obtaining potions reveals a 52Ã—52 square item slot with the original potion icon and a count. Tap to equip (gold border), tap again to unequip, and press Y to drink only while selected. The final potion removes the slot and clears selection; full health does not consume a potion. The selected potion briefly appears above the knight. Both players have independent inventories and selection, synchronized in the unchanged 276-byte snapshot. Arena/practice retain the bow in the same square layout. Health stays on the bottom screen.

v0.22 adds the missing LEVEL19 Castle Keep shop encounter after the continuous LEVEL13 route. The original 2448Ã—448 route retains its four 3/3/3/4-enemy groups; the adjoining 1024Ã—448 shop scene adds six barbarians together and a fighting gray knight at the extracted source positions. There are now 19 enemies. The shop opens after its fight, and leaving its right exit completes this prototype. v0.25 adds the preceding barracks; Barbarian War is not included.

The princess sequence uses an 18-tick approach/return pan matched to the extracted movie camera endpoints. Player positions remain fixed during the pan and movie. The gate into the separate shop room remains an intentional room transition, preserving health, XP and gold. Original actor/camera timelines now replace the old 64-frame cinematic approximation.

Foreground redraws now include the central arch, entry and exit towers, and front parapet. Large assemblies are sliced into visible pieces; 47 depth layers replace the previous 14. The original hay cart at world(917,333) is removed from the static background and breaks after four distinct melee or projectile hits, with its original hit/destruction artwork and hay fragments. Other stools/chairs in LEVEL13 and barrels/walls in LEVEL19 remain scenery: the audited scripts do not register them as breakables. The new barracks adds breakable barrels; full furniture destruction is still incomplete.

The potion stand uses the original potion sprite. After the six-enemy fight, stand near it and press **A** to buy a potion for **12 gold**, up to five. Tap the potion slot to equip it; **Y** then drinks one when injured and restores health. The 12-gold potion and 32-gold Skinny Sword prices are now verified directly from the custom source bytecode. Press A near a weapon to swap; the previous weapon is left on the ground. Potions, cart destruction, the gray ally and all six enemies synchronize in local co-op.

Loot now uses the extracted normal-enemy roll order: a 1/120 barbarian-weapon roll, then a 1/6 random item roll, otherwise a 9/10 coin roll. Random items choose uniformly between five small foods, chicken, bag and the gem/coin selector. Small food heals 10%, chicken 50%, coins give 1 gold, bags 5, and gem values span 6â€“16. The rare weapon roll now produces the original barbarian weapon; food variants share the existing food sprite and gems share one genuine source gem sprite. There are no pet modifiers or complete inventory menus, so this is not the complete original loot system. Pickup flashes, sounds and HUD feedback remain included.

Barbarians are moderately more assertive than v0.21: 10-tick windup, 30-tick recovery/attack spacing, and 24 ticks of mercy after a hit, with one attacker allowed per hero. Chase movement runs on three of four ticks. These remain deliberate DS balance adaptations. The gray ally can fight, take damage and die; ally hits award no player XP. Campaign projectiles are limited to two active at once to retain the existing wireless packet size. Arena/practice combat is unchanged.

Campaign starts at level 1 with 100 HP and 40 mana on the starting sword. The Skinny Sword raises maximum health to128 at level1; equipping it does not heal damage. Campaign melee damage, magic damage, movement and defense now use the verified weapon attributes with original base-stat minimums. Barbarian HP is 50 solo / 60 co-op. XP is earned per damaging hit; original cumulative thresholds put level 2 at 191 XP and level 3 at 401 XP. Air spin unlocks at level 2 and uppercut at level 8. Level-ups refill health/mana, and health grows by 3 per level. v0.27 persists gold/XP and equipment per knight and adds stat allocation; full later-game skill progression is outside this first-level build. Hold R beside a fallen partner for two seconds to revive at 25% health.

Choose **Single Player > Solo / Host Campaign / Join Campaign**. Both consoles need v0.25. Campaign sends nine compact actors, two projectiles, progression, pickups and shop state in the same 276-byte command packet. This is direct DS local wireless, not Nintendo Internet WFC. Physical two-device joining and hardware frame rate still require testing.

Terrain stays at full world-pixel resolution with 128-color palettes. Lossless compression of the three arena backgrounds frees room for the added shop/foreground assets while reusing existing decode memory. Arena masks share storage with the campaign staging surface. Terrain strips, foreground pieces and NPC decoding reuse one scratch buffer. Identical hardware palette colors in the shop were merged losslessly, saving 57,758 bytes to help fund staged terrain updates. The final link budget includes both heroes, six enemies, the ally, sprite caches and packet buffers; joining allocates no additional character framebuffer. A build gate reserves 48 KiB for the installed radio's 41,552-byte allocation and another 48 KiB for runtime overhead. This is a static budget, not a measured hardware heap peak. Hardware background wrapping remains enabled. Original menu, arena and campaign music remain included at their existing compressed rates. The user-confirmed v0.7 ARM7/ARM7i DSi boot sections are preserved byte-for-byte.

Validation: native DS build; normal-input solo/co-op barracks traversal, trainer gifts, barrel destruction, equipment purchases/swaps and level gates; solo/co-op route and completion; 30,000 compact campaign snapshots; seeded loot distributions; shop purchases/potions and cart hits; nine-actor combat; all 64 movie hashes; 40 software/reference-overlay scenarios; map/palette transitions, protected VRAM rows and camera pans including a guest missing pan-in packets. This revision has not been playtested on hardware or in melonDS. Terrain rebases are now staged before presentation; hardware timing still needs checking. Package: `releases/castle-arena-v0.25.zip`.

## Play

Build with `./build.ps1`, then load `castle-arena.nds` using your DS homebrew loader or flashcart. Both consoles need the same ROM; Download Play is not implemented.

The main menu has **Single Player**, **Practice**, **Multiplayer**, and **Controls**, using the installed game's menu illustration and font. Use Up/Down and A. Controls shows combat on the bottom screen and pause/start/rematch/sound help on the top; A or B returns. START toggles sound on the main menu.

For a duel, select **Multiplayer â†’ Host Room** on one console and **Multiplayer â†’ Join Room** on the other. Select the advertised room and press A. The host presses START after both knights are selected and input exchange is ready. Each player selects a knight. The host sees no second fighter or second health bar until an associated guest confirms a selection. A guest leaving before the round clears that slot. SELECT leaves a room. Host START begins another round after the result; score carries forward.

This uses DS-mode direct local wireless, with no router, Internet connection, Nintendo WFC service, IP stack, or cloud server. DSWiFi room discovery, host beacons, association, CMD state packets and REPLY input packets are implemented. Physical DS-to-DS reliability is **not yet verified**; successful compilation or emulator testing does not prove real radio compatibility.

| DS control | Action |
| --- | --- |
| D-pad | Move on the arena floor |
| A | Light attack |
| Y | Use the selected potion in campaign; equipped bow in arena; otherwise No item |
| Tap bottom-screen item square | Toggle potion selection in campaign, or bow in arena |
| X | Heavy attack; close grounded opponent: throw |
| B | Jump; release and press again airborne for spin |
| R + D-pad | Slow movement with directional shield |
| Hold L | Magic-ready glow until released |
| X X X | Three-hit heavy chain ending in a ground slam |
| X beside a fallen enemy | Contextual stomp |
| L + A | Magic projectile; airborne: diagonal downward shot |
| L + X | Magic splash |
| A A A A | Four-stage light chain |
| A then X | Uppercut / launcher (campaign level 8) |
| Jump + A | Alternating aerial slashes |
| Jump + X | Aerial heavy slam; X follow-up while still airborne |
| Airborne A then X | Uppercut (campaign level 8) |
| Downed B + Left/Right | Recovery roll; holding R also starts recovery |
| Weapon clash: tap A / X / B | Mash to break the lock; holding does not count |
| START | Pause practice / host starts round / rematch |
| SELECT | Return to menu |

## What is implemented

- Two-player duels and a CPU practice opponent, round countdown, last-knight-standing result, rematches and win totals.
- Light/heavy attacks, four sword poses, launcher, airborne attacks, directional blocking, hitstun, knockback, mana regeneration, blue ice and orange fire magic, freeze and burning.
- Original knight artwork and pose timelines, arena background from `level49.pak`, original menu illustration, original font, original health/magic meter and portrait assets.
- Full installed-game music and real effects use mono DS IMA ADPCM. Four Brave Champions (`NG90433.xma`) is the menu track at 8 kHz; arena music (`Waterflame_TheShow`) is 6 kHz, campaign MUDHOLES is 5,512 Hz, and effects remain 16 kHz. Loops use short boundary fades, not extracted original loop metadata.
- A host-authoritative 30 Hz simulation. Clients send controls and cumulative action counters; host snapshots synchronize both screens. Stale packets are rejected, lost input expires, and disconnects return an error screen. The client currently displays received snapshots without prediction/interpolation.

During fights, the touch screen shows the original portraits, health/mana meters, win totals, round status and a tappable bow item. Match control hints have been removed to make room; the main-menu Controls screen remains available. The top screen keeps the arena, countdown and winner announcement unobstructed by meters.

## Additional arenas and magic readiness

Practice and Host Room open an arena preview selector before starting. Castle, King's, and Volcano variants use original level49/46/47 PvP artwork with matching transformed props. Their extracted BSP collision files are byte-identical. The host's map ID is synchronized in snapshots and survives rematches. Additional animated ambience and hazards are not implemented; these are static arena presentations. Provenance is in `generated/arena-variants-audit.json`.

Holding L loops a smaller, translucent original character-colored magic-ready particle behind the knight; releasing L removes it. One particle replaces the original repeated particle emitter to bound DS rendering cost. The synchronized held modifier drives the effect, not an attack or mana-spend event. Double-heavy uses the original second-heavy animation and its two hit events. Repeated button presses no longer overwrite a pending buffered action. The practice CPU now sends deliberate attack presses too.

## Knight selection

Practice chooses a knight before starting and picks a different CPU knight. The host selects before opening a room. A guest remains connected while choosing, but its fighter, shadow and HUD stay hidden from the host until confirmation. Both players must confirm before the host can start. Choices survive rematches; equipment resets each round. Duplicate knight choices are allowed.

Blue uses freezing magic and Orange burning magic regardless of host/guest slot. Red and Green are playable with their original artwork and casting sounds, but their magic uses simplified direct-damage attacks. Continuous lightning and poison damage-over-time behavior are not yet recreated. This is an appearance/selection expansion, not a claim of complete character parity.

## Second-jump spin

Press B to jump, release, then press B again. A rising early tap waits until z=38, matching the readable original topspin height threshold. The original 13-frame spin has two strike events. It gives no second upward impulse. One spin is allowed per airtime (shared with the existing A-X route), resetting on landing. Damage or another committed attack cancels a pending early tap. Descending spin gravity approximates the original 0.3 factor by applying one gravity step every three animation ticks. Buffering and the one-spin cap are bounded prototype adaptations.

## Airborne downward magic

Jump, hold L, and press A to fire diagonally downward in the direction faced. Grounded L+A remains horizontal. The original downward-cast animation is used; projectile motion follows the readable source's half-scale 6.5 horizontal / 4.5 downward units per tick without gravity. Mapping this to the light-magic input for all four knights, including Blue, is an intentional user-requested adaptation. Blue originally used a ground ice effect in this branch. Red/Green retain their simplified direct-damage behavior.

## Bow and directional defence

Y fires the equipped bow; A is light attack. Tapping the bottom-screen bow toggles equipment without interrupting an active attack; committed arrows still release. Nothing is equipped at the start of each round. The original 20-frame bow animation releases the arrow and original sound on frame 11. Arrows cost no mana, arc with source-scaled upward velocity 2.5 and gravity 0.5, and use the fixed agility-10 preset: damage 12, horizontal speed rounded from 16.25 to 16 world units per tick. Airborne bow use briefly floats the shooter for 11 ticks as in the readable original script. Firing cadence and quantized motion are prototype adaptations. The CPU can use the bow at range.

Shield defence is directional: melee uses the attacker's position and projectiles use incoming travel direction. Rear hits bypass the shield; throws and splash remain unblockable. Projectile collision now checks the swept path against the original-scaled standing body height and width, so shots at torso height connect and fast arrows cannot skip past a fighter between updates. Rocks still obstruct low shots. Arrows do not apply either knight's magic status effect.

## v0.12 fidelity changes

Airborne A alternates the original punch11_1 and punch11_2 animations. Airborne X starts the original slamdown animation, then can follow with the existing second-heavy pose while still airborne. A then X retains the prototype's high-air spin route. New poses use their original frame-5 hit events and attack rectangles. Follow-ups unlock at the second aerial light's frame-7 marker and the slam's frame-9 marker; neither bypasses startup. A valid late attack input survives the six-tick landing recovery; knockdowns still discard it. These transitions preserve the bounded six-hit escape rather than claiming the full original juggle system.

Neutral-resistance burn now lasts 59 ticks and checks damage at the source's 30-tick cadence before decrementing. Fixed-timer freezing remains an adaptation; original human freeze used directional inputs to break free.

The existing elemental hit-burst artwork is rendered at about 55% of its previous width/height and plays once over 14 ticks. A small final ice shard marks the rest of freezing. These are readability adaptations: the selected original elemental melee FX are reused for magic hits, not an exact reconstruction of the original projectile impact/status particle system. Arrow angles use eight pre-rendered steps approximating the original vertical-velocity-driven rotation, without per-frame rotation work.

All 582 knight frames remain available. Lossless sharing of identical cropped pixel blocks saves 264,104 bytes versus storing the new atlas without sharing. Every expanded frame was hash-checked against its unshared pixels.

## Combat update

The shield now holds its raised pose instead of restarting its three-frame animation. Original roll (8 frames), aerial topspin (13), and sword-lock (5 looping) artwork is imported for both knights. Source references are recorded in `generated/asset-audit.json`.

Recovery roll moves horizontally with rock/boundary collision and temporary protection. A sixth hit in a short chain forces a protected knockdown; prone recovery is protected so opponents cannot keep resetting it. Airborne light then heavy can trigger topspin high enough above the floor, with two hit windows matching the original animation events. These recovery and anti-stunlock rules are prototype adaptations.

Eligible simultaneous grounded weapon hits involving a heavy attack or finisher occasionally start a sword lock. Tap A, X, or B: a five-tap lead or ten total taps resolves the lock; equal scores resolve without favoring either player, and inactivity has a timeout. The original script supplies the scoring thresholds and artwork; the deterministic every-third-eligible-contact trigger and timeout are adaptations because decompiled trigger conditions are incomplete. Button edges are preserved between simulation ticks and over wireless input packets. Holding a button is not a mash.

## Gameplay fidelity

Readable portions of the installed `MAIN.swf` were used for a fixed, balanced level-20/stat-10 arena preset: 409 health, 85 mana, .55 mana regeneration per game tick, and physical attack values 15/18/24/26. Movement is scaled for half-size character art. Original `PLAYER.swf` timelines supply the animation poses and strike-event frames: light attacks 3/5/4, finisher 10, heavy 7, uppercut 3, aerial attack 3, projectile cast 6, splash 7 (one-based).

Arena art and knight art share half-resolution world units: an 848x636 artwork viewport becomes a 424x318 world. The actual PvP frame (frame 3) is now exported; earlier builds mistakenly used the campaign frame with cropped cat objects at the sides. Eleven original props are sorted with players by foot position, and the original foreground parapets draw last. Foreground parapets are 20% shorter, anchored at their original base. When their opaque pixels overlap either knight's padded animation bounds, the foreground fades to about 31% opacity, then gradually returns to full opacity when clear. Jumping and defeat poses are included. This is a visual change; collision is unchanged.

Five rock footprints are recovered from the installed original `bsp49b.pak`, rounded to world coordinates. Foot movement sweeps in unit steps to prevent crossing rocks; body separation also respects rocks. Low projectiles and melee paths are obstructed by rocks. The five-unit foot radius, 16-unit body separation and 56-unit projectile clearance are adaptations. Melee now uses original pose-specific attack rectangles against the source-scaled 22.5-unit body half-width and 60-unit body height, with 15-unit lane tolerance. Unknown source lane offsets are treated as zero; front-center gating is retained except for aerial spin. Throw and splash volumes remain adapted. Bodies can pass when separated by floor depth or jump height. Spawns sit in the clear side corridors.

Playable foot positions are x=33..391 and y=175..307. The original BSP extends horizontally beyond this viewport, so this is still a bounded adaptation of the full original arena. A shared camera follows both knights, includes jump height, zooms out to keep them visible and eases back in when they approach. Its view ranges from 288x216 to 424x318. Camera movement does not change combat distances or movement speed. HUD remains on the bottom screen.

The animation atlas has 1,216 frames across four knights, including original landing, airborne hurt, grounded knockdown and defeat poses. Jump and hurt sequences hold their final frames instead of looping. The original standard launch speed (-20) and gravity (2) at 30 Hz become +10 and 1 at half scale, giving a 55-world-unit apex and landing on tick 21. Landing lasts six frames; grounded knockdown holds its six visual frames through a 21-tick recovery. Air attacks only grant lift on accepted hits, not misses. Original motion and timeline references are recorded in `generated/asset-audit.json`.

This build advertises v0.16 rooms (packet protocol 14); both consoles must use the updated ROM. Older room IDs are excluded. Hosts only accept fully associated clients and input from the selected guest association ID. Payload sizes are checked against the installed library: 276/276 bytes for host snapshots and 36/227 bytes for guest input. Native local radio operation is implemented; physical room joining still needs verification.

The entire original ActionScript engine has **not** been ported. Some decompiled scripts contain invalid expressions. Collision dimensions, recoil, magic/status tuning, input buffering, and simplified aerial behavior still need comparison against the PC game. Holding an attack does not repeat it; new presses buffer only during the final seven recovery ticks. The original full juggle system, complete weapon/stat selection, additional characters, animal orbs, full inventory/items, all combos, arena hazards, other stages/modes and four-player play are not complete. UI layout is adapted to 256Ã—192, not pixel-identical to the PC menus. This build should be treated as a functional starting point, not a claim of full gameplay parity.

Close-range grounded X now starts the original 17-frame throw animation when facing an unobstructed opponent within 22 world units and 10 depth units. Release occurs on animation frame seven, with 18 damage and an unblockable launch. Three-tick melee contact windows and an eight-tick input buffer make timing less brittle. Simultaneous valid hits and throws resolve symmetrically. These are prototype combat adaptations, not a complete reconstruction of the original engine.

Successful aerial hits briefly float both fighters for six ticks and reduce horizontal push, allowing launcher follow-ups. Whiffs do not sustain flight. The six-hit protected escape bounds long combos. Rectangle provenance is recorded in `generated/original-hitboxes-audit.json`; float duration, push and escape threshold are adaptations.

## Performance and startup checks

v0.10: foreground overlap scanning is restricted to the intersection with the character bounds. A five-run desktop CPU rendering benchmark over 6,000 synthetic frames (both knights holding L, six projectiles, three maps) measured medians of 243 ms before, 240 ms with that scan optimization alone, and 256 ms with the final soft glow. The final effect costs about 5% more than the old opaque effect in this stress scene. This is not DS or melonDS FPS; hardware speed remains unverified. Raw runs are packaged in `performance-v0.10.json`. Reference/overlay comparisons pass across all 17 scenarios.

The deferred full-radio candidate also white-screens on the user's DSi. The next separate experiment, `./build.ps1 -LocalRadioOnly`, produces `castle-arena-local-radio.nds`. Link-time wrappers route six TWL radio entry points to their NTR equivalents, preventing the unused ath6k/WPA infrastructure driver from being pulled into the ARM7 image. This requires the existing ARM9 `INIT_ONLY | WIFI_LOCAL_ONLY` contract and is not a general-purpose internet-capable core. The local DS driver, wireless FIFO, and host/client transport remain present. Its ARM7i section is 8,360 bytes versus 25,668 in the failed deferred build (working FTL: 7,824). Hybrid header and section bounds pass checks; NTR driver presence and TWL Init/Update absence were verified from linked symbols. Boot and wireless operation remain unverified; this tests driver inclusion/binary layout, not a proven root cause.

User follow-up: the FTL-core comparison fully launches and practice is playable on the DSi. This implicates the changed ARM7 core, without identifying an exact failing instruction. `./build.ps1 -DeferredRadio` builds the next candidate, `castle-arena-dsi-wireless.nds`. Its custom ARM7 follows the standard Maxmod startup order and installs/services DSWifi only after an explicit ARM9 request from Host/Join. An acknowledgment has a 180-VBlank timeout before calling the normal Wi-Fi initialization. Native DS local CMD/REPLY and `WIFI_LOCAL_ONLY` remain enabled; no internet mode is added. This candidate boots directly without diagnostic A prompts. DSi boot and room joining still require user verification. The confirmed FTL-core comparison is preserved separately.

The user reports the first boot diagnostic also stays white with no messages. `./build.ps1 -FtlCoreCheck` produces `castle-arena-ftl-core-check.nds`, keeping the full game assets and startup checkpoints but using the standard `arm7_maxmod.elf` core. Its ARM7 and ARM7i ROM sections were verified byte-for-byte identical to the local working `ftl-ds.nds`. Multiplayer is explicitly blocked in this diagnostic; practice remains available after continuing through the checkpoints. This isolates the secondary-processor core as a candidate, without claiming the white screen is fixed. The normal release retains its wireless core.

For the reported DSi white screen when launched through TWiLight Menu++ from SD, `./build.ps1 -BootDiagnostic` creates `castle-arena-boot-check.nds` in a separate build directory. It retains the same game assets and DSWiFi ARM7 core. It pauses at game entry, after the first VBlank, before audio initialization, before music, and before the normal loop. Press and release A at each checkpoint. The first two checkpoints use a simple text console before the game graphics are initialized; A is polled directly without the ARM7 input service. Report the last visible message (or whether no message appears). This is diagnostic instrumentation, not a verified boot fix. ROM header section bounds are recorded in `build/boot-check-manifest.json`.

Version 0.6 adds two top-screen pages, switching the prepared character layer and camera together during vertical blank. Dirty row spans reduce overlay clearing and uploads; unchanged HUD data skips redraws. The 30 Hz scheduler uses elapsed time instead of alternate main-loop iterations, avoiding an additional idle iteration after an overrun. It caps accumulated delay rather than running an unbounded catch-up loop. All animation frames are retained. Actual FPS and flicker improvement remain unverified in melonDS/on hardware.

The renderer uploads the composed arena to a 512x512 indexed DS background once, then uses the 2D graphics engine for camera scrolling and zoom. A separate transparent screen-sized bitmap contains animated sprites/effects and the fading foreground. Original prop depth is preserved using a precomputed occlusion mask. A 269,664-byte world framebuffer and its full-scene scaling pass have been removed. Static menus redraw only when changed; the bottom HUD updates at 10 Hz. Polygon bounds skip irrelevant collision calculations.

During a match, the bottom screen shows `Speed N/30` (rendered frames per emulated/hardware second) and `Work Nms` (peak main-loop processing time over the last second). This helps distinguish game load from emulator-host slowdown; also check melonDS's own emulation speed. The target is 30 game frames per second, not a claim that it has been reached on every device. Brightness is explicitly reset and startup stage messages identify hangs before the main loop. A crash handler is enabled.

The user reports earlier builds booted in an emulator and on 3DS, with poor performance, and white-screened on DSi. Version 0.6 has not yet been verified on those devices. The attempted melonDS test exposed windows after resetting only the isolated test configuration, but screenshot/target recovery failed (`FrameArrived timed out`, then `foreground window did not report a process id`); no automated runtime/FPS/audio result was obtained. DSi emulation also requires a user-provided valid BIOS/NAND setup; the isolated test config has none.

Audio output attenuation was reduced: master 127/127, menu 127, arena 112, effects 127. Measured predicted gains are about +5.9 dB for menu music, +5.0 dB for arena music and +2.3 to +5.2 dB for effects. Conversion retains at least 1 dB decoded ADPCM peak headroom with no full-scale clipped samples. Audible loudness remains to be checked on hardware.

## Rebuild local assets

The existing portable BlocksDS toolchain is reused from `../.tools`. The conversion pipeline uses Python/Pillow, Java/JPEXS FFDec and FFmpeg. `tools/vendor/decrypt_asset.py` is Ethan Roseman's MIT-licensed extractor; its license is retained beside it. The Blowfish dependency is vendored locally.

```powershell
./import-assets.ps1 -GameDirectory 'G:\SteamLibrary\steamapps\common\CastleCrashers'
./import-menu.ps1
./import-hud.ps1
./import-audio.ps1
./import-effects.ps1
./check.ps1
./build.ps1
```

Pass `-GameDirectory` to each importer for another installation; Flash importers accept `-FfdecJar`. Audio accepts `-Ffmpeg`. Importers read the game files without changing the installation. Generated artwork, audio, extracted files and ROMs stay local and are excluded from version control. Asset manifests record conversion parameters and source hashes.

## Verification and hardware checklist

Current build: see `build/release-manifest.json` for ROM size and linked ARM9 memory size (before runtime heap/stack allocations). Automated checks passed, including nine static scenes and a 120-frame software-reference versus hardware-overlay motion comparison. The melonDS window/control failure described above prevented runtime verification of this version. See `build/release-manifest.json` for the ROM hash and recorded status.

`check.ps1` builds and runs combat and protocol tests with strict warnings, then renders the actual arena renderer for inspection. Tests also cover camera framing across 60,000 position/jump pairs, world edges, scrolling and zoom. Tests additionally cover source-scaled jump height/airtime, landing and defeat, point-blank magic in both directions, shield recovery, symmetric aerial trades, body separation, original rock collision and 60,000 legal-position ticks. Tests cover countdowns, movement, hit timing, blocking, combos, launchers, magic, status effects, simultaneous-hit draws, rematches, sequence wraparound, packet rejection, input counter recovery and 60,000 simulation ticks. `python tests/audio_test.py` checks the ADPCM encoder/decoder contract and quality.

Before calling physical wireless complete, test with two DS consoles: discover/join/start, both players' movement and combos, short taps, magic with music playing, repeat rounds, guest leaving before start and rejoining, guest reboot, host leaving, radio interruption and several minutes of sustained play. Every client must run the same build. No real-console result is implied by the automated tests.

DSWifi reference: <https://blocksds.skylyrac.net/dswifi/md_documentation_2local__multiplayer__mode.html>

## v0.25 asset additions

`tools/prepare_campaign_barracks.py` and `tools/convert_campaign_barracks.py` export LEVEL2 scene378 at half source scale. `tools/prepare_barracks_actors.py` and `tools/convert_barracks_actors.py` preserve original character and furniture artwork. `tools/convert_campaign_weapons.py` imports PLAYER weapon frames1/2/9/19. `tools/convert_barracks_audio.py` imports only the opening5.37 seconds of barrackssong, stopping at the source frame161 cue. The regular main-menu and campaign tracks remain unchanged.

The Carrot can be discovered below level5 but cannot be equipped until level5. Ground swaps require a fresh A press and leave the previous weapon recoverable. At most four drops are active; a full set of four ground weapons blocks further shop purchases rather than charging and losing the previous weapon. v0.27 persists owned equipment per knight and adds touch-based first-level weapon selection.

## v0.22 asset additions

`tools/pack_arena_terrain.py` losslessly packs the three original arena backgrounds. `tools/prepare_campaign_shop.py` and `tools/convert_campaign_shop.py` export LEVEL19 and its BSP geometry; provenance is recorded in `generated/campaign-shop-audit.json`. The continuous world exporter also generates `campaign_breakables.h`; loot imports include the original FX834 gem and potion images. No AI-generated replacement artwork is used.

Historical v0.24 link budget: 99,260 bytes before runtime allocations, after accounting for both heroes, six enemies, gray ally, actor caches, terrain staging and packet buffers. The gate reserves 48 KiB for wireless and 48 KiB for additional runtime use, leaving 956 bytes beyond those conservative reserves. The installed radio allocation is 41,552 bytes. Actual heap high-water and physical radio reliability remain unmeasured. HUD and renderer now share one copy of item artwork.


Final v0.25 link budget: 157,204 bytes before runtime allocations; 58,900 bytes remain beyond the conservative 48 KiB radio plus 48 KiB runtime reserves. This includes both heroes, six enemies, the ally, new NPC artwork, item artwork, camera/terrain staging and packet buffers. ARM7/ARM7i are checked against the user-confirmed v0.7 when packaging. Hardware heap high-water, sustained FPS and physical two-console joining remain unmeasured.

The final dense barracks scene with both heroes, NPCs and all furniture took 593 ms for 2,400 optimized desktop render calls; the Keep scene took 544 ms. These are CPU benchmark totals, not DS or melonDS frame rates. Fast RLE is retained for repeatedly drawn NPC/furniture artwork, while bulk terrain uses DEFLATE. The earlier six-layer scene improved from 899 ms with DEFLATE actors to529 ms with RLE and offscreen culling, with identical pixels. Final artwork fixes may slightly change those timings.

## v0.26 memory and compatibility

Final link leaves110,580 bytes before runtime allocations: the48KiB radio and48KiB additional runtime reserves remain intact, with12,276 bytes beyond them. Both hero caches, six enemies and the ally are included. Campaign version26 requires the same release on both consoles; packets remain276 bytes host/campaign and36 bytes guest. The working v0.7 ARM7/ARM7i payloads are preserved.
