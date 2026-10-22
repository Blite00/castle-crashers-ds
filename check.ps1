$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
$savedPath = $env:PATH
try {
    New-Item -ItemType Directory -Force build | Out-Null
    $cc = Join-Path $PSScriptRoot '../.tools/msys64/ucrt64/bin/gcc.exe'
    $env:PATH = (Split-Path $cc) + ';' + $env:PATH
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/bottom_ui_test.c source/campaign_save.c source/bottom_ui.c source/item_selector.c generated/selector_art.c source/ui_skin.c source/campaign_menu.c source/hud.c source/weapon_art.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c source/campaign.c source/campaign_boss.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/bottom-ui-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bottom UI compilation failed' }
    & ./build/bottom-ui-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bottom UI checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/selector_art_test.c generated/selector_art.c -o build/selector-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Selector artwork compilation failed' }
    & ./build/selector-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Selector artwork checks failed' }
    foreach ($assetTest in @('boss_art','boss_actor_art','hub_ambient','progression_art','ui_icon_art','buried_art')) {
        & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated "tests/$($assetTest)_test.c" -o "build/$assetTest-test.exe"
        if ($LASTEXITCODE -ne 0) { throw "$assetTest compilation failed" }
        & "./build/$assetTest-test.exe"
        if ($LASTEXITCODE -ne 0) { throw "$assetTest checks failed" }
    }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_boss_test.c source/campaign.c source/campaign_boss.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_boss-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Boss compilation failed' }
    & ./build/campaign_boss-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Boss checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/hub_art_test.c -o build/hub_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "hub_art compilation failed" }
    & ./build/hub_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "hub_art checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/thief_art_test.c -o build/thief_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "thief_art compilation failed" }
    & ./build/thief_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "thief_art checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/war_live_art_test.c -o build/war_live_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_live_art compilation failed" }
    & ./build/war_live_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_live_art checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/war_alternate_test.c -o build/war_alternate-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_alternate compilation failed" }
    & ./build/war_alternate-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_alternate checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/war_art_test.c -o build/war_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_art compilation failed" }
    & ./build/war_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_art checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/war_machine_art_test.c -o build/war_machine_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_machine_art compilation failed" }
    & ./build/war_machine_art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_machine_art checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/chapter_audio_test.c source/chapter_assets.c -o build/chapter-audio-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Chapter audio compilation failed" }
    & ./build/chapter-audio-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Chapter audio checks failed" }
    python tests/chapter_audio_test.py
    if ($LASTEXITCODE -ne 0) { throw "Chapter audio samples failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/war_cannon_art_test.c -o build/war-cannon-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Cannon art compilation failed' }
    & ./build/war-cannon-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Cannon art checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated -Isource tests/war_geometry_test.c -o build/war-geometry-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'War geometry compilation failed' }
    & ./build/war-geometry-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'War geometry checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource tests/war_gray_test.c source/campaign_combat.c source/campaign_weapons.c source/arena.c source/stage.c -o build/war-gray-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Gray ally compilation failed' }
    & ./build/war-gray-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Gray ally checks failed' }
    python tools/test_chapter_assets.py
    if ($LASTEXITCODE -ne 0) { throw 'Chapter bank validation failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/pets_test.c source/pets.c -o build/pets-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Pet state compilation failed' }
    & ./build/pets-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Pet state checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barbarian_packed_test.c -o build/barbarian-packed-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Enemy compression compilation failed' }
    & ./build/barbarian-packed-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Enemy compression checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barracks_rail_test.c -o build/barracks-rail-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Handrail compilation failed' }
    & ./build/barracks-rail-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Handrail checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barracks_actor_test.c -o build/barracks-actor-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Barracks actor compilation failed' }
    & ./build/barracks-actor-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Barracks actor checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barbarian_bow_test.c -o build/barbarian-bow-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Enemy bow compilation failed' }
    & ./build/barbarian-bow-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Enemy bow checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/frame_clock_test.c -o build/frame-clock-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Frame clock compilation failed' }
    & ./build/frame-clock-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Frame clock checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/raster_foreground_test.c source/raster.c -o build/raster-foreground-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Foreground test compilation failed' }
    & ./build/raster-foreground-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Foreground composition test failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barracks_nearwall_test.c -o build/barracks-nearwall-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Nearwall test compilation failed' }
    & ./build/barracks-nearwall-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Nearwall source test failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/run_dust_test.c -o build/run-dust-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Run dust compilation failed' }
    & ./build/run-dust-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Run dust checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/overlay_copy_test.c -o build/overlay-copy-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Overlay batching compilation failed' }
    & ./build/overlay-copy-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Overlay batching checks failed' }
    python tools/test_menu_alpha.py
    if ($LASTEXITCODE -ne 0) { throw 'Menu alpha and doorway checks failed' }
    python tools/test_terrain_rows.py
    if ($LASTEXITCODE -ne 0) { throw 'Partial terrain codec failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/audio_timeline_test.c source/audio_timeline.c -o build/audio-timeline-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Audio timeline compilation failed' }
    & ./build/audio-timeline-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Audio timeline failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -DARM9 -Itests/audio_stubs -Isource -Igenerated '-DAUDIO_BANK_PREFIX="nitrofiles/audio/"' tests/audio_backend_test.c source/audio.c source/audio_timeline.c -o build/audio-backend-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Audio backend compilation failed' }
    & ./build/audio-backend-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Audio bank lifetime tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/barracks_door_test.c -o build/barracks-door-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Door compilation failed' }
    & ./build/barracks-door-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Door animation tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/pet_save_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/pet-save-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Pet save compilation failed" }
    & ./build/pet-save-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Pet save checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_pets_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_pets-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_pets compilation failed" }
    & ./build/campaign_pets-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_pets checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/war_physics_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/war_physics-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_physics compilation failed" }
    & ./build/war_physics-test.exe
    if ($LASTEXITCODE -ne 0) { throw "war_physics checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/campaign_war_test.c source/campaign_war.c -o build/campaign_war-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_war compilation failed" }
    & ./build/campaign_war-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_war checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/pet_art_test.c -o build/pet-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Pet art compilation failed" }
    & ./build/pet-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Pet art checks failed" }
    python tests/pet_art_source_test.py
    if ($LASTEXITCODE -ne 0) { throw "Pet source art checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/campaign_hubs_test.c source/campaign_hubs.c source/pets.c source/campaign_weapons.c -o build/campaign_hubs-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_hubs compilation failed" }
    & ./build/campaign_hubs-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_hubs checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_world_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_world-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_world compilation failed" }
    & ./build/campaign_world-test.exe
    if ($LASTEXITCODE -ne 0) { throw "campaign_world checks failed" }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_forest_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_forest-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Forest Entrance compilation failed' }
    & ./build/campaign_forest-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Forest Entrance checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_thieves_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/weapon_art.c source/stage.c -o build/campaign_thieves-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Thieves Forest compilation failed' }
    & ./build/campaign_thieves-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Thieves Forest checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/wire_ext_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/wire-ext-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Wire state compilation failed' }
    & ./build/wire-ext-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Wire state checks failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_bow_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_bow-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bow compilation failed' }
    & ./build/campaign_bow-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bow checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_loading_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign-loading-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Loading handshake compilation failed" }
    & ./build/campaign-loading-test.exe
    if ($LASTEXITCODE -ne 0) { throw "Loading handshake checks failed" }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_progress_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign-progress-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Progress/save tests compilation failed' }
    & ./build/campaign-progress-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Progress/save tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_feedback_test.c source/campaign_feedback.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign-feedback-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Campaign feedback compilation failed' }
    & ./build/campaign-feedback-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Campaign feedback checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/campaign_weapons_test.c source/campaign_weapons.c source/arena.c source/stage.c -o build/campaign-weapons-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Weapon tests compilation failed' }
    & ./build/campaign-weapons-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Weapon tests failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Igenerated tests/forest_croc_art_test.c -o build/forest-croc-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Croc art compilation failed' }
    & ./build/forest-croc-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Croc art checks failed' }
    New-Item -ItemType Directory -Force build/weapon-test | Out-Null
    Copy-Item nitrofiles/weapons/* build/weapon-test/ -Force
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated '-DWEAPON_PACK_PREFIX=\"build/weapon-test/\"' tests/weapon_art_test.c source/weapon_art.c -o build/weapon-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Weapon art compilation failed' }
    & ./build/weapon-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Weapon art checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Itests/arm7_stubs tests/arm7_startup_test.c -o build/arm7-startup-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'ARM7 startup test compilation failed' }
    & ./build/arm7-startup-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'ARM7 startup tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_test.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_thieves.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Campaign test compilation failed' }
    & ./build/campaign-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Campaign tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/campaign_combat_test.c source/campaign_combat.c source/campaign_weapons.c source/bot.c source/arena.c source/stage.c -o build/campaign-combat-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Group combat compilation failed' }
    & ./build/campaign-combat-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Group combat tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/sprint_attack_test.c source/arena.c source/stage.c source/protocol.c -o build/sprint-attack-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Sprint attack compilation failed' }
    & ./build/sprint-attack-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Sprint attack checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/starter_combat_test.c source/arena.c source/stage.c -o build/starter-combat-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Starter combat compilation failed' }
    & ./build/starter-combat-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Starter combat checks failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/cinematic_test.c source/cinematic.c -o build/cinematic-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Cinematic test compilation failed' }
    & ./build/cinematic-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Cinematic tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/bot_test.c source/bot.c source/arena.c source/stage.c -o build/bot-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bot test compilation failed' }
    & ./build/bot-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Bot tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/arena_test.c source/arena.c source/stage.c -o build/arena-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
    & ./build/arena-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Arena tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/physics_test.c source/arena.c source/stage.c -o build/physics-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Physics test compilation failed' }
    & ./build/physics-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Physics tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/stage_test.c source/arena.c source/stage.c -o build/stage-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Stage test compilation failed' }
    & ./build/stage-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Stage tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/shop_stage_test.c source/arena.c source/stage.c -o build/shop-stage-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Shop traversal test compilation failed' }
    & ./build/shop-stage-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Shop traversal tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/campaign_shop_test.c -o build/campaign-shop-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Shop asset test compilation failed' }
    & ./build/campaign-shop-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Shop asset tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/camera_test.c source/camera.c source/arena.c source/stage.c -o build/camera-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Camera test compilation failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_deer_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_thieves.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_deer-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Deer level test compilation failed' }
    & ./build/campaign_deer-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Deer level tests failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_flood_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_thieves.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_flood-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Flood level test compilation failed' }
    & ./build/campaign_flood-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Flood level tests failed' }
    & $cc -std=c11 -O2 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_camp_test.c source/campaign_save.c source/campaign_link.c source/campaign.c source/campaign_war.c source/campaign_forest.c source/campaign_deer.c source/campaign_flood.c source/campaign_camp.c source/campaign_thieves.c source/campaign_boss.c source/campaign_hubs.c source/pets.c source/campaign_combat.c source/campaign_weapons.c source/cinematic.c source/bot.c source/protocol.c source/arena.c source/stage.c -o build/campaign_camp-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Camp level test compilation failed' }
    & ./build/campaign_camp-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Camp level tests failed' }
    & ./build/camera-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Camera tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/level_contract_test.c source/arena.c source/stage.c -o build/level-contract-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Level contract compilation failed' }
    & ./build/level-contract-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Level contract failed' }
    python tools/level_audit.py
    if ($LASTEXITCODE -ne 0) { throw 'Newest level is not registered everywhere' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/wireless_capacity_test.c -o build/wireless-capacity-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Wireless capacity check compilation failed' }
    & ./build/wireless-capacity-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Wireless capacity check failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource tests/protocol_test.c source/protocol.c source/arena.c source/stage.c -o build/protocol-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Protocol test compilation failed' }
    & ./build/protocol-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Protocol tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/effects_test.c generated/item_art.c source/effects.c source/raster.c source/arena.c source/stage.c -o build/effects-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Effects test compilation failed' }
    & ./build/effects-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Effects tests failed' }
    python tests/audio_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Audio tests failed' }
    python tests/audio_loudness_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Audio loudness tests failed' }
    python tools/test_audio_mix.py
    if ($LASTEXITCODE -ne 0) { throw 'Audio mix tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/hud_item_test.c source/hud.c source/weapon_art.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c -o build/hud-item-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Item HUD test compilation failed' }
    & ./build/hud-item-test.exe
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/world_shop_test.c -o build/world-shop-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'World shop test compilation failed' }
    & ./build/world-shop-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'World shop tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Igenerated tests/food_art_test.c generated/loot_art.c -o build/food-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Original food art test compilation failed' }
    & ./build/food-art-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Original food art tests failed' }
    if ($LASTEXITCODE -ne 0) { throw 'Item HUD tests failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tools/render_preview.c source/arena.c source/stage.c source/render.c source/weapon_art.c generated/selector_art.c source/cinematic.c source/camera.c source/effects.c source/raster.c source/hud.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c tests/thieves_stubs.c -o build/render-preview.exe
    if ($LASTEXITCODE -ne 0) { throw 'Renderer compilation failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -DRENDER_TEST_OVERLAY -Isource -Igenerated tools/render_preview.c source/arena.c source/stage.c source/render.c source/weapon_art.c generated/selector_art.c source/cinematic.c source/camera.c source/effects.c source/raster.c source/hud.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c tests/thieves_stubs.c -o build/render-overlay-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Hardware-overlay test compilation failed' }
    & $cc -std=c11 -Wall -Wextra -Werror -Isource -Igenerated tests/campaign_render_test.c source/arena.c source/stage.c source/render.c source/weapon_art.c generated/selector_art.c source/cinematic.c source/camera.c source/effects.c source/raster.c source/hud.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c tests/thieves_stubs.c -o build/campaign-render-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Continuous map renderer test compilation failed' }
    & ./build/campaign-render-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Continuous map renderer tests failed' }
    foreach ($variant in @('reference','overlay')) {
        $flags = @()
        if ($variant -eq 'overlay') { $flags += '-DRENDER_TEST_OVERLAY' }
        & $cc -std=c11 -Wall -Wextra -Werror @flags -Isource -Igenerated tests/barracks_present_test.c source/arena.c source/stage.c source/render.c source/weapon_art.c generated/selector_art.c source/cinematic.c source/camera.c source/effects.c source/raster.c source/hud.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c tests/thieves_stubs.c -o "build/barracks-$variant-test.exe"
        if ($LASTEXITCODE -ne 0) { throw 'Barracks presentation compilation failed' }
        & "./build/barracks-$variant-test.exe" "build/barracks-$variant.bin"
        if ($LASTEXITCODE -ne 0) { throw 'Barracks presentation test failed' }
    }
    if ((Get-FileHash build/barracks-reference.bin).Hash -ne (Get-FileHash build/barracks-overlay.bin).Hash) { throw 'Barracks presentation pixel mismatch' }
    python tests/cinematic_occlusion_test.py
    if ($LASTEXITCODE -ne 0) { throw 'Source princess doorway occlusion failed' }
    python tools/test_foreground_culling.py
    if ($LASTEXITCODE -ne 0) { throw 'Foreground culling pixel identity failed' }
    python tools/test_asset_codec.py
    if ($LASTEXITCODE -ne 0) { throw 'Asset codec tests failed' }
    foreach ($scenario in 0..89) {
        & ./build/render-preview.exe "build/reference-$scenario.ppm" $scenario
        if ($LASTEXITCODE -ne 0) { throw 'Reference render failed' }
        & ./build/render-overlay-test.exe "build/overlay-$scenario.ppm" $scenario
        if ($LASTEXITCODE -ne 0) { throw 'Overlay render failed' }
    }
    python -c "from pathlib import Path; r=Path('build'); assert all((r/f'reference-{i}.ppm').read_bytes()==(r/f'overlay-{i}.ppm').read_bytes() for i in range(88)); print('PASS: software reference and hardware-overlay composition match across all88 scenarios')"
    & $cc -std=c11 -Wall -Wextra -Werror -DRENDER_TEST_OVERLAY -DCHAPTER_ASSETS_TEST -Isource -Igenerated tools/render_preview.c source/chapter_assets.c source/arena.c source/stage.c source/render.c source/weapon_art.c generated/selector_art.c source/cinematic.c source/camera.c source/effects.c source/raster.c source/hud.c source/ui_skin.c source/menu.c generated/item_art.c generated/loot_art.c generated/hud_art.c generated/menu_art.c tests/thieves_stubs.c -o build/render-banked-test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Banked renderer compilation failed' }
    foreach ($scenario in 0..89) {
        & ./build/render-banked-test.exe build/banked.ppm $scenario
        if ($LASTEXITCODE -ne 0) { throw 'Banked renderer failed' }
        python -c "from pathlib import Path; assert Path('build/banked.ppm').read_bytes()==Path('build/reference-$scenario.ppm').read_bytes()"
        if ($LASTEXITCODE -ne 0) { throw "Banked pixels differ in scene $scenario" }
    }
    Write-Output 'PASS: all88 chapter-bank scenes match linked-asset reference pixels'
    if ($LASTEXITCODE -ne 0) { throw 'Hardware-overlay pixel comparison failed' }
    python -c "from pathlib import Path; b=Path('build/reference-10.ppm').read_bytes(); n=256*192*3+len(b'P6\n256 192\n255\n'); assert all(b[i*n:(i+1)*n]==b[2*n:3*n] for i in range(3,120)); print('PASS: held shield stays visually stable after raising')"
    if ($LASTEXITCODE -ne 0) { throw 'Shield stability test failed' }
    & ./build/render-preview.exe build/preview.ppm
    if ($LASTEXITCODE -ne 0) { throw 'Preview render failed' }
    python -c "from PIL import Image; im=Image.open('build/preview.ppm'); im.save('build/preview.png'); im.resize((768,576),Image.Resampling.NEAREST).save('build/preview-3x.png')"
    if ($LASTEXITCODE -ne 0) { throw 'Preview conversion failed' }
} finally { $env:PATH = $savedPath; Pop-Location }
