# Next-stage source audit

Audited local installed original assets, 2026-10-04. Do not infer level names from numeric IDs.

## Verified route and map grouping

- LEVEL19 `frame_1/DoAction.as:f_LeaveLevel2` marks19 and14 completed and unlocks11.
- LEVEL11 `f_LeaveLevel2` marks11 complete/visited and unlocks12.
- LEVEL12 `f_LeaveLevel` marks12 completed and unlocks7.
- MAP `frame_1/DoAction.as:f_GuyInit` remaps12 to node11,15/16 to14,2/59 to13.
- MAP `f_MapInit` links node13→19→11→7 and node14 beside13. Consequently the next complete map node is LEVEL11 plus LEVEL12 bridge continuation. **LEVEL12 is not the giant Barbarian Boss. LEVEL7 contains `f_BarbarianBoss` and loads `ebarboss.swf`.** This was confirmed by searching all locally decrypted level archives, not a guide assumption.
- Bounded implementation endpoint: complete11+12, unlock node7, return to map. Do not silently include7.

Files: `assets/level11-scripts/scripts/frame_1/DoAction.as`, `assets/level12-scripts/scripts/frame_1/DoAction.as`, `assets/map-scripts/scripts/frame_1/DoAction.as`.

## LEVEL11 Barbarian War

Art scene376 in `assets/extracted/level11-full.xml`; physics `assets/extracted/bsp11.bin`. Original SWF declared lengths can be stale: use normalized `*-full.swf` and complete XML. Proposed DS map6 origin source(-2112,-700), world=(source-origin)/2, canvas4608x448. BSP physical bounds x[-1969.56,6978.68], y[-325.03,8.686]. BSP waypoint table begins after20+edgeFloatCount*4 bytes and contains23 nonzero triples, WP0..22. Exact converted points and fixed spawn anchors are in `source/war_sequence.h`.

| Trigger | Fixed actors | Source anchor |
|---|---:|---|
| Initial entrance |5barbarians +2GrayKnight friends(type6)|WP1 offsets; friends(-100,0),(-50,+50)|
| WP3 |5barbarians|WP4 offsets; two approach fromfar-right|
| WP5 |6barbarians|WP6/WP7 plus twofromWP3|
| WP8 |2thieves|named dead1/dead2 x,y-1; search pose|
| WP9 |7barbarians|WP10 plus threefromWP7left|
| WP10 |horse/cannon sequence|intro movie363|
| WP12 |WarMachine intro/fight|movie364; machineatWP14+450,WP13y|
| WP13 |4barbarians|WP14 offsets|
| WP14 |4barbarians|WP15 plus twofromWP13|
| WP15 |4sleepingbarbarians|namedbed1..4; activateswoodgate HP150*players|
| WP19 |2barbarians|WP21 offsets|

Nine fixed groups total39 enemies, excluding machine, machine summons, horse-sequence runners, and optional wrestlingpair. Preserve pending seventh actor if only six live slots fit; do not erase corpses to make room.

Additional authored content:

- Scene376 sprite303 wrestlingpair is hittable; `DefineSprite_303/frame_1/DoAction.as:f_Hit` spawns1barbarian plus1GrayKnight ally, incrementskillgoal. It is not a static backdrop.
- `f_Level11LimitUpdate` defines four reinforcement barbarians atWP11/WP10, but no caller was located in LEVEL11/MAIN/EWARMACH scripts. Do not invent when to call it; inspect native/BSP callback integration before claiming it belongs to a fixed wave.
- `f_Main` has optional `barb_attack` recurring melee reinforcements (timer100→70, active-player-dependent cap). Its enabling assignment likewise remains unverified.
- Horse movie363 startsAutoRun atframe2, spawns11 fleeingmelee barbarians at15,21,27,33,39,45,51,57,63,69,75; cannonattack113, playersnuked128, camerareset238/244, stop245.
- Machine movie364 spawn45; walls change75..78; playerrelease126; alternate authored end267. Branch/label selection must be resolved, not simply hold all267frames as a static screen.
- Scene actors/props include gate133, tree172, flagdude247, wrestling303, grass316, portals358, story363/364. Neutral fences/corpses must retain depth occlusion.

