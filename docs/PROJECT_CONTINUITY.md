# v0.40 boss movement (2026-10-05)

Supersedes v0.39 game release; trailer remains v0.39 footage. Fixed both bosses selecting walk while stationary. Barbarian Boss uses idle at attack distance and after intro,36-tick source walk while pursuing, continuous pose across its half-speed legal depth cadence. Ram uses idle after movement/clamp produces zero displacement. Native boss/War suites include new locomotion and guest snapshot regressions. Protocol/save/art/render unchanged; memory gate passes. No claim of new physical DS or wireless testing.

# v0.39 encounter and boss repair candidate (2026-10-05)

This section supersedes historical release notes below once the v0.39 manifest is packaged. Keep the v0.7 ARM7/ARM7i boot payloads, unitcode2 and banner checks. Campaign protocol37, arena22, save4; radio snapshot276B and input36B unchanged. World levels unlock sequentially, with Home Castle milestone8 and legacy later clears implying earlier access. Continuous level transitions remain.

War repair: ram approaches during introduction and stays in hero-accessible2042..3279 bounds. The on-foot ambush releases after landing at168ticks; optional horse mounting is not implemented. Cinematic runners stay outside combat and finish leaving. Real melee opens the final gate, then walking crosses the bridge into the boss level. Camera encounter bounds stop scrolling before the hero reaches the visible edge. Frog camera excludes transparent source margin at136; movement minimum168 keeps the hero visible and within reach of portal152. Sleeping enemies use source lying/breathing clip and source hit-triggered delayed wake checks; the separate rolling clip is not called by that source wake sequence.

Boss burial uses original PLAYER1680 entry29, alternatingRIGHT/LEFT eight times, then emergence38..41. Spare boss-only prop fields carry state; packet cooldownbit64 plus animation1..41 carries it to guests. Reset on entry/exit and never enlarge packets. Humanoid campaign clashes identify their participants by POSE_CLASH; giant boss excluded, endingPvP retained. Original damage feedback must not interrupt boss attack animation. Punch/spikes can hit reinforcements, with ordinary corpse/drop lifetimes.

Buried artwork shares palette/XOR variants and is partly banked to preserve both48KiB memory reserves. Boss-bank spare space is now very tight: always rebuild bank metadata and run memory/audio workspace checks after art changes. No new full-screen buffers. Hardware FPS and physical wireless remain unverified by native tests.

Trailer request: AFTER all game fixes are tested and deployed, capture the updated engine again, keep the action-led visual format, add combat from more areas, replace jokey captions with accurate DS/gameplay descriptions and fix the Frog view. Never present native engine captures as actual DS/emulator performance or a physical wireless session.

# v0.36 boss and chapter QA (2026-10-04)

Current delivery: v0.36; authoritative ROM hash and memory figures are in build/release-manifest.json. Supersedes older status below. Route: Home Castle -> Castle Keep -> Barbarian War -> bridge -> Barbarian Boss without mandatory map return. Boss is map11, chapter bank4, map node4; clearing War unlocks it and boss completion uses bit4 of completed. Save format3 remains112bytes and migrates versions1/2. Campaign version36 rejects older peers;276-byte snapshots/36-byte requests unchanged. Arena protocol22/gameID0x43415218 adds dash poses30/31. Sprint+A roll and sprint+X dive use source timing; campaign level4 gate and level2 spin are preserved. Eleven-byte local feedback observer detects real level gains, original LEVEL/New Combo labels display supported unlocks, and stats opens on safe map/completion entry.

Boss source ending: intro carries other princesses away; death frame73 chest;29 authored gold births; chest320 rope; cut rope; release33;83-tick multiplayer preparation; duel; winner approaches; full142-tick kiss. Draws replay the duel, guest disconnect cannot bypass it, and imported kiss palettes preserve all four knight colors. Solo skips duel. All29 money items are represented by a seeded layout and29-bit pickup mask in inactive boss snapshot bytes. Preserve these rewards and selected knight/profile on both consoles. Original boss/hero kiss/chest/carriers are banked; source collision and attack cues documented in BARBARIAN_BOSS_AUDIT.md. No source weapon/pet reward was invented. Future source unlocks character5 and nodes18/9 are unsupported.

Music includes original FLUTEY map, NG64061 boss, goldgrab treasure, NG21749 duel and RaceVictory kiss. Blacksmith has original hammer animation/audio. Bank5 count includes arena, Keep, War, hubs, boss. Boss decoded tiles borrow48KiB at bank end, excluded from the audio workspace; preserve that isolation. Never increase common bank capacity or remove either48KiB reserve to fit new art.

