# Next-level art and memory plan

Status: source/pipeline audit after v0.33, followed by the approved existing-asset bank prototype described at the end. The current build has only 292 bytes above the mandatory 48 KiB heap plus 48 KiB wireless reserves. The parent approved the architecture and assigned the loader/asset wrappers; gameplay integration remains with the parent.

## What the next content actually is

The original next world-map node is Barbarian War. The source-level route leaves LEVEL19 for LEVEL11, then LEVEL11 unlocks/leads to LEVEL12. LEVEL11 includes the war-machine encounter; LEVEL12 contains the three-Barbarian encounter and bridge cinematic; the giant Barbarian Boss is a different source level and is not implied by the filename12. The sequence audit reports that the original map associates LEVEL12 with node11. Completing only another generic Barbarian wave would not complete this requested second node.

Local source evidence:

- `assets/level19-scripts/scripts/frame_1/DoAction.as`: completion unlocks11.
- `assets/level11-scripts/scripts/frame_1/DoAction.as`: source level11, BSP11; starts with five Barbarians and two allied gray knights; later waves include bow users, thieves and sleeping enemies. `f_CreateWarMachine` loads `../ewarmach/ewarmach.swf`. Its initial machine HP is250+100*active_players. Completion unlocks12. Authored source requests20 enemy slots, which exceeds the current DS six-enemy implementation and needs deliberate encounter scheduling rather than silently dropping enemies.
- `assets/level12-scripts/scripts/frame_1/DoAction.as`: source level12/BSP12; completion unlocks7. Existing source XML and SWF are available.
- Cached `assets/extracted/campaign-levels/LEVEL14.swf` contains blacksmith and exits to LEVEL16 and LEVEL15/LEVEL15_NEW. LEVEL15 has WEAPONFROG music/weapon inventory logic; LEVEL16 has `f_InitAnimals` and unlocked animal placement. These are the Blacksmith, Weapon Frog and Animal Ark rooms, respectively.
- Original map asset exists at `G:/SteamLibrary/steamapps/common/CastleCrashers/data/game/map.pak` (524,540 bytes); `map_new.pak` also exists. MAIN refers to `../map/map.swf`; choose the matching classic map branch, not the modern alternative blindly.
- Original animals and machine assets exist as `data/game/animals.pak` (64,964 bytes) and `ewarmach.pak` (96,630 bytes). These source-file sizes are NOT estimates of final DS runtime art.

The complete LEVEL1..61 source SWFs (with original naming gaps/variants) are already cached under `assets/extracted/campaign-levels`. LEVEL11 is982,620 bytes, LEVEL12 is108,636 bytes, LEVEL14 is102,900 bytes, LEVEL15 is172,484 bytes, LEVEL15_NEW is311,304 bytes and LEVEL16 is106,732 bytes. LEVEL11 scripts and LEVEL12 XML/scripts already exist; no broad frame export is needed merely to establish their identities.

## The first-area pet

The current barracks source contains an actual buried animal placement. `assets/level2-scripts/scripts/DefineSprite_378/frame_1/PlaceObject2_320_599/CLIPACTIONRECORD onClipEvent(load).as` sets `animal_type=24`. Sprite320's frame1 action calls `f_RegisterShovelSpot(this)` and stops. Therefore the authored first-area acquisition is a shovel spot, not an ordinary visible pickup. The display name and digging/unlock gate must be confirmed from the animal/localization source before presenting it to the player. Do not invent an always-visible animal or silently grant it at level completion.

MAIN already provides source behavior references: `f_AssignAnimal`, `f_AttachAnimal`, `f_UnAttachAnimal`, `f_PlaceAnimal`, and `f_SetAnimalTimer`. A selected pet needs an owner, visible follow animation, correct ability/stat effects, persistent unlock/equipment and deterministic guest replication. Existing `f_CreateAnimals` calls alone are not a pet implementation.

## Exact current payload budget

Numbers below are existing linked compressed payload array lengths, not source SWF sizes or new-level estimates. Small palettes, frame tables, collision data and decoder code remain outside this initial move.

| First-level payload | Bytes |
|---|---:|
| Keep64-row terrain chunks |183154|
| Keep foreground |30340|
| Shop terrain |72896|
| Barracks terrain |127987|
| Barracks actor frames |74656|
| Departing-guard choreography pixels |13386|
| Barracks door animation |5996|
| Keep breakable cart data |8798|
| Cinematic background pixels |75464|
| Cinematic actor pixels |248529|
| Neutral peasant pixels |4859|
| Original shop-board state pixels |6531|
| **First-level total** |**852596**|

Four-byte alignment of those12 chunks requires852608 bytes. This keeps all three first-level maps, cutscenes and dynamic art resident for the entire current route. There are no extra storage reads at ordinary waves, camera pans, doors or shop interactions.

