# Castle Crashers DS — an unofficial Nintendo DS demake (early build)

> **Early, in-progress build.** Expect missing features, rough edges and bugs. It has been developed and tested in emulators and native host tests; it has **not** been verified on real DS hardware, and DS-to-DS wireless play is untested.

A from-scratch Nintendo DS engine (C, BlocksDS) that plays parts of **Castle Crashers** on a Nintendo DS (homebrew flashcart) or in a DS emulator:

- **Arena:** two-player local-wireless duels and a CPU practice opponent on three original arenas, with the original knights, combos, magic, bow, blocking, throws and weapon clashes.
- **Campaign (two players):** Home Castle, Barracks, Castle Keep with its cinematics, the shop, Barbarian War, the bridge, the Barbarian Boss, the blacksmith/frog/ark hubs, Forest Entrance (with the crocodile mount), Thieves' Forest and the Troll Mother, the Mill Run deer chase, the Flooded Castle with the Catfish and Tall Grass Field with the bear boss. World map, levelling and stats, weapons, pets, potions, shop, score tally and saves.

**This repository contains no Castle Crashers assets.** No art, animation, music, sound, levels or game data from Castle Crashers is stored here, and neither is the tool that unpacks the game's archives. You need your own copy of Castle Crashers (Steam, PC); a setup script reads it on your computer, converts what the DS needs, and builds the ROM locally.

