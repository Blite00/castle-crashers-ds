# Thieves' Forest (LEVEL4) source audit

Identification: LEVEL18 `f_LeaveLevel2` unlocks level 4; LEVEL4 (`level4.pak`, `bsp4.pak`) is the long forest walk ending at a cabin. Music `NG57239` for the level and `Waterflame_TheShow` for the Troll Mother.

Coordinates: world x = source x / 2; world y = (source y + 1350) / 2 minus a per-column shear (tools/thieves_shear.py). The corridor climbs about 500 source pixels over 13,000, which cannot fit the DS world's 448 rows together with the canopy, so every column is shifted vertically; art, depth layers, floor bounds (tools/convert_thieves_geometry.py) and all markers use the same table. World width 6,688. Waypoints WP0..WP26 are the float table after the BSP edges.

Encounters (f_Main, waypoint hit -> leash -> spawns -> release when cleared):
- WP1 the thief hiding in the bush. WP5 one hanging thief plus four walkers. WP6 four walkers plus three vine thieves (waits 0/20/80). WP9 six vine thieves (waits 0,20,50,80,120,140).
- WP11..WP14 one troll each from trollspawn1..4 (they drop in from the trees).
- WP19 the imp (speed 8). WP20 the three washing thieves (sprite 178 at thiefwash). WP22 three panicked thieves running left.
- WP23 the Troll Mother (350 health) at WP25: tosses a troll every 60 ticks, charges 20 ticks at speed 14 each 20 damage, takes her brood with her on death; two panicked thieves; boss music.
- Exit: the cabin door (door2), completing level 4 and setting the Thieves' Forest clear bit (bit 5).

Level items: six coin bags (5 gold), weapon 30 at weapon_pt, animals 9 (Troll) at trollpet and 21 at animal_pt, both unlocking for the party.

Not played: the intro scream, the owl, bear, deer and "poop" cutscenes, the washing-thief and campfire ambience, and the mounted vine "hang" poses; waypoints 2, 7 and 17 are cutscene-only in the source. Imps do not steal the coin bag.
