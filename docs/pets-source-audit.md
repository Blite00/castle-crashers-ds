# Standalone Animal Orb state

`source/pets.c` and `source/pets.h` provide host-authoritative helpers only. Campaign/world placement, render art, input, packet packing and save integration are separate. New profiles own no pets. Supported source IDs are17(Snoot) and24(Mr Buddy); adding Snoot to the supported table does not place or unlock him in an incorrect level.

## Source evidence

- MAIN `f_SetAnimalTimer`, `assets/main-scripts/scripts/frame_2/DoAction.as:11877`: reset all four bonuses to0; case17 strength+4. Case24 has no stat bonus.
- MAIN `f_AnimalFollow`,12581: follow goal50source pixels behind the owner and at ownerY; approach by one tenth of the remaining distance. Half-scale goal is25DS pixels. The module retains signed fractional sixteenths for deterministic gradual following rather than resetting the pet to the owner each frame.
- MAIN `f_AttachAnimal`,12707: detach the owner's old pet before attachment. An `unlock` pickup shares discovery with alive players; ownership/discovery is distinct from physical attachment.
- MAIN `f_UnAttachAnimal`,12745: old pet becomes lost/state150 with ignore_timer51 and love_timer0.
- MAIN `f_AnimalLost`,13020–13058: state150 ignores pickup while its timer is positive, then checks strict source50horizontal/20depth range; pickup requires an unlocked animal or an unlock discovery. It sets love_timer60 after attachment. DS bounds are strict25by10. The old pet occupies the consumed pickup slot on a swap, so capacity cannot silently destroy a pet.
- LEVEL16 `f_InitAnimals`, `assets/level16-scripts/scripts/frame_1/DoAction.as:196`: Ark availability depends on participating profiles' animal unlocks; already-carried animals are suppressed. This is not an all-pets cheat. The helper rejects a non-unlock ground/Ark animal if the collecting player has not unlocked it.
- LEVEL2 sprite320, depth599, marks animal24 at original(-189.9,-141.55), approximatelyDS(155,286). It registers a shovel spot. `PLAYER DefineSprite_1102/frame_39/DoAction.as` spawns the animal only on successful digging and marks its unlock=true. The helper requires both a shovel and a completed dig; it never grants Buddy simply for entering the room. The caller must mark the spot dug only after successful placement and avoid repeated completion events.
- `PLAYER DefineSprite_1102/frame_7/DoAction.as` dispatches pet24 to help dig; frame34 skips to the finishing sequence for pet24. This proves its faster-dig ability. The data helper reports the ability; the real dig animation is not implemented here.

## State and integration contract

`PetProfile` stores a32-bit source-ID ownership bitmask plus one equipped ID. Serialize with the canonical five-byte little-endian helpers, not a native struct dump. Decode rejects unsupported bits, unknown IDs, and equipped-but-unowned state without modifying the destination.

`PetFollower` is8bytes and `PetDrop` is8bytes. Pickup is transactional. One actor receives the physical pet; living participants receive only ownership on first discovery. A later attempt to collect the consumed slot cannot duplicate that pet. Changing/removing equipment separately requires an empty drop slot for the old pet; failure changes nothing. The campaign must supply an existing world pickup slot rather than allocating an unbounded secondary item pool.

Follower heart timing uses the source60-tick love timer. A renderer can use it for hearts; no specific imported happy/pop animation has been wired. Dropped hover height rounds original35source pixels to18DS pixels. Deterministic fixed-point follow and that half-pixel rounding are DS adaptations. Ground pet wandering/angry/launch animation, passive attack abilities for other animals and full Ark selection are not implemented by this module.

Tests cover no default ownership, shovel/dig requirements, original pickup bounds, shared living-player discovery, one physical owner, locked Ark pickup, atomic full-slot failures,51-tick no-reclaim interval, exact save bytes, invalid-save rejection, source stat bonuses and5000 deterministic follower steps. No campaign, renderer, main, arena, save or packet files were edited for this module.
