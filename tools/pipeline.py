"""Ordered asset conversion pipeline (used by tools/setup_assets.py).

Each step is a tuple:
  ('decrypt', 'data/levels/level49.pak')        decrypt a game archive into assets/extracted
  ('ffdec', arg, arg, ...)                      run JPEXS FFDec with these arguments
  ('py', 'tools/script.py', arg, ...)           run a converter with the current Python
  ('each', [values], step-with-{v})             repeat a step for each value
Arguments may contain {game} (the Castle Crashers folder).
"""
STEPS = []

def S(*a):
    STEPS.append(a)

def scene(name, *opts):
    """FFDec: rebuild assets/extracted/NAME.xml into a SWF and export its frames to assets/NAME."""
    S('ffdec', '-xml2swf', f'assets/extracted/{name}.xml', f'assets/extracted/{name}.swf')
    S('ffdec', '-ignorebackground', *opts, '-export', 'frame', f'assets/{name}', f'assets/extracted/{name}.swf')

# ---- Arena (PvP) ---------------------------------------------------------
S('py', 'tools/restore_frozen.py')
S('py', 'tools/extract_game_files.py', '{game}', '{ffdec}')
S('py', 'tools/prepare_frames.py')
for c in ('blue', 'orange', 'red', 'green'):
    S('ffdec', '-xml2swf', f'assets/extracted/{c}.xml', f'assets/extracted/{c}.swf')
    S('ffdec', '-ignorebackground', '-export', 'frame', f'assets/{c}', f'assets/extracted/{c}.swf')
S('py', 'tools/prepare_background.py')
S('ffdec', '-xml2swf', 'assets/extracted/arena.xml', 'assets/extracted/arena.swf')
S('ffdec', '-select', '1', '-export', 'frame', 'assets/arena', 'assets/extracted/arena.swf')
S('ffdec', '-xml2swf', 'assets/extracted/arena-props.xml', 'assets/extracted/arena-props.swf')
S('ffdec', '-ignorebackground', '-export', 'frame', 'assets/arena-props', 'assets/extracted/arena-props.swf')
S('ffdec', '-xml2swf', 'assets/extracted/arena-foreground.xml', 'assets/extracted/arena-foreground.swf')
S('ffdec', '-ignorebackground', '-select', '1', '-export', 'frame', 'assets/arena-foreground', 'assets/extracted/arena-foreground.swf')
S('py', 'tools/convert_assets.py')
S('py', 'tools/convert_items.py', '--prepare')
S('ffdec', '-xml2swf', 'assets/extracted/items.xml', 'assets/extracted/items.swf')
S('ffdec', '-ignorebackground', '-export', 'frame', 'assets/items', 'assets/extracted/items.swf')
S('py', 'tools/convert_items.py')
# banner needs the HUD portraits: see HUD section
S('py', 'tools/convert_background.py')
for name, num in (('king', 46), ('volcano', 47)):
    S('py', f'tools/prepare_{name}_background.py')
    for suffix in ('', '-foreground', '-props'):
        part = f'arena-{name}{suffix}'
        S('ffdec', '-xml2swf', f'assets/extracted/{part}.xml', f'assets/extracted/{part}.swf')
        S('ffdec', '-ignorebackground', '-export', 'frame', f'assets/{part}', f'assets/extracted/{part}.swf')
    S('py', f'tools/convert_{name}_background.py')
S('py', 'tools/export_arena_collision.py', '{game}/data/bsps/bsp49b.pak')

# ---- Menu, HUD, icon -----------------------------------------------------
S('ffdec', '-select', '1', '-export', 'frame', 'assets/menu-vignettes', 'assets/extracted/MENU_VIGNETTES.swf')
S('py', 'tools/convert_menu.py', '--game-directory', '{game}')
S('ffdec', '-selectid', '182,192', '-export', 'sprite', 'assets/hud-parts', 'assets/extracted/MAIN.swf')
S('ffdec', '-selectid', '167', '-export', 'sprite', 'assets/portraits', 'assets/extracted/MAIN.swf')
S('ffdec', '-format', 'shape:png', '-selectid', '169', '-export', 'shape', 'assets/hud-shapes', 'assets/extracted/MAIN.swf')
S('py', 'tools/convert_hud.py')
S('py', 'tools/convert_banner.py')

# ---- Audio ---------------------------------------------------------------
S('py', 'tools/convert_audio.py', '{game}', '--ffmpeg', '{ffmpeg}')

