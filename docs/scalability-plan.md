# Scaling the port to the whole game

Measured on v0.49 (all sizes bytes).

## What already scales
- Levels: each chapter is one NitroFS bank (215 KB to 853 KB) loaded into one shared 852 KB buffer. Cost per level is SD space only (about 0.4 MB average, so roughly 16 MB for 40 levels).
- Chapter music: loaded per level into the bank tail. Menu and arena music now do the same.

## What does not scale (and by how much)
| Content | Today | Whole game | Limit hit |
|---|---|---|---|
| Held weapon art | 248 KB patches for 6 weapons (about 40-60 KB each, with dash and combo art) | about 90 weapons: 4-5 MB | Resident ROM data. Must stream. |
| Weapon id on the wire | 3 bits (8 slots), rides in hero hp spare bits | 90 ids need 7 bits | Packet is fixed at 276 bytes. |
| Weapons owned | 6-bit mask | 90-bit set | Save slot and wire. |
| Pets | 19 KB resident for 3 pets (about 6 KB each); 2-bit code, 3 pets | about 60 pets: 380 KB | Resident data and 2-bit code. |
| Mounts | 26 KB resident for one croc | 4 mount types | Fine if streamed per level. |
| Levels cleared | 5-bit field; 13 maps | about 40 levels | Clear bits in the packet. |
| Enemy kinds | 2-bit knight id; thief, barbarian, beefy | dozens of enemy types with own art | Resident per-enemy art, 2-bit id. |
| Magic and effects | 4 resident magic clips, 105 KB effect art | per-element sprites and sounds | Resident data. |
| Memory margin | 361 KB beyond the multiplayer reserves | | Fine for one more tier of content, not for all of it. |

## Plan
1. Registry: one table per content type (weapon, pet, mount, item, enemy, level) with a stable numeric id, stats and asset file names. Replace the hard-coded enums and switches.
2. Streamed art: per-weapon and per-pet files loaded into fixed slots when equipped (two heroes means two weapon slots and two pet slots, so cost does not grow with the catalogue). Pilot with the six existing weapons.
3. Wire v2: send content ids as a loadout message on change over the existing request channel, keeping the 276-byte snapshot. Snapshots carry only slot numbers.
4. Save v5: bitsets sized for the whole catalogue, 64-bit level clears, ids widened to 8 or 16 bits. Old saves migrate.
5. One pipeline manifest (JSON) that drives the held-weapon, combo, dash, icon and pet converters for any list of ids, as the weapon list is already a parameter in those scripts.
6. Per-level enemy art banks, loaded with the level, replacing the two-bit enemy id.

Steps 1 to 3 change the wireless protocol, so they ship together and break compatibility with older builds once.

## Status after v0.50
Done: per-weapon art packs streamed on equip (held, heavy combo, dash, icon), 128-weapon / 64-pet / 64-level sets in the save (v5) and in the guest profile message, rotating weapon, pet and level-clear state on the 276-byte snapshot, and the content guide in adding-content.md.
Open: pets, mounts, enemies and effects still resident; world map node rules are code.
