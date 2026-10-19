# Sprint attacks: source and DS implementation

Original locally installed PLAYER root47 `dashpunch` uses sprite737. Root46 `dashslam` uses736. MAIN `f_PunchSet1` / `f_PunchSet2` select group4/1 or4/2 only for grounded dashing with true speed and combo slot2; the audited unlock is level4. Arena uses its existing unrestricted level99 rules, campaign uses the character's level.

- Sprint+A: dashpunch rolling strike. Source speed15 decays1 per tick (`f_DashSlam`); DS halfscale rounds that decaying motion to integer steps. Sprite737's first hit marker exists at frame2, widens at5, and punching clears at7. Active zero-based animation1..5;12-tick recovery. Damage is the existing low melee tier; source recoil-8 becomes upward4.
- Sprint+X: dashslam dive. Source speed18 becomes9DS/tick; `f_DashDive` starts at frame2, uses10 float ticks, and the source ends at12. DS starts at height1 with downward velocity2, floats10ticks, then lands naturally. Low melee damage; source upward26 becomes13DS. The protocol's valid launch limit extends to13, while its existing movement integrator still bounds height to80.
- Both strikes face forward, use original15DS floor-lane tolerance, respect wall sweeps, and can be blocked by a shield facing the attacker. They do not grant invulnerability. Dash reset requires building sprint momentum again afterward.
- Holding magic preserves the existing spell dispatch. Jump attacks and the level2 double-jump spin remain unchanged. A sprint strike has its own pose, distinct from the protected recovery roll after knockdown.

Hitboxes were recovered from original marker18 and body/root transforms, rounded outward in half-pixel collision units:

| Pose/frame | left | top | right | bottom |
|---|---:|---:|---:|---:|
| Dashpunch before source5 | -47 | -87 | 50 | 0 |
| Dashpunch from source5 | -45 | -84 | 134 | 4 |
| Dashslam | -21 | -103 | 114 | 1 |

The implementation adds poses30/31 and reuses HIT_LIGHT. Fighter size, campaign packet276 bytes, and input packet36 bytes remain unchanged. Arena protocol22 prevents mixing old pose tables. Campaign version36 was already unreleased.

Tests: sprint_attack_test covers real sprint activation, momentum, default attacks before sprint, level4 campaign gating, source attack windows, one hit per target action, front/rear shield behavior, depth avoidance, dashslam launch, stage boundaries and valid snapshots. Existing arena60,000-tick deterministic simulation, protocol60,000-snapshot simulation, and starter-combat tests passed. Three original artwork keys per pose are a DS compression adaptation; physics retains the full timeline.