# ---- Effects -------------------------------------------------------------
S('py', 'tools/convert_effects.py', '--prepare')
scene('effects')
S('py', 'tools/convert_effects.py')

# ---- Loot ----------------------------------------------------------------
S('py', 'tools/prepare_potion.py')
scene('potion')
S('py', 'tools/prepare_food_variants.py')
scene('food-variants')
scene('shop-potion')
S('py', 'tools/convert_loot.py', '--prepare')
scene('loot')
S('py', 'tools/convert_loot.py')

# ---- Castle Keep (LEVEL13), barracks (LEVEL2) ------------------------------
S('py', 'tools/prepare_campaign_background.py')
scene('campaign')
S('ffdec', '-export', 'image', 'assets/campaign-sky-images', 'assets/extracted/SKY1.swf')
S('py', 'tools/convert_campaign_background.py', '{game}/data/bsps/bsp13.pak')
S('py', 'tools/prepare_campaign_world.py')
scene('campaign-world')
scene('campaign-world-fg')
S('py', 'tools/convert_campaign_world.py')
S('py', 'tools/export_campaign_world_geometry.py')
S('py', 'tools/prepare_campaign_breakables.py')
scene('campaign-breakables')
scene('campaign-hay')
S('py', 'tools/convert_campaign_breakables.py')
S('py', 'tools/pack_arena_occlusion.py')
S('py', 'tools/pack_arena_terrain.py')
S('py', 'tools/prepare_barracks_actors.py')
S('py', 'tools/prepare_barracks_furniture.py')
S('py', 'tools/export_barracks_actors.py')
S('py', 'tools/audit_barracks_sources.py')
S('py', 'tools/convert_barracks_actors.py')
S('py', 'tools/prepare_campaign_barracks.py')
scene('campaign-barracks')
S('py', 'tools/convert_campaign_barracks.py')
S('py', 'tools/convert_barracks_choreography.py', '--prepare')
scene('barracks-exit')
S('py', 'tools/convert_barracks_choreography.py')
S('py', 'tools/prepare_barracks_door.py')
scene('barracks-door-closed')
scene('barracks-door-open')
S('py', 'tools/convert_barracks_door.py')
S('py', 'tools/prepare_barracks_doorframe.py')
scene('door-frame-audit')
S('py', 'tools/convert_barracks_doorframe.py')
S('py', 'tools/prepare_barracks_rail.py')
scene('barracks-rail')
S('py', 'tools/convert_barracks_rail.py')
S('py', 'tools/convert_barracks_nearwall.py')

# ---- Shop (LEVEL19) ----------------------------------------------------------
S('py', 'tools/prepare_campaign_shop.py')
scene('campaign-shop')
S('py', 'tools/convert_campaign_shop.py')
S('py', 'tools/prepare_shop_boards.py')
scene('shop-boards')
S('py', 'tools/convert_shop_boards.py')

# ---- Keep cinematics and townsfolk --------------------------------------------
S('ffdec', '-export', 'script', 'assets/level13-scripts', 'assets/extracted/LEVEL13-full.swf')  # read by tests/cinematic_occlusion_test.py
S('py', 'tools/convert_cinematics.py', '--prepare')
S('py', 'tools/export_cinematics.py')
S('py', 'tools/convert_cinematics.py')
S('py', 'tools/convert_neutral_npc.py', '--prepare')
scene('neutral-npc')
S('py', 'tools/convert_neutral_npc.py')
S('py', 'tools/convert_gray_npc.py', '--prepare')
scene('gray-npc')
S('py', 'tools/convert_gray_npc.py')
S('py', 'tools/convert_pickup_feedback.py', '--prepare')
scene('onward')
scene('pickup')
S('py', 'tools/convert_pickup_feedback.py')

# ---- Barbarian and thief variants --------------------------------------------
S('py', 'tools/convert_barbarian.py', '--prepare')
scene('barbarian')
S('py', 'tools/convert_barbarian.py')
S('py', 'tools/retain_barbarian_bow.py')
S('py', 'tools/pack_barbarian_art.py')
S('py', 'tools/convert_thief.py', '--prepare')
scene('thief')
S('py', 'tools/convert_thief.py')

# ---- Weapons: held poses, dash attacks, default combos, icons ------------------
S('py', 'tools/prepare_held_weapons.py')
for c in ('blue', 'orange', 'red', 'green'):
    for w in (2, 9, 19, 10, 38, 30, 37, 42):
        scene(f'held-{c}-{w}')
