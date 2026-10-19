# Forest Entrance (LEVEL18) source audit

Identification: the original MAP links node7 (Barbarian Boss) -> node101 -> node18 -> node4. LEVEL18 `f_LeaveLevel2` marks 18 complete and unlocks level 4 (Thieves' Forest); a community level guide lists Forest Entrance as the fifth level with a croc-riding thief, a barbarian ambush with a beefy thief, and ends by walking into the bushes. LEVEL9 (visited marker, no combat) is a separate side area and is not part of this build.

Coordinates: DS world = (source - origin) / 2 with origin (-256,-736), 2304x448. BSP18 corridor: x -149..4212, y -364..-31. Waypoints WP0..WP8 = (183,234) (362,232) (628,238) (1011,239) (1050,247) (1373,296) (1789,293) (2147,298) (2192,288). Entry marks p1_1 (176,243) p2_1 (156,258); cutscene marks p1_4 (570,225) p2_4 (555,242); exit door2 (2140,293); edges 50 and 2214.

Flow (f_Main / sprite234):
- WP1: leash WP0..WP2; croc thief at (778,232), thieves at (678,207) (728,277); two players add (878,257) (903,207). Clearing the wave fires `fp_SpecialEvent = f_Cinema`.
- Cutscene (165 ticks): heroes walk to p*_4, King (sprite222) starts at frame 30, litter (sprite211) at 3, princess thieves (sprite187) at 87; beefy thief (200 HP) spawns at King x-100; at frame 75 the barbarians in ambush1..5 (x 642,890,550,816,733; tree y 91,121,125,97,84) drop to ambush_y 194 after wait 10,30,50,70,80 plus 36 ticks. Release at 163. Two players add two thieves near WP3.
- WP5: eleven source thieves in the DS build as six plus four archers; leash WP4..WP6.
- WP6: two thieves at WP7, thieves at WP8 after 0/60/120/180 ticks; two players add three more.
- Exit: walk into the bushes at door2; completion unlocks Thieves' Forest.

Decompiler note: the plain decompiler drops the NOT performed by custom opcode 0x70; the raw p-code was used for thresholds.

DS adaptations: archers use the shared ranged enemy rule; the croc mount is not yet drawn (the croc thief appears as a normal thief); the parallax layers (sprites 19, 23, 248) are baked as a tiled half-scale backdrop under a dusk gradient (no sky pak exists for sky type 10); ambush barbarians start at most 80 units above the road (wire limit); six enemy slots, so larger waves feed in as slots free.