> Unofficial fan project. Not affiliated with, endorsed by or supported by The Behemoth. *Castle Crashers* is © The Behemoth; all names, characters and content belong to them. Buy the game: [Steam](https://store.steampowered.com/app/204360/Castle_Crashers/).

## What you need

| | |
|---|---|
| **Castle Crashers** | Your own copy of the Steam (PC) release, installed. The setup only reads its `data/` folder; it never changes your game. |
| **Python 3.9+** | [python.org](https://www.python.org/downloads/). The setup installs Pillow, NumPy and `blowfish` with pip if they are missing. |
| **Java 11+** | Runs [JPEXS FFDec](https://github.com/jindrapetrik/jpexs-decompiler), which renders the Flash artwork. Windows: `winget install EclipseAdoptium.Temurin.21.JRE` · macOS: `brew install --cask temurin` · Linux: `sudo apt install default-jre`. The setup downloads FFDec 22.0.2 itself if you do not have it. |
| **ffmpeg** | Converts the music and sound. Windows: `winget install Gyan.FFmpeg` · macOS: `brew install ffmpeg` · Linux: `sudo apt install ffmpeg` |
| **A C compiler** | A few converters compile small helpers. Windows: [MSYS2](https://www.msys2.org/) with `pacman -S mingw-w64-ucrt-x86_64-gcc` · macOS: `xcode-select --install` · Linux: `gcc` |
| **A DS toolchain** (to build the ROM) | [BlocksDS](https://blocksds.skylyrac.net/docs/setup/) **or** [Docker](https://www.docker.com/) (the setup then uses the `skylyrac/blocksds` image automatically). |
| **Internet, once** | The setup downloads FFDec and the archive extractor (see below). |

## Quick start

1. Install Castle Crashers with Steam, and the tools above.
2. Download this repository (green **Code** button → *Download ZIP*, or `git clone`).
3. Run the setup:
   - **Windows:** double-click `setup.bat`
   - **macOS / Linux:** `./setup.sh`
4. When it finishes you have **`castle-crashers-ds.nds`**. Copy it to your flashcart's microSD card or open it in [melonDS](https://melonds.kuribo64.net/).

The conversion renders several thousand Flash frames and takes a while (roughly an hour on a typical PC). If it stops, fix the cause and continue where it left off with `python tools/setup_assets.py --resume N` (the step number is printed).

The setup finds Castle Crashers in your Steam libraries (including extra library folders). If yours is elsewhere:

```sh
python tools/setup_assets.py --game "D:/Games/Steam/steamapps/common/CastleCrashers"
```

No install on this computer? If you own Castle Crashers on Steam, the setup can download your copy with Valve's [SteamCMD](https://developer.valvesoftware.com/wiki/SteamCMD) (you log in to Steam yourself; ownership is checked by Steam):

```sh
python tools/setup_assets.py --steam-download YOUR_STEAM_LOGIN
```

Other options: `--find` lists the installs it can see, `--no-build` only converts assets, `--build-only` rebuilds the ROM from already converted assets, `--ffdec PATH` and `--ffmpeg PATH` choose those tools.

## What the setup does

1. **Finds and checks your copy** and reports whether that release has been tested.
2. **Fetches two third-party tools** into git-ignored folders: [JPEXS FFDec](https://github.com/jindrapetrik/jpexs-decompiler) 22.0.2 (GPL-3.0) into `.tools/ffdec/`, and Ethan Roseman's MIT-licensed archive extractor [`decrypt_asset.py`](https://github.com/ethteck/castlecrashers) (a pinned version, checked by hash) into `tools/vendor/`.
3. **Converts assets locally.** About 415 steps (`tools/pipeline.py`) unpack the game's `.pak` archives, render the Flash artwork with FFDec at DS resolution, and convert art, animation, level geometry and audio into DS formats in `assets/`, `generated/` and `nitrofiles/`. All three folders are git-ignored.
4. **Builds the ROM.** BlocksDS compiles the engine in `source/` and `arm7/` together with your converted assets into `castle-crashers-ds.nds`.

The engine code, the converters and the tests are original work in this repository. The converted assets and the ROM contain The Behemoth's content, so **keep them for yourself: do not upload, share or distribute `castle-crashers-ds.nds`, `assets/`, `generated/` or `nitrofiles/`.** Point people to this repository instead; they build their own ROM from their own copy. See [NOTICE.md](NOTICE.md).

## Playing

- **Flashcart:** copy `castle-crashers-ds.nds` to the microSD card. The campaign saves to the card (DLDI must be enabled; most kernels do this automatically).
- **Emulator:** [melonDS](https://melonds.kuribo64.net/) with DLDI/SD card emulation enabled so the game can save.
- **Two players:** both consoles need the same ROM. One player picks **Multiplayer → Host Room**, the other **Join Room**. This uses direct DS-to-DS local wireless; it is not verified on real hardware yet.

| DS control | Action |
| --- | --- |
| D-pad | Move |
| A | Light attack (A A A A: four-hit chain) |
| X | Heavy attack (X X X: chain ending in a slam); next to a grounded opponent: throw |
| A then X | Uppercut / launcher (campaign level 8) |
| B | Jump; press again in the air to spin |
| R + D-pad | Block (slow movement with shield) |
| L + A / L + X | Magic projectile / magic splash (airborne L + A: downward shot) |
| Y or tap the item square | Use the selected item (potion, bow) |
| START / SELECT | Pause / back to the menu |

## For developers

```sh
python tools/setup_assets.py --no-build   # convert assets once
python tools/check.py                     # host tests (gcc/clang): runs everything listed in check.ps1
make                                      # with BLOCKSDS set, or use --build-only
```

Windows users with the portable toolchain next to this folder (`../.tools/msys64`) can keep using `build.ps1` and `check.ps1`. `python tools/run_pipeline.py --list` prints every conversion step; `--from N --to M` reruns a range.

Notes for converter work:

- Each converter reads only files from your game copy or earlier steps. If you add one, add its step to `tools/pipeline.py`, so a fresh setup reproduces it.
- `data/frozen/` holds the few small numeric tables (such as a palette chosen once) that converters read but no longer regenerate. Never put artwork, audio or text there.
- `docs/` contains the per-version QA notes, the source audits (which sprite and frame ids a level uses) and the [new level workflow](docs/new-level-workflow.md).

### Keep the repository clean

Before every commit, run the audit. It fails on any ROM, archive, image, audio, converted file, on the archive extractor, or on any run of text taken from the game:

```sh
python tools/audit_clean.py --install-hook                       # once: checks every commit automatically
python tools/audit_clean.py --game PATH/TO/CastleCrashers        # full check against your copy (before a release)
```

CI runs the file rules on every push and pull request.

## Project layout

| Path | Contents |
|---|---|
| `source/`, `arm7/` | DS engine (C, BlocksDS/libnds, DSWiFi, maxmod) |
| `tools/` | Converters (`prepare_*`, `export_*`, `convert_*`, …), `pipeline.py`, `setup_assets.py`, `check.py`, `audit_clean.py` |
| `tests/` | Host tests: combat, campaign levels, renderer, protocol, saves, converted-asset checks |
| `docs/` | QA notes per version, source audits, design notes |
| `data/frozen/` | Small numeric converter inputs (no game content) |
| `assets/`, `generated/`, `nitrofiles/` | *Created by the setup from your copy; git-ignored* |

## Status

Early build (internal version 0.91). New since 0.75: Tall Grass Field (Level 20) with the bear boss, the Rammy, Hawkster and Burly Bear pets and the Lobster, Umbrella and Wrench weapons; the Flooded Castle's original fish, bat and Catfish scripts, riding logs and sound effects; a reworked Barbarian Boss intro with its original sounds (see `docs/CHANGELOG.md`). Known gaps are listed per level in `docs/v0xx-qa.md`; for example several original cutscenes, the parallax skies and some sound effects are missing. Nothing has been run on real DS hardware yet; reports from flashcart users are very welcome.

## License

The code in this repository is released under the MIT License ([LICENSE](LICENSE)). That license covers only this repository's own code and documentation. It grants no rights to Castle Crashers or to anything the setup converts from it; see [NOTICE.md](NOTICE.md).