## War Machine source behavior

LEVEL11 `f_SpawnWarMachine` uses **250+100*activeplayers HP**, sourcewidth130,height190; source resists melee50/fire40/electric50/poison70/ice50. World placement is(3279,290) for agreed origin.

`f_WarMachine` modes1/2 ram left/right at15sourcepixels/tick; mode3 tracks a player at up to4sourcepixels/tick, maintains roughly150..250sourcehorizontal spacing, timer400; mode4 shakes/windsup46ticks before another charge. Three aimable cannon barrels, projectile speed40/gravity1, damage10; barrel clip14 fires atframe9. Custom comparison opcode70 is omitted by FFDec: conditions such as `mode_timer>0` in readable AS can be inverted; use raw bytecode before final transition implementation.

EWARMACH sprite59 jack/summon frames42 and75 each spawn a barbarian ifactive_enemies<3 and incrementkillgoal. Its death sprite106 plays188frames: explosion chain, frame128switchdie2, frame129wreck becomes nonpunchable object, frame188clearsbossflag and healthbar, callsEnemyDieStay and restartsMUDHOLES music. Assets/scripts: `assets/ewarmach-scripts` and `assets/extracted/EWARMACH.swf`.

## LEVEL12 bridge continuation

Scene94, `assets/extracted/level12-full.xml`; map7 origin(-1024,-800), canvas1024x448. BSP bounds x[-898.143,672.325], y[-347.688,-90.436]. Six waypoints0..5. WP1 spawns three barbarians aroundWP2(+100,0),(+200,+50),(+250,-30). WP2 starts authored bridge/story sequence and stopsplayers. No giantbossfunction orassetload exists in this level.

Story85 setscinematiccamera frame2/5, selects tour/skip branch at25, fadeout34, activatesplayers/camerareset43, fadein44, stop46. Door48, doorjamb75, crow74 need original foreground layering. Completion unlocks7 as above.

## World map and home facilities

Original resources: game/map.pak→MAP.swf (game/map_new.pak alsoinstalled); level14.pak→blacksmith exterior, level15.pak/level15_new.pak→WeaponFrog interior, level16.pak→AnimalArk. `LEVEL14` has `s_Blacksmith` in sprite72/frame13 and three entrance branches start/arc/frog. It links to15/16; MAP groups bothinteriors under14.

LEVEL16 f_InitAnimals iteratesbarn1..29: show animal ifanyactiveHUD hasunlocked it, suppress whenalreadycarried byanyactiveHUD. Cheat1 override is not default ownership. f_Trash sends heldorb back to itsbarn andclearscarriedpet. No evidence of allpetsavailablebydefault. Read `docs/pets-source-audit.md` for independentpetstate audit.

## First-area animal: buried, not a free pickup

LEVEL2 scene378 child320 depth599 loadscript sets **animal_type24**. Sprite320 registers a shovelspot. Sourceposition(-189.9,-141.55) translates to DS(155.05,286.225) usingexistingbarracksorigin(-500,-714).

PLAYER digclip1102/frame39 only aftersuccessfuldig callsPlaceAnimal(type,spotx,spoty), setsunlock=true andbody_y0. The shovel gate is real. Animal24 helpsdig in PLAYER1102/frame7 and34; installedlocalizedEnglish refers to 'Mr Buddy helps'/'dig faster'. This supports the familiar MrBuddy identification; numericID24 anddigbehavior are directscriptfacts. The user explicitly chose to retain the shovel requirement: do not putalooseanimal thereatstart.

The original shovel acquisition is **LEVEL41 f_GetShovel**, `assets/level41-scripts/scripts/frame_1/DoAction.as` nearline653: livingplayer withinstrictdx50/dy20source, body_y>-30 orhorse; unlockitem2, equip2, playweaponpickup, hidegroundshovel. This is laterprogression; no shovelgrant was found in initial11/12 content.

