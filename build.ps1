param([switch]$BootDiagnostic,[switch]$FtlCoreCheck,[switch]$DeferredRadio,[switch]$LocalRadioOnly)
$ErrorActionPreference = 'Stop'
$bash = Join-Path $PSScriptRoot '../.tools/msys64/usr/bin/bash.exe'
$savedMsystem = $env:MSYSTEM
$savedChere = $env:CHERE_INVOKING
Push-Location $PSScriptRoot
try {
    foreach ($asset in @('castle_art.h','arena_background.h','arena_king.h','arena_volcano.h','arena_collision.h','menu_art.h','hud_art.h','audio_assets.h','magic_fx.h','item_art.h','arena_terrain_packed.h','campaign_breakables.h','campaign_shop.h','campaign_shop_geometry.h','campaign_world.h','campaign_world_geometry.h','arena_occlusion_packed.h','barbarian_art.h','barbarian_packed.h','cinematic_art.h','neutral_npc_art.h','loot_art.h','loot_art.c','pickup_feedback_art.h','pickup_audio.h','barracks_audio.h','campaign_barracks.h','campaign_barracks_geometry.h','asset_inflate.h','weapon_art_shared.h','barracks_choreography.h')) {
        if (!(Test-Path (Join-Path 'generated' $asset))) { throw "Missing $asset. Run python tools/setup_assets.py --no-build first (see README.md)." }
    }
    foreach ($asset in @('generated/barracks_door_art.h','generated/barracks_nearwall.h','generated/run_dust_art.h','generated/barracks_rail.h','generated/cinematic_audio.h','nitrofiles/audio/cinematic.bin','nitrofiles/audio/barracks.bin','nitrofiles/weapons/icons.bin','nitrofiles/weapons/w0.bin')) {
        if (!(Test-Path $asset)) { throw "Missing release asset: $asset" }
    }
    python tools/pack_chapter_assets.py --check
    if ($LASTEXITCODE -ne 0) { throw 'Chapter banks do not match source artwork.' }
    if (!(Test-Path $bash)) { throw 'Portable BlocksDS toolchain missing from ../.tools.' }
    $env:MSYSTEM = 'UCRT64'
    $env:CHERE_INVOKING = '1'
    if ($LocalRadioOnly) {
        & $bash -lc 'source /opt/wonderful/bin/wf-env && make -B -f Makefile.arm7 -j2 LOCAL_RADIO_ONLY=1 BUILDDIR=build/arm7-local ELF=build/arm7-local.elf MAP=build/arm7-local.map && make -B -j2 NAME=castle-arena-local-radio DEFERRED_RADIO=1 LOCAL_RADIO_ONLY=1 GAME_SUBTITLE="Local radio boot test"'
    } elseif ($DeferredRadio) {
        & $bash -lc 'source /opt/wonderful/bin/wf-env && make -B -f Makefile.arm7 -j2 && make -B -j2 NAME=castle-arena-dsi-wireless LOCAL_RADIO_ONLY=0 DEFERRED_RADIO=1 GAME_SUBTITLE="DSi wireless startup test"'
    } elseif ($FtlCoreCheck) {
        & $bash -lc 'source /opt/wonderful/bin/wf-env && make -B -j2 NAME=castle-arena-ftl-core-check LOCAL_RADIO_ONLY=0 DEFERRED_RADIO=0 BOOT_DIAGNOSTIC=1 BOOT_FTL_CORE=1 GAME_SUBTITLE="FTL core boot test"'
    } elseif ($BootDiagnostic) {
        & $bash -lc 'source /opt/wonderful/bin/wf-env && make -B -j2 NAME=castle-arena-boot-check LOCAL_RADIO_ONLY=0 DEFERRED_RADIO=0 BOOT_DIAGNOSTIC=1 GAME_SUBTITLE="DSi boot diagnostics"'
    } else {
        & $bash -lc 'source /opt/wonderful/bin/wf-env && make -j2'
    }
    if ($LASTEXITCODE -ne 0) { throw 'Castle Arena build failed.' }
    if (!$BootDiagnostic -and !$FtlCoreCheck -and !$DeferredRadio -and !$LocalRadioOnly) {
        python tools/check_memory_budget.py
        if ($LASTEXITCODE -ne 0) { throw 'DS multiplayer memory budget exceeded.' }
    }
} finally {
    $env:MSYSTEM = $savedMsystem
    $env:CHERE_INVOKING = $savedChere
    Pop-Location
}