S('py', 'tools/convert_held_weapons.py')
S('py', 'tools/prepare_dash_attacks.py')
S('py', 'tools/export_dash_attacks.py')
S('py', 'tools/measure_dash_attacks.py')
S('py', 'tools/factor_dash_attacks.py')
S('py', 'tools/encode_dash_attacks.py')
S('py', 'tools/prepare_default_combos.py')
S('py', 'tools/export_default_combos.py')
S('py', 'tools/build_default_combos.py')
S('py', 'tools/convert_campaign_weapons.py', '--prepare')
scene('campaign-weapons')
S('py', 'tools/build_weapon_packs.py')

# ---- War (LEVEL11), bridge (LEVEL12) ----------------------------------------
S('py', 'tools/prepare_campaign_war.py')
S('py', 'tools/export_campaign_war.py')
S('py', 'tools/convert_campaign_war.py')
S('py', 'tools/convert_war_geometry.py')
S('py', 'tools/prepare_war_alternates.py')
scene('war-alternates')
S('py', 'tools/prepare_war_gate_chip.py')
scene('war-gate-chip')
S('py', 'tools/convert_war_alternates.py')
S('py', 'tools/convert_war_choreography.py')
S('py', 'tools/prepare_war_live_actors.py')
S('py', 'tools/export_war_live_actors.py')
S('py', 'tools/convert_war_live_actors.py')
S('py', 'tools/prepare_war_machine.py')
S('py', 'tools/export_war_machine.py')
S('py', 'tools/convert_war_machine.py')
S('py', 'tools/prepare_war_cannon.py')
S('py', 'tools/convert_war_cannon.py')

# ---- Hubs: blacksmith (LEVEL14), frog (LEVEL15), ark (LEVEL16), world map ----
S('py', 'tools/prepare_campaign_war.py', '--hubs')
S('py', 'tools/export_campaign_war.py', '--hubs')
S('py', 'tools/convert_campaign_war.py', '--hubs')
S('py', 'tools/convert_hub_geometry.py')
S('py', 'tools/prepare_hub_ambient.py')
S('py', 'tools/export_hub_ambient.py')
S('py', 'tools/convert_hub_ambient.py')
S('py', 'tools/prepare_original_worldmap.py')
S('py', 'tools/export_original_worldmap.py')
S('py', 'tools/convert_original_worldmap.py')

# ---- Barbarian boss (LEVEL7) -------------------------------------------------
S('py', 'tools/prepare_campaign_boss.py')
S('py', 'tools/export_campaign_boss.py')
S('py', 'tools/convert_campaign_boss.py')
S('py', 'tools/convert_boss_geometry.py')
S('py', 'tools/prepare_boss_actor.py')
S('py', 'tools/export_boss_actor.py')
S('py', 'tools/prepare_boss_props.py')
S('py', 'tools/export_boss_props.py')
S('py', 'tools/prepare_boss_knight_kiss.py')
S('py', 'tools/export_boss_knight_kiss.py')
S('py', 'tools/convert_boss_actor.py')
S('py', 'tools/boss_door.py', 'prepare')
S('py', 'tools/boss_door.py', 'convert')

# ---- Forest Entrance (LEVEL18) ----------------------------------------------
S('py', 'tools/prepare_campaign_war.py', 'forest')
scene('campaign-forest')
scene('campaign-forest-fg')
S('ffdec', '-selectid', '19,23', '-export', 'sprite', 'build/l18bg', 'assets/extracted/LEVEL18-full.swf')
S('py', 'tools/convert_campaign_war.py', '--forest')
S('py', 'tools/prepare_forest_props.py')
for n in ('king', 'litter', 'thieves'):
    scene(f'forest-prop-{n}')
S('py', 'tools/convert_forest_props.py')
S('py', 'tools/prepare_forest_croc.py')
scene('forest-croc')
S('py', 'tools/convert_forest_croc.py')
S('py', 'tools/rider_art.py', 'prepare')
S('py', 'tools/rider_art.py', 'convert')
S('py', 'tools/make_forest_thief.py', 'forest')