## Original orb pickup/swap

MAIN `f_AnimalLost` state150: settle body_y toward-35; count downignore_timer; afterexpiry nearestplayer withinstrictdx50/dy20source canattach whenownerprofileunlocks[type] OR animal.unlock OR Cheat1. `f_AttachAnimal` firstcallsUnAttachAnimal(existingpet), thenassignsnewowner/zone.pet, resetsstate/timer andfollow; discovery unlock flags arepropagated toaliveplayers. `f_UnAttachAnimal` leavesoldorbphysicallypresent, ownerundefined,lost,state150,ignore_timer51,love_timer0. Thus replacingpetdrops theoldone with51tick regrabdelay, notdeletion or instantback-and-forth swapping. Ownedprofile/unlock predicates requirehost-authoritative validation.

Source boundaries: decompiled `§§pop()` and customUnknown70 conditions are not trustworthy exactexpressions; documented coordinate/constants anddirectactions areverified. Unresolvedcinematicbranch/callbackconditions areidentifiedratherthaninvented.

## Runtime adaptations and audited boundaries

The DS War controller uses all 39 fixed spawn anchors, retaining dead bodies and queueing the seventh member of the WP9 encounter until a slot becomes free. It retains both initial gray knights. The source horse run schedules all eleven fleeing extras; when six surviving corpse slots prevent an exact source-frame entrance, extras wait for a free slot rather than replacing a corpse. The original nuke velocity of 100 source units becomes a compressed DS 16-unit launch, peaking below the network/render limit of 80 units. This is an explicit framing adaptation. The machine's tracked acceleration is a fixed two world units per tick, and its fight is bounded to world x2042..3279 so heroes cannot leave the active encounter. These are not exact simulations of the source's fractional motion/camera leash.

LEVEL12 starts with exploration and triggers its three Barbarians at waypoint1 (world337). Its exit guide is scene94 sprite80, depth119, source position607.6,-265.3, or world815.8,267.35. The controller's exit proximity begins at world804 after the bridge story. It does not use unreachable waypoint5 at world938 as a foot target: BSP12's outer corridor ends at world848. The exact custom-bytecode portal hit predicate is not yet decoded; x804 is an explicit entrance-proximity adaptation.

`tools/convert_war_geometry.py` derives the outer walk corridor from BSP11/12 edges1/2/3, adds the same five-pixel foot margin used by other DS scenes, blocks columns outside the authored corridor, and losslessly stores repeated column bounds as runs. Decorative or interactive interior collision is not inferred from the outer corridor.

## Implemented hub source mapping

Blacksmith map8 uses LEVEL14 scene112 and BSP14, origin(-512,-700), 768x448. Its three door99 placements are door1(-249.3,-35.55), door2(-121.5,-156.55), door3(459.15,-137.85). They respectively return to the world map, enter Animal Ark, and enter Weapon Frog. The entry anchors p1_start/p2_start, p1_arc/p2_arc, and p1_frog/p2_frog are retained. Doors use a 24px proximity rectangle and a 30-tick arrival debounce: these are DS interaction adaptations rather than a claim of decoded portal bytecode.

Weapon Frog map10 uses LEVEL15 scene207 and BSP15, origin(-384,-700), 3200x448. The authored weapon racks are w2(709.45,-204.6), w9(1557.2,-202.1), and w19(757.85,-204.35). The source rack initializer begins at weapon2; no starter sword1 stand is invented. Only the three supported collectible weapons are displayed and selected. `f_InitWeapons` makes a rack visible when an active participant owns it; `f_CheckGetWeapon` still checks the collecting participant's ownership and weapon level. The DS A-edge interaction keeps those restrictions. As requested for this prototype, swapping also leaves the previous weapon as a persistent pickup; when all four pickup slots are occupied the swap waits, rather than destroying loot. This ground-drop detail extends the original rack selection behavior.

