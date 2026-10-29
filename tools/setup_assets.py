#!/usr/bin/env python3
"""Castle Crashers DS one-step setup: find your own copy of Castle Crashers,
convert its assets, build the ROM.

This repository ships no Castle Crashers art, audio, levels or data. Everything
the DS build needs is read from the Steam (PC) release of Castle Crashers that
you own, and converted on your machine. The converted files land in ignored
folders (assets/, generated/, nitrofiles/) and in the ROM you build. Keep them
to yourself: do not commit, upload or share them.

  python tools/setup_assets.py                       # auto-detect the game, convert, build
  python tools/setup_assets.py --game "D:/Steam/steamapps/common/CastleCrashers"
  python tools/setup_assets.py --steam-download YOUR_STEAM_LOGIN
  python tools/setup_assets.py --no-build            # only convert assets
  python tools/setup_assets.py --resume 120          # continue a conversion from step 120
  python tools/setup_assets.py --build-only          # rebuild the ROM from converted assets
  python tools/setup_assets.py --find                # only show where the game was found

The game folder is only read, never modified.
"""
import argparse
import hashlib
import io
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / 'tools'
GENERATED = ROOT / 'generated'
LOCAL_TOOLS = ROOT / '.tools'
ROM = ROOT / 'castle-crashers-ds.nds'
FINISHED = ROOT / 'finished_rom'      # the finished ROM is copied here for players
STEAM_APP_ID = '204360'              # Castle Crashers on Steam.
STEAM_DIR_NAME = 'CastleCrashers'

# Game archives every part of the conversion reads.
REQUIRED_FILES = ['data/game/player.pak', 'data/game/main.pak', 'data/game/fx.pak', 'data/game/menu.pak',
                  'data/levels/level13.pak', 'data/levels/level49.pak', 'data/bsps/bsp13.pak',
                  'data/music/INTRO.xma', 'data/sounds/sound_zap.xma']
# sha256 of data/game/player.pak in releases this project was tested with.
KNOWN_RELEASES = {'6fc1f346b2889f2378b459eba4569567a68c3c44e71544bb42443eaf47211915': 'Steam (Windows), 2.x'}

# JPEXS Free Flash Decompiler renders the Flash artwork. The conversion was
# verified with this exact version; other versions can rasterize differently.
FFDEC_VERSION = '22.0.2'
FFDEC_URL = f'https://github.com/jindrapetrik/jpexs-decompiler/releases/download/version{FFDEC_VERSION}/ffdec_{FFDEC_VERSION}.zip'

# Ethan Roseman's MIT-licensed Castle Crashers archive extractor. It is not
# shipped in this repository (see NOTICE.md); the setup fetches this exact file.
DECRYPTOR = TOOLS / 'vendor' / 'decrypt_asset.py'
DECRYPTOR_COMMIT = 'fc59b55300b44d8beb53f2cfbb6bcad570e9ae09'
DECRYPTOR_URL = f'https://raw.githubusercontent.com/ethteck/castlecrashers/{DECRYPTOR_COMMIT}/decrypt_asset.py'
DECRYPTOR_SHA256 = '9d9d4da1b7d2825ee094232e7f75b9743d3ff566a29b1fbe56cbf7207b70df07'
# On Windows the setup installs its own MSYS2 with the Wonderful toolchain's
# BlocksDS into .tools/msys64 when no DS toolchain is found.
MSYS2_URL = 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-base-x86_64-latest.sfx.exe'
WONDERFUL_URL = 'https://wonderful.asie.pl/bootstrap/wf-bootstrap-windows-x86_64.tar.gz'
PIP_PACKAGES = {'PIL': 'Pillow', 'numpy': 'numpy', 'blowfish': 'blowfish==0.6.1'}

# Files the DS build needs; all must exist after the conversion.
REQUIRED_OUTPUTS = [
    'generated/castle_art.h', 'generated/audio_assets.h', 'generated/campaign_world.h', 'generated/cinematic_art.h',
    'generated/menu_art.c', 'generated/hud_art.c', 'generated/chapter_asset_manifest.h', 'generated/weapon_art_shared.h',
    'generated/magic_fx.h', 'generated/resident_music.h', 'generated/game-icon.gif',
    'nitrofiles/chapters/keep.bin', 'nitrofiles/audio/menu.bin', 'nitrofiles/audio/cinematic.bin',
    'nitrofiles/weapons/icons.bin', 'nitrofiles/lobby.bin',
]