# ---- Thieves' Forest (LEVEL4) -------------------------------------------------
S('py', 'tools/thieves_shear.py')
S('py', 'tools/prepare_campaign_war.py', 'thieves')
scene('campaign-thieves', '-zoom', '0.5')
scene('campaign-thieves-fg', '-zoom', '0.5')
S('py', 'tools/convert_campaign_war.py', '--thieves')
S('py', 'tools/convert_thieves_geometry.py')
S('py', 'tools/thieves_scenes.py', 'prepare')
S('py', 'tools/thieves_scenes.py', 'convert')
for n in ('troll', 'mother', 'imp'):
    S('py', 'tools/species_art.py', 'prepare', n)
    scene(f'species-{n}')
    S('py', 'tools/species_art.py', 'convert', n)
S('py', 'tools/make_forest_thief.py', 'thieves')
S('py', 'tools/break_points.py')

# ---- Mill run (LEVEL22) -------------------------------------------------------
scene('campaign-deer', '-zoom', '0.5')
S('ffdec', '-zoom', '0.5', '-selectid', '88', '-export', 'sprite', 'assets/l22m', 'assets/extracted/LEVEL22-full.swf')
S('py', 'tools/deer_scene.py')
S('py', 'tools/convert_campaign_war.py', '--deer')
S('py', 'tools/deer_art.py', 'prepare')
S('py', 'tools/deer_mount.py', 'prepare')
S('py', 'tools/deer_extra.py')
S('py', 'tools/deer_arms.py')
S('py', 'tools/deer_actors.py')
S('py', 'tools/deer_geometry.py')

# ---- Flooded castle (LEVEL23) -------------------------------------------------
scene('campaign-flood', '-zoom', '0.5')
scene('campaign-flood-fg', '-zoom', '0.5')
S('py', 'tools/convert_campaign_war.py', '--flood')
S('py', 'tools/flood_extra.py')
S('py', 'tools/flood_actors.py')
S('py', 'tools/flood_geometry.py')

# ---- Pets, digging, buried knights --------------------------------------------
S('py', 'tools/export_pet_dig.py')
S('py', 'tools/export_pets_extra.py')
S('py', 'tools/convert_pet_dig.py')
S('py', 'tools/export_buried.py')
S('py', 'tools/convert_buried.py')

# ---- Party dance, magic, interface ----------------------------------------------
S('py', 'tools/dance_art.py', 'prepare')
S('py', 'tools/dance_art.py', 'convert')
S('py', 'tools/magic_fx.py', 'prepare')
S('py', 'tools/magic_fx.py', 'convert')
S('ffdec', '-zoom', '0.5', '-selectid', '63', '-export', 'sprite', 'assets/tally', 'assets/extracted/MAIN.swf')
S('py', 'tools/tally_art.py')
S('ffdec', '-selectid', '312', '-export', 'sprite', 'assets/boss-meter', 'assets/extracted/MAIN.swf')
S('py', 'tools/convert_boss_meter.py')
S('py', 'tools/prepare_progression_art.py')
S('py', 'tools/export_progression_art.py')
S('py', 'tools/convert_progression_art.py')
S('py', 'tools/prepare_ui_icons.py')
S('py', 'tools/export_ui_icons.py')
S('py', 'tools/convert_ui_icons.py')
S('py', 'tools/export_item_selector.py')
S('py', 'tools/export_item_popups.py')
S('py', 'tools/convert_selector_art.py')
S('ffdec', '-format', 'sprite:png', '-export', 'sprite', 'build/lobby/LOBBY', 'assets/extracted/LOBBY.swf')
S('py', 'tools/convert_lobby_art.py')
S('py', 'tools/prepare_run_dust.py')
scene('run-dust')
S('py', 'tools/convert_run_dust.py')

# ---- Sound cues and chapter music ----------------------------------------------
S('py', 'tools/make_resident_music.py')
S('py', 'tools/convert_barracks_audio.py', '{game}', '--ffmpeg', '{ffmpeg}')
S('py', 'tools/convert_cinematic_audio.py', '{game}', '--ffmpeg', '{ffmpeg}')
S('py', 'tools/convert_pickup_audio.py')
S('py', 'tools/convert_menu_sfx.py')
S('py', 'tools/convert_break_sfx.py')
S('py', 'tools/convert_deer_sfx.py')
S('py', 'tools/convert_thieves_sfx.py')
S('py', 'tools/convert_chapter_audio.py')
S('py', 'tools/convert_audio_envelopes.py')

# ---- Chapter banks (last: wraps the per-chapter arrays) ---------------------------
S('py', 'tools/pack_chapter_assets.py')