Animal Ark map9 uses LEVEL16 scene98 and BSP16, origin(640,-760), 2816x448. Entry p1_start is (4123.05,-136.9), p2_start(4090.05,-169.5), and door1(4348.15,-229.55). Supported barns are barn17(3184.2,-216.45) and barn24(3396.3,-44). `f_InitAnimals` requires at least one active profile to own a pet and excludes pets currently carried by either hero. It never grants either supported pet on entry. Pickup/swap uses the shared host-authoritative pet item path, including leaving the previous orb with its 51-tick repickup delay. Trash sprite65 at scene98 depth195 is (4397.7,-61.45); its enterFrame calls `f_Trash`. The original 70x50-source-unit proximity returns the carried pet to its barn. The DS implementation moves it directly to the barn pickup; the original travel-to-barn animation is not reproduced. Pet17/Snoot has no invented first-area acquisition. Pet24/Mr Buddy remains buried in the original barracks and requires a shovel.

The Ark's remaining 27 pets, its veterinarian text, secret pet acquisition interactions, and the Frog's other weapon types remain unsupported. Their room art is not evidence that they have been unlocked or implemented.

### FFDec custom portal-guide tag

LEVEL14 and LEVEL15 contain source tag148 symbols96/188, each wrapped by door-guide chains97/98/99 and189/190/191. FFDec stops parsing after this nonstandard tag. `tools/prepare_hub_audit.py` substitutes a transparent one-frame symbol for this guide only, allowing the original later room scene112/207 tags to be parsed. This is an audit/export adaptation; it must not be described as a repair of corrupted original assets. The art pipeline hides the runtime-invisible portal guides anyway. LEVEL16 does not require this substitution. Complete Ark clip-action evidence is in `assets/level16-full-scripts` (the initial declared-length script export omitted its trailing room actions).

## War prop and sleeping-enemy behavior

Scene376 wrestling pair303 at source(-759.85,-85.3), world(676,307), registers both punch and arrow `f_Hit`. The handler raises the kill goal by one, tosses a Barbarian, makes fighter6 friendly, sets the prop inactive, and plays its hit animation. The DS controller queues those two actors independently until enemy/ally slots become free; it never removes an existing corpse to make room. Broken bit0 selects the authored finished prop.

Tree172 at source(4004.25,-222.3), world(3058,239), calls MAIN `f_SetTree`, which starts health3 and accepts punch/arrow/juggle hits. `f_HitTree` decrements health and changes body to frame2 once exhausted; `f_RandomTreeItem` produces one of food2..6 at the zero transition. The p-code includes `Greater; Unknown_70; Not`, which the plain decompiler incorrectly expresses as a contradictory positive-health/zero-health branch. The DS controller counts three distinct contacts, then produces one food, with no repeated farming. Its falling fruit uses the existing DS pickup bounce rather than a separate original treefall trajectory.

Grass316 has seven placements at source(1710.25,-223.4),(2006.25,-217.4),(1715.25,-126.4),(1989.2,-76.4),(2376.2,-62.4),(2566.2,-140.4),(2138.25,-10.4). The first six instance-load scripts explicitly set item_type4,3,4,5,2,3; these fixed authored drops are intentional. The last has no fixed item: MAIN melee grass handling rolls random(3)==1, then food random(5)+2. Cut grass selects frame2 and never drops twice. Pet12's guaranteed-grass-food bonus is outside the supported pet subset. Broken bits2..8 track these seven placements.

LEVEL11 waypoint15 sets `_root.sleep=true` and starts all four bed Barbarians at `sleep`. MAIN `f_MeleeCheckHit` sets global sleep=false on successful contact. PLAYER1886 checks at frame30 and60: once sleep is false, `random(14)==1` returns to standing. The DS AI does not wake sleepers by proximity. A directly hit sleeper reacts immediately; the other sleepers make the authored one-in-fourteen wake roll every30 ticks after contact. The independent original sleep timelines are synchronized to the DS encounter clock, a timing adaptation. Broken bit1 tracks the tree's spent fruit; War flag16 records the released sleep latch.