QA: native source hashes/canaries,79 renderer scenarios, combat/treasure/ending/packet tests and route/loading/save regressions. Actual physical DS FPS and two-console wireless are not established by these tests. User previously canceled emulator automation; do not restart without renewed authorization. Retain v0.7 boot payloads. Remaining AI/cinematic/inventory adaptations are documented rather than claimed exact.

# v0.35 map and pause update (2026-10-04)

User correction: the castle must have both original map stops. Previous v0.34 omitted node19 and mislabeled node13 as Castle Keep. Source MAP connects node13 (Home Castle), node19 (Castle Keep), node11 (Barbarian War). Keep the connected gameplay route, but allow direct Castle Keep entry at its own stop. Boss node7 remains unsupported. Normal progression must continue Home Castle -> Castle Keep -> Barbarian War -> bridge automatically. World map remains a manual pause-menu option and the stop after the implemented bridge route.

START menu: solo pause, campaign host pauses shared simulation with radio active; guest menu is local. Host can return party to map, sound toggles locally, Save and Quit uses existing SD save. Arena local menus keep the wireless match running. Campaign snapshot running bit1 carries pause, bit0 running; protocol35 remains276B. Input/profile36B unchanged.

Map: chosen knight animation, one/two ready heroes, host-authoritative timed walking and camera following. Use existing scenery scratch to cache tiled map, no extra full bitmap allocation. Original single-frame parchment artwork is color-adapted by vector materials, not a claim of recovered original full-color variant. 84 tiles of32x64,128colors,59,995compressedbytes; source audit assets/worldmap-color-audit.json.

Preserve v0.7 ARM7/ARM7i, unitcode2, banner CRC, two48KiB runtime reserves and scene-loaded handshake. Do not claim physical DS performance or two-console testing from native tests. Older baseline follows.

# Castle Crashers DS continuity and acceptance baseline

Updated 2026-10-04. Latest released build: v0.34, SHA256 68cbcddefb388f1bef081ef4a8b616d40a70c421d30d3a99b100612bb42e440e. Delivered v0.33 remains preserved (SHA256 fa481acc91d3f6c79c133d737bd45e04d39e9f560764a2a871e03ead24fbea70). User praised v0.32 first-level quality and requested preserving everything that made it work, then second level, world map, blacksmith, animal ark/pet room, and source-faithful pet pickup/swap.

## Scope and sequencing

Finish and package v0.33 aerial combat/default-combo update first. Next extend beyond Castle Keep into the verified original next level. Then add original world-map flow, blacksmith and animal ark. Mr. Buddy is buried in the barracks; the user explicitly chose to retain the original later-shovel requirement. Do not grant an early shovel or loose pet. Equipping a new pet should release the previous one as a world pickup; keep ownership distinct for both players. Source IDs, gates, behaviors and adapted parts should be documented.

## Non-negotiable existing behavior

- Preserve the DSi-working v0.7 ARM7 and ARM7i payloads byte-for-byte; unitcode2 and valid banner CRC. TWiLight Menu++ SD launch previously white-screened with other boot payloads.
- Native local DS wireless, no internet requirement; preserve host-authoritative simulation and room/knight selection. Two heroes, six enemies and an ally are already reserved. Do not claim physical two-console validation from native tests.
- Preserve 48 KiB radio plus 48 KiB additional runtime reserves. v0.33 has only292 bytes beyond them. New levels/art require a scalable storage/streaming plan, not simply more linked asset arrays or reduced reserves. Campaign host276 bytes / input36 bytes. Version/broadcast IDs must reject incompatible builds.
- Parent FTL project stays untouched. Keep changes inside castle-arena.
- User canceled native/emulator automation with Escape earlier. Do not reopen that automation without renewed authorization. Native tests and generated image inspection are available; do not claim melonDS or hardware FPS from them.

## Presentation and performance that must survive

- First barracks introduction is composed opaquely into the hidden page. Do not write live terrain during its vertical camera movement. Switching to transparent gameplay restores terrain first, then changes complete image/camera/background together.
- Render cadence targets two refreshes, no catch-up bursts; movies use15fps source-derived artwork. Slow frames can still miss deadlines: tests are not hardware FPS measurements.
- Horizontal terrain ring uploads only newly exposed columns; vertical rebase/partial rows and reserved overlay VRAM rows are tested. Avoid per-pixel software division and new full-screen buffers.
- Foreground artwork is prebaked, then restored only within actor/effect dirty spans. Do not expand spans recursively across wall strips. Foreground tests compare against full composition.
- Handrail depth follows actual solid BSP2 stair risers. Barracks near wall uses original sprite305 and includes row182; departing NPCs use original static foreground depth. Princess doorway uses original LEVEL foreground alpha to remove only obscured actor pixels at conversion time.
- Cinematic caching reuses existing workspace; peasant/shop caches borrow only unused enemy buffers, have explicit ownership invalidation, and fall back safely when all enemies are present.
- Every actor has its own corpse blink/removal clock. New waves never instantly delete old bodies or block slot reuse merely because drops are full.
- Both heroes' held weapons are visible in authored artwork; picking a new weapon drops the old one. No weapon equip menu. Health remains bottom screen; contextual square items only appear when owned.
- Holding magic keeps a stable readiness effect and preserves locomotion. Running starts after30 grounded movement ticks, keeps momentum across direction/depth changes, and uses modest original dust.