def say(msg=''):
    print(msg, flush=True)


def fail(msg):
    say('\nERROR: ' + msg)
    sys.exit(1)


# ---------------------------------------------------------------- finding the game
def is_game(folder):
    return all((Path(folder) / f).is_file() for f in REQUIRED_FILES[:2])


def steam_libraries():
    roots = []
    if os.name == 'nt':
        try:
            import winreg
            for hive, key in ((winreg.HKEY_CURRENT_USER, r'Software\Valve\Steam'),
                              (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Valve\Steam'),
                              (winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Valve\Steam')):
                for value in ('SteamPath', 'InstallPath'):
                    try:
                        with winreg.OpenKey(hive, key) as k:
                            roots.append(Path(winreg.QueryValueEx(k, value)[0]))
                    except OSError:
                        pass
        except ImportError:
            pass
        for env in ('ProgramFiles(x86)', 'ProgramFiles'):
            if os.environ.get(env):
                roots.append(Path(os.environ[env]) / 'Steam')
        roots += [Path(d + ':/SteamLibrary') for d in 'CDEFGH']
    else:
        home = Path.home()
        roots += [home / '.steam/steam', home / '.local/share/Steam', home / '.var/app/com.valvesoftware.Steam/.local/share/Steam',
                  home / 'Library/Application Support/Steam']
    libraries = []
    for root in roots:
        if not root.is_dir():
            continue
        libraries.append(root)
        vdf = root / 'steamapps' / 'libraryfolders.vdf'
        if vdf.is_file():
            for path in re.findall(r'"path"\s+"([^"]+)"', vdf.read_text(encoding='utf-8', errors='replace')):
                libraries.append(Path(path.replace('\\\\', '\\')))
    seen, unique = set(), []
    for lib in libraries:
        if str(lib).lower() not in seen:
            seen.add(str(lib).lower())
            unique.append(lib)
    return unique


def find_games():
    found = []
    for lib in steam_libraries():
        folder = lib / 'steamapps' / 'common' / STEAM_DIR_NAME
        if is_game(folder) and folder.resolve() not in found:
            found.append(folder.resolve())
    return found


def steam_download(user):
    """Download your own copy through SteamCMD (requires owning Castle Crashers on that account)."""
    steamcmd = shutil.which('steamcmd') or shutil.which('steamcmd.exe') or shutil.which('steamcmd.sh')
    if not steamcmd:
        fail('SteamCMD was not found. Install it (https://developer.valvesoftware.com/wiki/SteamCMD) and put '
             '"steamcmd" on your PATH, or install Castle Crashers with the normal Steam client and rerun without '
             '--steam-download.')
    target = ROOT / '.game-install'
    target.mkdir(exist_ok=True)
    say(f'Downloading Castle Crashers with SteamCMD into {target}.\nSteam asks for your password and Steam Guard '
        'code; they go straight to Steam, never to this script.')
    cmd = [steamcmd, '+@sSteamCmdForcePlatformType', 'windows', '+force_install_dir', str(target),
           '+login', user, '+app_update', STEAM_APP_ID, 'validate', '+quit']
    if subprocess.call(cmd) != 0 or not is_game(target):
        fail('SteamCMD did not produce the game data. Check that this Steam account owns Castle Crashers.')
    return target


def verify(game):
    missing = [f for f in REQUIRED_FILES if not (game / f).is_file()]
    if missing:
        fail(f'{game} does not look like a complete Castle Crashers (Steam, PC) install; missing: ' + ', '.join(missing))
    sha = hashlib.sha256((game / 'data/game/player.pak').read_bytes()).hexdigest()
    label = KNOWN_RELEASES.get(sha)
    if label:
        say(f'Verified Castle Crashers at {game}: {label}')
    else:
        say(f'Found Castle Crashers at {game} (player.pak sha256 {sha}).\nThis release has not been tested; '
            'continuing. If a conversion step fails, please open an issue with this hash.')


# ---------------------------------------------------------------- tools
def pip_install(packages):
    say('Installing Python packages: ' + ', '.join(packages))
    if subprocess.call([sys.executable, '-m', 'pip', 'install', '--user', *packages]) != 0:
        fail('Could not install %s. Run  %s -m pip install --user %s  yourself and try again.'
             % (', '.join(packages), Path(sys.executable).name, ' '.join(packages)))


def check_python():
    if sys.version_info < (3, 9):
        fail('Python 3.9 or newer is required.')
    missing = []
    for module, package in PIP_PACKAGES.items():
        try:
            __import__(module)
        except ImportError:
            missing.append(package)
    if missing:
        pip_install(missing)


def download(url, what):
    say(f'Downloading {what}\n  {url}')
    try:
        with urllib.request.urlopen(url, timeout=120) as r:
            return r.read()
    except OSError as e:
        fail(f'Could not download {what} ({e}).')


def ensure_decryptor():
    if DECRYPTOR.is_file() and hashlib.sha256(DECRYPTOR.read_bytes()).hexdigest() == DECRYPTOR_SHA256:
        return
    data = download(DECRYPTOR_URL, 'the Castle Crashers archive extractor (ethteck/castlecrashers, MIT)')
    if hashlib.sha256(data).hexdigest() != DECRYPTOR_SHA256:
        fail('The downloaded decrypt_asset.py does not match the expected version. Download it yourself from\n'
             f'  {DECRYPTOR_URL}\nand save it as {DECRYPTOR}.')
    DECRYPTOR.parent.mkdir(exist_ok=True)
    DECRYPTOR.write_bytes(data)


def winget_install(package, what):
    """Windows: install a missing tool with winget. Returns False when winget is unavailable or fails."""
    if os.name != 'nt' or not shutil.which('winget'):
        return False
    say(f'\n{what} is missing; installing it with winget ({package}). Windows may ask for permission.')
    return subprocess.call(['winget', 'install', '-e', '--id', package, '--accept-package-agreements',
                            '--accept-source-agreements', '--disable-interactivity']) == 0


def find_installed(exe, patterns):
    """Find a tool winget just installed (its folder is not on this window's PATH yet) and add it to PATH,
    so this script and every conversion step can run it by name."""
    import glob
    for pattern in patterns:
        for hit in sorted(glob.glob(os.path.expandvars(pattern)), reverse=True):
            if Path(hit).is_file():
                os.environ['PATH'] = str(Path(hit).parent) + os.pathsep + os.environ.get('PATH', '')
                return shutil.which(exe) or hit
    return None


JAVA_PATTERNS = [r'%ProgramFiles%\Eclipse Adoptium\*\bin\java.exe', r'%LOCALAPPDATA%\Programs\Eclipse Adoptium\*\bin\java.exe',
                 r'%ProgramFiles%\Java\*\bin\java.exe', r'%ProgramFiles%\Microsoft\jdk-*\bin\java.exe']
FFMPEG_PATTERNS = [r'%LOCALAPPDATA%\Microsoft\WinGet\Links\ffmpeg.exe',
                   r'%LOCALAPPDATA%\Microsoft\WinGet\Packages\Gyan.FFmpeg*\*\bin\ffmpeg.exe']


def find_java():
    java = shutil.which('java')
    if not java and os.name == 'nt':
        java = find_installed('java', JAVA_PATTERNS)
        if not java and winget_install('EclipseAdoptium.Temurin.21.JRE', 'Java'):
            java = find_installed('java', JAVA_PATTERNS)
    if not java:
        fail('Java is needed to run JPEXS FFDec (the Flash renderer). Install a Java runtime 11 or newer, for example\n'
             '  Windows: winget install EclipseAdoptium.Temurin.21.JRE   macOS: brew install --cask temurin   '
             'Linux: sudo apt install default-jre')
    return java


def ffdec_version(jar):
    try:
        out = subprocess.run(['java', '-jar', str(jar), '--help'], capture_output=True, text=True, timeout=120).stdout
    except (OSError, subprocess.SubprocessError):
        return None
    m = re.search(r'Decompiler v\.?\s*([\d.]+)', out)
    return m[1] if m else None


def find_ffdec(explicit):
    candidates = [explicit, os.environ.get('FFDEC_JAR'), LOCAL_TOOLS / 'ffdec' / 'ffdec-cli.jar',
                  'C:/Program Files (x86)/FFDec/ffdec-cli.jar', 'C:/Program Files/FFDec/ffdec-cli.jar',
                  '/usr/share/java/ffdec/ffdec-cli.jar', '/opt/ffdec/ffdec-cli.jar',
                  '/Applications/FFDec.app/Contents/Resources/ffdec-cli.jar']
    for c in candidates:
        if c and Path(c).is_file():
            jar = Path(c)
            break
    else:
        data = download(FFDEC_URL, f'JPEXS Free Flash Decompiler {FFDEC_VERSION} (GPL-3.0)')
        target = LOCAL_TOOLS / 'ffdec'
        target.mkdir(parents=True, exist_ok=True)
        zipfile.ZipFile(io.BytesIO(data)).extractall(target)
        jar = target / 'ffdec-cli.jar'
        if not jar.is_file():
            fail(f'The FFDec download did not contain ffdec-cli.jar. Unpack {FFDEC_URL} into {target} yourself.')
    version = ffdec_version(jar)
    if version != FFDEC_VERSION:
        say(f'Note: {jar} is FFDec {version or "(unknown version)"}; this project was verified with {FFDEC_VERSION}. '
            'Artwork can come out slightly different.')
    return jar.resolve()


def find_ffmpeg(explicit):
    ffmpeg = explicit or shutil.which('ffmpeg')
    if not ffmpeg and os.name == 'nt':
        ffmpeg = find_installed('ffmpeg', FFMPEG_PATTERNS)
        if not ffmpeg and winget_install('Gyan.FFmpeg', 'ffmpeg'):
            ffmpeg = find_installed('ffmpeg', FFMPEG_PATTERNS)
    if not ffmpeg:
        hint = ('winget install Gyan.FFmpeg' if os.name == 'nt' else
                'brew install ffmpeg' if platform.system() == 'Darwin' else 'sudo apt install ffmpeg')
        fail(f'ffmpeg is needed to convert the music and sound effects. Install it (for example: {hint}), '
             'or pass --ffmpeg PATH.')
    return ffmpeg


def msys_candidates():
    roots = [ROOT.parent / '.tools' / 'msys64', LOCAL_TOOLS / 'msys64']
    if os.environ.get('MSYS2_ROOT'):
        roots.append(Path(os.environ['MSYS2_ROOT']))
    return roots + [Path('C:/msys64')]


def find_msys_blocksds():
    """bash.exe of an MSYS2 that has BlocksDS (Wonderful toolchain) installed, or None."""
    for msys in msys_candidates():
        if (msys / 'usr/bin/bash.exe').is_file() and (msys / 'opt/wonderful/bin/wf-env').is_file() \
                and (msys / 'opt/wonderful/thirdparty/blocksds/core').is_dir():
            return msys / 'usr/bin/bash.exe'
    return None


def msys_run(bash, script, what):
    env = dict(os.environ, MSYSTEM='UCRT64', CHERE_INVOKING='1')
    if subprocess.call([str(bash), '-lc', script], cwd=str(ROOT), env=env) != 0:
        fail(f'Installing the DS toolchain failed while {what} (see above). Run setup again to retry; '
             f'delete {LOCAL_TOOLS / "msys64"} first if it keeps failing.')


def docker_running():
    return bool(shutil.which('docker')) and subprocess.call(
        ['docker', 'info'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0


def ensure_windows_toolchain():
    """Windows: install MSYS2 + BlocksDS into .tools/msys64 unless a DS toolchain is already there."""
    if os.name != 'nt' or find_msys_blocksds() or os.environ.get('BLOCKSDS') or docker_running():
        return
    msys = LOCAL_TOOLS / 'msys64'
    bash = msys / 'usr/bin/bash.exe'
    say(f'\nNo Nintendo DS toolchain found. Installing BlocksDS into {msys}\n'
        '(one time, about 1.5 GB; it stays inside this folder and does not touch the rest of your PC).')
    if not bash.is_file():
        LOCAL_TOOLS.mkdir(exist_ok=True)
        sfx = LOCAL_TOOLS / 'msys2-base.sfx.exe'
        sfx.write_bytes(download(MSYS2_URL, 'MSYS2 (a small Unix environment BlocksDS runs in)'))
        if subprocess.call([str(sfx), '-y', '-o' + str(LOCAL_TOOLS)]) != 0 or not bash.is_file():
            fail(f'Could not unpack MSYS2 into {LOCAL_TOOLS}.')
        sfx.unlink()
        msys_run(bash, 'true', 'setting up MSYS2')
        # The first update may replace MSYS2's own core and stop early; the second finishes it.
        subprocess.call([str(bash), '-lc', 'pacman -Syuu --noconfirm'], cwd=str(ROOT),
                        env=dict(os.environ, MSYSTEM='UCRT64', CHERE_INVOKING='1'))
        msys_run(bash, 'pacman -Syuu --noconfirm', 'updating MSYS2')
    msys_run(bash, 'pacman -S --needed --noconfirm make tar curl mingw-w64-ucrt-x86_64-gcc',
             'installing make and gcc')
    msys_run(bash, 'set -e; if [ ! -x /opt/wonderful/bin/wf-pacman ] && [ ! -f /opt/wonderful/bin/wf-pacman.exe ]; then '
             f'mkdir -p /opt/wonderful && curl -fL --retry 3 -o /tmp/wf-bootstrap.tar.gz {WONDERFUL_URL} && '
             'tar xzf /tmp/wf-bootstrap.tar.gz -C /opt/wonderful && rm -f /tmp/wf-bootstrap.tar.gz; fi',
             'downloading the Wonderful toolchain')
    msys_run(bash, 'set -e; export PATH=/opt/wonderful/bin:$PATH; wf-pacman -Syu --noconfirm wf-tools && '
             'wf-config repo enable blocksds && wf-pacman -Syu --noconfirm && '
             'wf-pacman -S --needed --noconfirm blocksds-toolchain', 'installing BlocksDS')
    if not find_msys_blocksds():
        fail(f'BlocksDS did not end up in {msys}. Install it by hand (https://blocksds.skylyrac.net/docs/setup/windows/) '
             'or install Docker, then run this again with --build-only.')
    say('BlocksDS installed.')


def check_host_compiler():
    sys.path.insert(0, str(TOOLS))
    import host_cc
    return host_cc.path(ROOT)


# ---------------------------------------------------------------- convert + build
def remember(game, ffdec, ffmpeg):
    """Save the tool locations for tools/check.py and later runs (git-ignored)."""
    import json
    LOCAL_TOOLS.mkdir(exist_ok=True)
    (LOCAL_TOOLS / 'setup.json').write_text(json.dumps(dict(game=str(game), ffdec=str(ffdec), ffmpeg=str(ffmpeg)), indent=1))


def run_pipeline(game, ffdec, ffmpeg, start):
    remember(game, ffdec, ffmpeg)
    env = dict(os.environ, FFDEC_JAR=str(ffdec), CC_GAME_DIR=str(game), FFMPEG=str(ffmpeg))
    say('\nConverting (this takes a while: FFDec renders several thousand frames) ...')
    code = subprocess.call([sys.executable, '-W', 'ignore', str(TOOLS / 'run_pipeline.py'), '--game', str(game),
                            '--ffdec', str(ffdec), '--from', str(start)], cwd=str(ROOT), env=env)
    if code != 0:
        fail('A conversion step failed (see the output above). Fix the cause, then continue from that step with\n'
             '  python tools/setup_assets.py --resume N')
    missing = [f for f in REQUIRED_OUTPUTS if not (ROOT / f).is_file()]
    if missing:
        fail('The conversion finished but these files are missing: ' + ', '.join(missing))
    say('Assets converted into assets/, generated/ and nitrofiles/ (all git-ignored).')


def build_rom():
    blocksds = os.environ.get('BLOCKSDS')
    wonderful = Path('/opt/wonderful/thirdparty/blocksds/core')
    msys_bash = find_msys_blocksds() if os.name == 'nt' else None
    if msys_bash:
        say(f'Building with BlocksDS ({msys_bash.parents[2]}) ...')
        env = dict(os.environ, MSYSTEM='UCRT64', CHERE_INVOKING='1')
        code = subprocess.call([str(msys_bash), '-lc', 'source /opt/wonderful/bin/wf-env && make -j2'], cwd=str(ROOT), env=env)
    elif (blocksds and Path(blocksds).is_dir()) or wonderful.is_dir():
        say('Building with BlocksDS ...')
        if Path('/opt/wonderful/bin/wf-env').is_file() and shutil.which('bash'):
            code = subprocess.call(['bash', '-lc', 'source /opt/wonderful/bin/wf-env && make -j2'], cwd=str(ROOT))
        else:
            code = subprocess.call(['make', '-j2'], cwd=str(ROOT))
    elif docker_running():
        say('Building inside the BlocksDS Docker image (the first run downloads it) ...')
        code = subprocess.call(['docker', 'run', '--rm', '-v', f'{ROOT}:/work', '-w', '/work',
                                'skylyrac/blocksds:slim-latest', 'make', '-j2'])
    else:
        if shutil.which('docker'):
            say('\nDocker is installed but not running. Start Docker (Docker Desktop on Windows/macOS), then run\n'
                'this again with --build-only.')
            return False
        say('\nAssets are ready, but no Nintendo DS toolchain was found, so the ROM was not built.\n'
            'Install BlocksDS (https://blocksds.skylyrac.net/docs/setup/) or Docker, then run this again\n'
            'with --build-only. See README.md.')
        return False
    if code != 0 or not ROM.is_file():
        fail('The DS build failed. See the compiler output above.')
    subprocess.call([sys.executable, str(TOOLS / 'check_memory_budget.py')], cwd=str(ROOT))
    FINISHED.mkdir(exist_ok=True)
    shutil.copy2(ROM, FINISHED / ROM.name)
    bar = '=' * 72
    say(f'\n{bar}\n  FINISHED! Your Castle Crashers DS ROM is ready:\n\n    {FINISHED / ROM.name}\n\n'
        '  Copy it to your flashcart or open it in melonDS. It contains converted\n'
        '  Castle Crashers assets: keep it for yourself and do not share it.\n' + bar)
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', type=Path, help='the Castle Crashers install folder (contains data/)')
    ap.add_argument('--steam-download', metavar='STEAM_LOGIN',
                    help='download your own copy with SteamCMD (the account must own Castle Crashers)')
    ap.add_argument('--ffdec', help='path to ffdec-cli.jar (default: found or downloaded into .tools/ffdec)')
    ap.add_argument('--ffmpeg', help='path to ffmpeg (default: found on PATH)')
    ap.add_argument('--resume', type=int, default=0, metavar='N', help='continue the conversion from step N')
    ap.add_argument('--no-build', action='store_true', help='convert assets only; do not build the ROM')
    ap.add_argument('--build-only', action='store_true', help='skip the conversion; build from existing assets')
    ap.add_argument('--no-toolchain-install', action='store_true',
                    help='Windows: do not install MSYS2/BlocksDS into .tools/ when no DS toolchain is found')
    ap.add_argument('--find', action='store_true', help='list Castle Crashers installs found on this computer and exit')
    args = ap.parse_args()

    if args.find:
        found = find_games()
        say('\n'.join(map(str, found)) if found else 'No Castle Crashers install found automatically. Use --game PATH.')
        return
    if args.build_only:
        missing = [f for f in REQUIRED_OUTPUTS if not (ROOT / f).is_file()]
        if missing:
            fail('Converted assets are missing (%s). Run without --build-only first.' % ', '.join(missing[:4]))
        if not args.no_toolchain_install:
            ensure_windows_toolchain()
        build_rom()
        return

    check_python()
    if args.steam_download:
        game = steam_download(args.steam_download)
    elif args.game:
        game = args.game.expanduser()
        if not is_game(game) and is_game(game.parent):
            game = game.parent
    else:
        found = find_games()
        if not found:
            fail('Could not find Castle Crashers automatically. Install it with Steam and run again, or point to it:\n'
                 '  python tools/setup_assets.py --game "PATH/TO/CastleCrashers"\n'
                 'or download your own copy with SteamCMD:\n  python tools/setup_assets.py --steam-download STEAM_LOGIN')
        game = found[0]
        if len(found) > 1:
            say('Several installs found; using the first (choose another with --game):\n  ' + '\n  '.join(map(str, found)))
    game = game.resolve()
    if ROOT in game.parents and '.game-install' not in game.parts:
        fail('Keep the game outside this repository folder so it can never be committed by accident.')
    verify(game)
    find_java()
    ffdec = find_ffdec(args.ffdec)
    ffmpeg = find_ffmpeg(args.ffmpeg)
    if not args.no_toolchain_install:
        ensure_windows_toolchain()  # before the long conversion; it also provides gcc
    check_host_compiler()
    ensure_decryptor()
    run_pipeline(game, ffdec, ffmpeg, args.resume)
    if not args.no_build:
        build_rom()


if __name__ == '__main__':
    main()