Merely replacing852596 bytes of static data with an852608-byte buffer does not create space for new gameplay code. The useful additional overlap is with existing PvP-only payloads:

| PvP payload | Bytes |
|---|---:|
| All three packed arena terrains |135890|
| Packed arena occlusion |3080|
| Castle arena foreground layers |25440|
| King arena foreground layers |24592|
| Volcano arena foreground layers |26288|
| **PvP total** |**215290**|

The old uncompressed arena composed-pixel arrays are already unused and should not be counted as another saving. Existing sprite caches, terrain staging, radio memory and audio workspace are also not spare capacity.

## Recommended bank architecture

Use one fixed **856064-byte (836 KiB)** bank buffer. Move all above first-level and PvP payloads to NitroFS and remove their linked arrays; do not keep duplicate static fallback payloads in the shipping configuration. A64-byte header and up to32 directory entries of32 bytes fit with the first-level chunks and alignment. The raw data-only net saving is approximately**211822 bytes**, before new loader/pointer code, metadata changes and alignment effects in the final ELF. This is a budget projection: the linked memory report remains the gate.

Bank files:

1. `nitro:/chapters/keep.bin`: every existing barracks/Keep/shop payload listed above. Loaded once before starting or resuming that chapter.
2. `nitro:/chapters/arenas.bin`: all three existing PvP terrains, masks and foregrounds. Loaded before practice/lobby launch. Selecting among the three arenas requires no further I/O.
3. `nitro:/chapters/war.bin`: source LEVEL11 terrain/foreground/encounter-specific actors, machine, relevant enemy variants and pets. Prefer including LEVEL12 too if the measured encoded bank fits.
4. If the measured war node does not fit, separate `boss.bin` only at the original LEVEL11-to12 level transition. Never reload at ordinary waves or camera boundaries.
5. `hub.bin`: world map plus Blacksmith/Weapon Frog/Animal Ark, if the measured total fits; otherwise separate original room transitions. Menu/error UI stays resident so boot and storage failures remain visible.

The current Keep bank is a measured upper-bound example, not evidence that the unexported War/bridge/hub art fits. If a new chapter exceeds856064 bytes, first measure lossless compression, shared original actor frames and authored transition boundaries. Do not enlarge the buffer out of the required96 KiB reserves. Do not reduce source art or promise performance before measuring it.

### File layout and validation

Use a fixed-width little-endian format, never dumped native structs or serialized pointers:

-64-byte header: magic, format revision, chapter ID, payload bytes, entry count, content/build compatibility hash, payload checksum, reserved fields zero.
-32-byte chunk entries: stable chunk ID, byte offset, packed length, decoded/semantic bounds, codec/version, flags and reserved fields.
-All chunk offsets aligned4, entirely within declared file size and bank capacity. Reject arithmetic overflow, overlap, duplicate IDs, missing required chunks, unknown codecs, wrong revision and checksum mismatch.
-Data bytes remain the exact current compressed streams. Existing row and actor decoders receive bank pointers plus validated lengths. Per-scene palettes/geometry/frame tables may initially remain linked; a later directory revision can bank more metadata if measured need justifies it.
-Retain existing64-row terrain granularity, staged VRAM uploads and dirty-row handling. This is compressed-asset ownership replacement, not a return to disk reads per row or full-map decompression.

### Loading without hiding a failure

Loading is an explicit menu/level-transition state. Preserve the last complete displayed frame and a resident progress/error overlay. Do not blank VRAM while locating/loading a file. Parse/check the small header/directory first, invalidate bank generation before overwriting its payload, read bounded chunks while servicing input/network/loading status, validate full contents, then publish the new bank pointers atomically and initialize the map. Begin gameplay only after the whole bank is ready.

This is one in-place buffer, not a double-buffered transaction. Once loading starts, the old bank is overwritten and cannot be resumed without reloading. On failure, retain resident menu/error rendering, never dereference the invalid bank, and offer retry/return to menu. Test missing/truncated/corrupt files explicitly. Avoid a failure path that silently becomes another white screen.

Local co-op host announces chapter ID/content revision and a transition generation. Both peers load locally, exchange ready/error states, and only then resume authoritative simulation. Guest hardware must not be penalized for slower storage. No new synchronization is required for an ordinary encounter because its complete bank is resident.

Cinematic audio currently loads once into borrowed render/audio workspace. Keep its ownership rules intact; do not let chapter loading claim those same bytes. Stop channels reading bank-owned audio before a future bank replacement. Do not assume the radio or a live actor cache can serve as the chapter buffer.

## Reversible implementation order and gates

