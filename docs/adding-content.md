# Adding content (v0.50 foundation)

## A new weapon (the template for everything else)
1. Pick the next registry slot (the index in `campaign_weapons.c`; slots are stable save and wire ids, at most 128).
2. Add the row to `weapons[]` (name, source id, level, price, critical divisor, strength, defense, magic, agility from `f_WeaponStats`).
3. Add the source id to the weapon lists in `prepare_held_weapons.py`, `import-held-weapons.ps1`, `convert_held_weapons.py` (weapon tuple), `convert_campaign_weapons.py` (`IDS`), `prepare_default_combos.py`, `export_default_combos.py`, `measure_default_combos.py`, `prepare_dash_attacks.py`, `export_dash_attacks.py`, `measure_dash_attacks.py`, and set `NW` in both factor scripts. Run the import script, the three measure/factor/encode chains, then `tools/build_weapon_packs.py`.
4. `build_weapon_packs.py` writes `nitrofiles/weapons/w<slot>.bin` for every weapon (rebuilt together because they share Huffman trees), `icons.bin`, and `generated/weapon_art_shared.h`. Nothing else in the ROM grows: a hero's pack loads when the weapon is equipped.
5. Place the pickup in the level (`campaign_weapon_place`) and run `check.ps1`: `weapon_art_test` verifies every held, combo and dash frame against the source-derived hashes.

Cost per weapon: about 70 to 90 KB on the SD card, nothing resident. Two heroes hold two packs (about 188 KB) however large the catalogue.

## Wire and save
- Snapshot size is fixed at 276 bytes. A rotating byte in the heroes' spare hp bits carries, over a 32-snapshot cycle: each hero's weapon id (every 4 snapshots), the eight level-clear bytes (64 levels) and each hero's equipped pet id. New per-player ids use the spare cycle slots (`ext_byte` in `campaign.c`).
- Save v5 (208 bytes): per knight 128-bit weapon set, 64-bit pet set, 64-bit level clears. Saves from v1 to v4 migrate on load.
- The guest's profile message is the 64-byte input reply (`CampaignRequest`), carrying the same 48-byte record.

## Still to do for the whole game
- Pets: art is still resident (about 6 KB each); move to per-pet packs like weapons when the catalogue grows past a handful. Wire and save already carry 64 pet ids.
- Mounts, enemies and magic effects: add the same pack pattern per level bank (the crocodile is the pilot; it is resident for now).
- World map: node unlock rules are still code for six nodes; make them a table before the second region.