## First-level completeness baseline

Continuous barracks/keep/courtyard/shop route,19 enemies and multiplayer-friendly transitions; no teleport before princess scene. Original NPCs and breakables, source shop boards/items/price/selection/purchase/failure artwork, original shop potion and five source food variants. Shop belongs in-world; bottom health/inventory remain.

Enemy loot rolls when the corpse disappears. Original20-frame bounce/pickup delay; independent age, late blink, expiration at frame304. Four visible items,48 queued overflow; visible lifetime begins on appearance. Ground weapons persist. Packed ages retain276-byte snapshots. Purchase uses coin feedback; food feedback reports actual healed health, not packed variant bits.

Campaign starts with source low stats, per-hit XP, verified level thresholds, source weapon attributes and per-knight saves. Difficulty/AI remains reconstructed. Keep readable windups/recovery/mercy, lane changes/flanking, directional shield behavior and no silent claims of exact original AI.

## v0.33 combat baseline

A=light, X=heavy, B=jump, R=block, L=magic, Y=current item/no-item. Level1 four-light chain, three-ground-heavy chain (third is downward slam), airborne light/heavy chains, close throw, contextual stomp beside downed targets. Stomp scans nearby opponents, can hit behind, strict30x10 feet range, source hitframe9. Thirdheavy hitframe4. Source keyframes preserve12/24-tick timelines across knights/held weapons without new sprite buffers.

Air light has one authored collision tick (frames3/5) and follow-up markers6/7. It pauses attacker7 ticks without resetting vertical velocity. Victim is never given matching float or upward rebound; ground launch9 vs airborne re-hit4 and horizontal separation6 are integer half-scale adaptations. Air-heavy rebound7 is separate; spin and second air heavy do not get generic float. Level2 second-jump spin; level8 A-X ground/air uppercut. Later unlocks4/16/32/50 were audited, not fully implemented. Existing six-hit escape remains a DS adaptation. First-level barbarians do NOT have increasing weight (source0/0); avoid inventing that penalty. See source audits.

## Validation/release workflow

Run check.ps1 after related changes, including meaningful native combat and deterministic packet checks, source RGB555 hashes and canaries, full route, reference/overlay image sequences, foreground/cinematic/background transitions. Build with build.ps1; memory check must pass unmodified. Original knight palette merging is RGB555-lossless. Common enemy walk/attack/hurt/dead/bow retain fast RLE; only other poses use bounded Huffman packing.

Package using the latest build/package-vNNN.py pattern: verify original ARM7/ARM7i and embedded audio, correct bannerCRC, unique versionedROM and ZIP, SHA256, memory report, release manifest and concise honest limitations. Preserve prior releases. Both consoles use the same version. Link the exact built ROM; do not substitute a plan for delivery.


## v0.34 extension and multiplayer map acceptance

Barbarian War is original LEVEL11 plus LEVEL12 bridge. Giant Barbarian Boss (LEVEL7) is the next unsupported map node. Keep3/4/5, War6/7 and hubs8/9/10 each share a bank. Hubs also hold the original western world map. All banks reuse852608bytes. Never add per-wave disk reads or reduce wireless/runtime reserves. Audio uses checked aligned bank tails; stop it before changing banks. Keep/PvP asset bytes remain identical.

World map is phase22; selectednode uses the group byte. Host owns selection. Nodes0Blacksmith,1Keep,2War (requires clearedKeep),3futureBoss (unavailable). Original map artwork/node positions are used. Keep/War cumulative clearbits1/2 survive saves. Blacksmith source doors lead to Ark/Frog/worldmap. Only supported ownedpets/weapons are interactive; no earlyshovel or Snoot grant.

Multiplayer remains HOST/CLIENT on the map. Host pauses simulation at each destination until guest sends CAMPAIGN_REQUEST_LOADED after loading chapter data. The36-byte request validates exactmap/worldmap stamp and expectedbank. Reject obsolete/reordered replies. Do not permit worldmap travel while the current scene awaits acknowledgement: a fast return to the same map otherwise deadlocks. Both sides have extended loadinggrace; keep the room connected. Host276bytes/reply36bytes stay unchanged. Campaignversion34/arena21 and new gameIDs reject older builds.