1. Add the bank packer/validator and host-side roundtrip tests, with current linked data as the independent reference. Keep a build-time legacy backend only for regression comparison, not duplicated in the final ROM.
2. Move existing first-level and arena payloads before importing new content. Bind views using a generation ID; assert stale views cannot render. Preserve exact array bytes, dimensions, palette indices and codec outputs.
3. Compare every current native render scenario, cinematics, menus/lobbies and both-player routes against the legacy backend. Exercise Keep→arena→Keep and interrupted/failed loads. Include all current body/weapon and board pixel checks.
4. Confirm actual ARM9 static headroom remains above both48 KiB reserves and measure the new loader's live allocation/stack use. Loading directly into the fixed buffer must not allocate another chapter-sized copy.
5. Hardware-test DS/DSi TWiLight Menu++ SD launch, normal DS-mode hardware and emulator launch before declaring boot fixed. No GUI/emulator operation was performed in this audit; the prior user restriction remains in force.
6. Instrument storage reads: gameplay/camera/combat must read exactly zero bytes from storage after bank readiness. Compare frame-time distributions for existing peasant entry, combat, camera scrolling and shop scenes. Native timing is useful regression evidence, not DS FPS proof.
7. Export/audit source LEVEL11/12 bridge and hub assets in bounded batches only after the bank architecture passes. Keep source depth layers, breakables, gates and walkable BSP geometry separate; do not bake animated doors or interaction objects into terrain again.
8. Add the source pet, unlock/save fields and replicated owner/selection only after the source placement/ability is verified. Budget its tiny active cache separately; it is not permissible to quietly overwrite an occupied sixth enemy cache.

## Approved bank prototype implemented after this audit

The selected implementation uses an exact852608-byte buffer aligned32, with a32-byte serialized header outside the payload. The chunk directory is a compiled generated manifest rather than a mutable on-disk directory, so it does not consume buffer space. Full expected payload size and CRC fix every chunk offset to the build's manifest; mismatched bank files are rejected. This saves the additional3456-byte pool slack proposed above.

Implemented files: `source/chapter_assets.c/.h`, `tools/pack_chapter_assets.py`, `generated/chapter_asset_manifest.h`, `generated/chapter-asset-audit.json`, and header views for all17 selected arrays. Native builds retain reference arrays unless `CHAPTER_ASSETS_TEST`; ARM9 uses fixed bank views preserving `sizeof` and constant-address initializers. Existing asset generators refresh the bank wrappers through a shared post-generation hook.

`nitrofiles/chapters/arenas.bin` has215292 payload bytes plus32 header bytes; `keep.bin` has852608+32. APIs expose load/ready/error/invalidation and a bounded-read progress callback. Same-bank load returns immediately without opening a file. The loader uses no chapter-sized heap allocation or extra image buffer.

`python tools/test_chapter_assets.py` passes byte-exact bank/reference regeneration, aligned array-view checks, same-bank no-read behavior, all three Keep terrain decoders, missing/short/wrong-version/wrong-bank/wrong-size/corrupt/trailing/truncated file failures and subsequent recovery. Parent still owns main/render/build/check integration, final ARM9 memory measurement and device testing. No War/Boss/hub/pet graphics have yet been exported by this task.


## Implemented additional original art

Chapter 2 now contains the continuous LEVEL11 War and LEVEL12 bridge, authored machine keys, broken fence/tree/grass/wrestling variants, distinct original Thief costume, horse and four mounted knight costumes, and sleeping Barbarian keys. The payload is 700,920 bytes. The 852,608-byte shared buffer leaves 151,688 bytes after artwork, enough for the 124,472-byte chapter music bank without reducing either runtime reserve.

Chapter 3 contains original LEVEL14 Blacksmith, LEVEL15 weapon-frog interior, LEVEL16 Animal Ark, and the western region of classic MAP (672 by 256 pixels). Its payload is 582,516 bytes. The map region has original half-scale origin (128,928); the Blacksmith, Keep, War, and next boss markers retain original relative positions. The untranslated inactive insane-mode label and flame are hidden. Weapon-frog inventory icons are intentionally dynamic, not baked as repeated selector-frame-one swords.

LEVEL14/15 use the audited SWFs where custom tag148 is replaced only in the invisible portal-guide chain. Visible room vector symbols remain original. Cinematic camera-guide symbols are excluded from terrain. Hub NPCs currently retain their authored first-frame appearance; this bank does not claim to reproduce all ambient timeline animation.

Native checks cover all row-range decodes against original generated palette-index references; foreground hashes and output canaries; all Thief pose slots and mounted/sleep keys; original broken-prop bounds; all four bank loads with actual decode calls; and missing, invalid, corrupt, truncated, and trailing-data failures. Bank switching performs no incremental loading during ordinary rendering or waves.
