# Pet and shovel artwork audit

These assets are extracted from the locally installed game. There is no generated replacement art. The export uses isolated `pet-common-*` / `dig-common-*` files and FFDec settings under `build/pet-art-appdata`.

## Source timelines

- `assets/extracted/ANIMALS.swf`, selector sprite50: its action is `gotoAndStop(_parent._parent.animal_type)`. Frame17 is Snoot; frame24 is Mr Buddy. This selector is frozen to the requested ID, not allowed to cycle through other pets.
- ANIMALS root `follow` frame3 and `lost` frame4 both use sprite51, fourteen authored hovering frames. Runtime body elevation/lost music are separate from this body animation; the renderer supplies follower/drop elevation.
- `happy` root frame9 uses sprite60,49 authored frames. Export samples1,5,9,...49 preserve body rotation/squash. Script-triggered ancillary glow/music effects are not baked into these body sprites.
- `dig` root frame12 uses sprite75. Its frame1 action selects frame5 for animal_type24, which places sprite74 at source offset(2.05,-19.95); sprite74 has16 frames. Only Buddy receives this special digging pose. Selected frames1,4,7,10,13,16 retain that placement.
- PLAYER1102 contains76 frames, with label `finish` at39. Frame7 dispatches Buddy toward the dig spot; frame34 skips directly to finish when Buddy is equipped, otherwise waits for the runtime dig action. Frame39 creates the authored reward only after a successful dig. The exported frames are1,7,13,19,25,31,34,39,45,51,57,63,69,76. The caller owns source timeline advancement/waits; this asset does not make shovel ownership or dig completion decisions.
- All four knight color variants use the genuine1102 shovel timeline. No held sword or weapon variant is composited over the shovel.

## Raster and packing

The source canvas is384x240 with actor origin(192,176); Lanczos half-scale gives192x120 with origin(96,88). Cropped frame x/y offsets are relative to that original actor origin. Alpha threshold100 and the existing knight palette match the project's established sprite conversion; pets use their own255-color palette. Compression then preserves every resulting indexed pixel exactly. It uses bounded Huffman-coded RLE and at most one color-delta reference to a blue frame, with no heap allocation or recursive chains.

The selected source PNG SHA256 and indexed-pixel SHA256 are recorded in `generated/pet-art-audit.json` and `generated/dig-art-audit.json`. Native expected RGB555 FNV hashes are in the paired test-hashes files. Estimated common static data44.3KB, maximum decoded pet952pixels and knight3393pixels. Selection is an explicit sample of authored frames, not a claim to preserve every intermediate source frame.

## API and validation

- `pet_art_frame(source_id,pose,tick)` takes source ID17/24 and zero-based source tick. FOLLOW loops14 frames; HAPPY clamps at49; Buddy DIG loops16. Lost uses FOLLOW with caller-provided vertical displacement. Invalid pet ID returns PET_FRAME_COUNT.
- `dig_art_frame(color,source_frame)` takes color0..3 in blue/orange/red/green order and one-based PLAYER1102 source frame. It holds the most recent selected frame. Invalid color returns DIG_FRAME_COUNT.
- `pet_decode_frame` / `dig_decode_frame` accept the returned index and an existing caller buffer. Frame metadata and palettes are supplied by the corresponding header. Decoders reject invalid IDs/null outputs and validate stream and crop bounds.
- `tests/pet_art_test.c` verifies all98 selected source pixel hashes repeatedly, prefix/suffix canaries, invalid inputs and timeline lookups. `tests/pet_art_source_test.py` pins all selected source PNG inputs.

Rebuild: run `tools/export_pet_dig.py`, then `tools/convert_pet_dig.py`. The latter reuses the tested knight codec generator without modifying existing knight assets.