War source fixedencounters, queued sevenenemywave, twoinitialgrayallies, mountedsequence, WarMachine, sleepers and optionalwrestling/tree/grass are included. Machine multiplexes actor8 when allyunused. Packetround carries Warflags/brokenprops; countdown carries gateHP+1 for strictnegative opening. Machinebody uses halfwidth65/height95, ram2/cannon10, gate melee7/arrow1. Cannon muzzle/aiming and cinematic positions remain adapted. Owner8/map6 cannon permits velocity down to-64 for its taller spawn; ordinaryprojectile validation remains unchanged.

Pet ownership is perhero and save-migrated. Picking up a newpet releases the oldpet into the same slot with51tick delay; source unlock shares only with livingparticipants. User explicitly required MrBuddy stay buried atbarracks(155,286), requiring ownedshovel and actualdig completion. Fullpickupslots leave it retryable. Dig uses PLAYER1102 frames, completion39(Buddy34), duration76. Arkbin returns ownedpets to theirbarns. No earlyshovel is granted. Collectedweapon flags persist for Frogstands; physicalswaps leaveoldweapon onground and no equipmenu is restored.

Native checks cover originalframehashes/guards,64 reference/overlay/bankedscenes, firstlevel30000snapshots, War/co-op30000snapshots, originalgeometry, walkingallhubdoors, progression/saves and loading-request sequencing. These are not physicalwireless or DS FPS tests. HubambientNPCs remain static sourceposes; AI/cinematics are adapted. Do not reopen canceled emulator UIautomation without renewedauthorization.

Release verification: check-v034-final.log passed; DS build retains93700bytes beyond two48KiB reserves. Packager verified originalARM7/ARM7i and allnine embeddedNitroFS files. Physical two-console/DS FPS verification remains outstanding.


## v0.37 bottom-screen contract

`bottom_ui.c` provides shared campaign/arena/map/hub cards; `ui_skin.c` provides clipped parchment panels and item tiles. No additional framebuffer or chapter bank allocation. Health/mana are normalized per hero and absent guests stay invisible. Item squares remain shovel x34..85, potion/bow102..153, stats170..221 at y118..169. Stats use original MENU647 icons (544 bytes static,256 decode bytes), existing y90..169 rows and back x214+ y66..87. Keep host request/ack spending, original shop boards and no equipped-weapon menu.

World-map card y64..112 x8..247: x<34 previous, x>=222 next, middle enter. Only host/solo injects navigation into existing world-map controls. Guest uses read-only host/travel context. HUD cache tracks dirty-save and nearby weapon changes without invalidating on every movement tick. Both runtime48KiB reserves remain unchanged. v0.36 ROM/archive/snapshot remain untouched; v0.37 retains wire/save compatibility.


## v0.38 main-screen quick inventory

Supersedes the v0.37 separate shovel/potion touch squares: shared square x64..115 y118..169, previous x30..57 and next122..149 in same y range. Stats remains170..221. Arrows equip automatically; square taps use; Y preserved. Ownership mask never grants items. Fresh tools=0; no earlyshovel. UI fixtures can show four slots but normal campaign only implements potion/ownedshovel. Bow/sandwich acquisition and sandwich gameplay remain future work. Local cursor is normalized when inventory changes; guest switches retain host authority and tap-use waits for ack. Source overhead selector art lives selector_art.c,4455B; originalbow feedback unchanged. Keep new C dependency in native renderer builds. ROM/protocol36/22 and savev3 unchanged. Previous releases untouched.


## v0.39 sequential campaign access

Home Castle completion now uses bit8 at the actual transition to Castle Keep. Keep/War/Boss bits1/2/4 retain meaning. campaign_world_unlocked is shared by UI and entry validation; any later completion implies preceding access for legacy saves. Hubnode0 and first campaignnode1 stay available. Nodes2/3/4 require previous chapter. Normal continuous transitions remain. Saveformat4 accepts1–3 and preserves all fields;112B file/24Bcharacter unchanged. Campaign snapshot runningbit3 carries HomeCastle8; packet276/reply36 unchanged. Campaignprotocol37 and gameID0x43414337; both consoles needv0.39. Arena22 unchanged. DS memory gate keeps8,900B beyond both48KiB reserves.

Trailer delivered in trailer/Castle Crashers DS - Gameplay Trailer.mp4:38s1440x1080, gameplay-first edit, original lettering, nativeengine footage explicitly labeled. Not DS/emulator FPS or physicalwireless evidence. Reference agent visually inspected Behemoth and DS Kirby trailer samples in browser; sound not heard. Replay/rebuild and capture provenance live undertrailer/ and tools/trailer_*.
