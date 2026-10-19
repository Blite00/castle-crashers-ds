#!/usr/bin/env python3
"""Check that the repository contains no Castle Crashers assets before you commit or push.

1. File rules (always): no ROMs, archives, Flash files, images, audio, converted
   binaries, assets/, generated/ or nitrofiles/ content, and not the archive
   extractor (tools/vendor/decrypt_asset.py, fetched by the setup) among the
   files git would publish.
2. Content check (with --game): every file is compared against your own copy.
   A file identical to a game file or to a decrypted game SWF fails the audit,
   and so does any run of 40+ readable characters taken from the game's SWFs
   (text, script strings).

  python tools/audit_clean.py                   # file rules only (also run in CI)
  python tools/audit_clean.py --game PATH       # full check against your copy
  python tools/audit_clean.py --install-hook    # run the file rules before every commit

Exit status 0 means clean.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_DIRS = ('assets/', 'generated/', 'nitrofiles/', 'build/', '.tools/', '.game-install/', 'releases/', 'trailer/')
FORBIDDEN_FILES = ('tools/vendor/decrypt_asset.py', 'tools/vendor/blowfish.py')
FORBIDDEN_EXT = {'.nds', '.pak', '.swf', '.xma', '.dds', '.fnt', '.png', '.gif', '.bmp', '.jpg', '.jpeg', '.ogg', '.wav',
                 '.mp3', '.adpcm', '.pcm', '.bin', '.raw', '.pkl', '.ppm', '.sav', '.zip', '.7z', '.xz', '.gz', '.elf',
                 '.exe', '.dll', '.ttf', '.mp4', '.webm'}
MIN_RUN = 40


def repo_files():
    """Files git would publish (tracked + untracked, minus .gitignore); falls back to a walk."""
    try:
        out = subprocess.run(['git', 'ls-files', '-co', '--exclude-standard', '-z'], cwd=str(ROOT),
                             capture_output=True, check=True).stdout
        return sorted(p for p in out.decode('utf-8').split('\0') if p)
    except (OSError, subprocess.CalledProcessError):
        skip = ('.git/',) + FORBIDDEN_DIRS
        files = []
        for p in ROOT.rglob('*'):
            rel = p.relative_to(ROOT).as_posix()
            if p.is_file() and not rel.startswith(skip) and '__pycache__' not in rel and rel not in FORBIDDEN_FILES:
                files.append(rel)
        return sorted(files)


def file_rules(files):
    problems = []
    for rel in files:
        if rel.startswith(FORBIDDEN_DIRS):
            problems.append(f'{rel}: converted/private folder must stay local')
        elif rel in FORBIDDEN_FILES:
            problems.append(f'{rel}: the archive extractor is downloaded by the setup and must not be committed')
        elif Path(rel).suffix.lower() in FORBIDDEN_EXT:
            problems.append(f'{rel}: {Path(rel).suffix} files are not allowed (could carry game assets)')
    return problems


def game_fingerprints(game):
    sys.path.insert(0, str(ROOT / 'tools' / 'vendor'))
    try:
        import decrypt_asset as dec
    except ImportError:
        sys.exit('The content check decrypts your game archives: run tools/setup_assets.py once first '
                 '(it fetches tools/vendor/decrypt_asset.py).')
    hashes, runs = {}, set()
    for f in sorted((game / 'data').rglob('*')):
        if not f.is_file():
            continue
        data = f.read_bytes()
        hashes[hashlib.sha256(data).hexdigest()] = f.relative_to(game).as_posix()
        if f.suffix == '.pak':
            try:
                name, enc = dec.unzip_to_cok6_nrec(f)
                plain = dec.blowfish_decrypt(name, enc)
            except Exception:
                continue
            hashes[hashlib.sha256(plain).hexdigest()] = f.relative_to(game).as_posix() + ' (decrypted)'
            for m in re.finditer(rb'[\x20-\x7e]{%d,}' % MIN_RUN, plain):
                run = m[0].decode('ascii').strip()
                if len(run) >= MIN_RUN and ' ' in run:
                    runs.add(run)
    return hashes, runs


def content_check(files, game):
    hashes, runs = game_fingerprints(game)
    print(f'Comparing {len(files)} files against {len(hashes)} game files and {len(runs)} text runs...')
    by_prefix = {}
    for r in runs:
        by_prefix.setdefault(r[:24], []).append(r)
    problems = []
    for rel in files:
        data = (ROOT / rel).read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if digest in hashes:
            problems.append(f'{rel}: identical to game file {hashes[digest]}')
        text = data.decode('utf-8', errors='ignore')
        hits = 0
        for i in range(len(text) - 23):
            for r in by_prefix.get(text[i:i + 24], ()):
                if text.startswith(r, i):
                    if hits < 3:
                        problems.append(f'{rel}: contains game text "{r[:60]}..."')
                    hits += 1
        if hits > 3:
            problems.append(f'{rel}: ...and {hits - 3} more game-text matches')
    return problems


def install_hook():
    hook = ROOT / '.git' / 'hooks' / 'pre-commit'
    if not hook.parent.is_dir():
        sys.exit('No .git folder: run "git init" first.')
    hook.write_text('#!/bin/sh\nexec python3 tools/audit_clean.py || exec python tools/audit_clean.py\n', newline='\n')
    hook.chmod(0o755)
    print('Installed', hook)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--game', type=Path, help='your Castle Crashers folder, for the full content comparison')
    ap.add_argument('--install-hook', action='store_true')
    args = ap.parse_args()
    if args.install_hook:
        install_hook()
        return
    files = repo_files()
    problems = file_rules(files)
    if args.game:
        problems += content_check([f for f in files if (ROOT / f).is_file()], args.game)
    if problems:
        print(f'NOT CLEAN - {len(problems)} problem(s):')
        print('\n'.join('  ' + p for p in problems))
        sys.exit(1)
    print(f'Clean: {len(files)} files checked' + (f' against {args.game}.' if args.game else ' (file rules).'))


if __name__ == '__main__':
    main()
