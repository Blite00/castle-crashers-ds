"""Run tools/pipeline.py steps. Resumable: --from N, --to N, --list."""
import argparse, os, site, subprocess, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import pipeline
FFDEC = None
FFMPEG = os.environ.get('FFMPEG', 'ffmpeg')
# Steps run with APPDATA pointed at build/appdata so FFDec keeps its settings
# there. On Windows Python also derives the per-user site-packages folder from
# APPDATA, which would hide packages that pip installed with --user (Pillow,
# numpy, blowfish). Pin the real per-user folder so every step still finds them.
USER_BASE = os.environ.get('PYTHONUSERBASE') or site.getuserbase()
REQUIRED_MODULES = ('PIL', 'numpy', 'blowfish')

def step_env(game, ffdec):
    env = os.environ.copy()
    env['APPDATA'] = str(ROOT / 'build' / 'appdata')
    env['PYTHONUSERBASE'] = USER_BASE
    env['FFDEC_JAR'] = str(ffdec)
    env['CC_GAME_DIR'] = str(game)
    env['CC_DEFER_CHAPTER_BANKS'] = '1'
    env['FFMPEG'] = FFMPEG
    env.setdefault('PYTHONIOENCODING', 'utf-8')
    return env

def check_modules(game, ffdec):
    """Fail early, with a fix, if the steps' Python cannot import the packages."""
    code = 'import importlib.util, sys; m = [m for m in %r if importlib.util.find_spec(m) is None]; sys.exit(", ".join(m) if m else 0)' % (REQUIRED_MODULES,)
    r = subprocess.run([sys.executable, '-c', code], cwd=str(ROOT), env=step_env(game, ffdec), capture_output=True, text=True)
    if r.returncode != 0:
        missing = (r.stderr or r.stdout).strip()
        sys.exit(f'Python packages missing for {sys.executable}: {missing}\n'
                 f'Install them with:\n  "{sys.executable}" -m pip install Pillow numpy blowfish==0.6.1\n'
                 'then run setup again.')

def expand(step, game):
    return [str(a).replace('{game}', str(game)).replace('{ffdec}', str(FFDEC)).replace('{ffmpeg}', FFMPEG) for a in step]

def run(step, game, ffdec, java='java'):
    kind, *args = expand(step, game)
    env = step_env(game, ffdec)
    if kind == 'decrypt':
        cmd = [sys.executable, 'tools/vendor/decrypt_asset.py', str(Path(game) / args[0]), 'assets/extracted']
    elif kind == 'ffdec':
        cmd = [java, '-Xmx3g', '-jar', str(ffdec)] + args
    elif kind == 'py':
        cmd = [sys.executable] + args
    else:
        raise ValueError(kind)
    return subprocess.run(cmd, cwd=str(ROOT), env=env).returncode

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--game', default=os.environ.get('CC_GAME_DIR'))
    ap.add_argument('--ffdec', default=os.environ.get('FFDEC_JAR'))
    ap.add_argument('--from', dest='start', type=int, default=0)
    ap.add_argument('--to', type=int, default=None)
    ap.add_argument('--list', action='store_true')
    a = ap.parse_args()
    global FFDEC
    FFDEC = a.ffdec
    if a.list:
        for i, s in enumerate(pipeline.STEPS): print(i, ' '.join(s))
        return
    for d in ('assets/extracted', 'generated', 'nitrofiles', 'build/appdata'):
        (ROOT / d).mkdir(parents=True, exist_ok=True)
    end = len(pipeline.STEPS) if a.to is None else a.to + 1
    check_modules(a.game, a.ffdec)
    for i in range(a.start, end):
        s = pipeline.STEPS[i]
        t = time.time()
        print(f'[{i}/{len(pipeline.STEPS)-1}] {" ".join(s)}', flush=True)
        if run(s, a.game, a.ffdec) != 0:
            sys.exit(f'FAILED at step {i}: {" ".join(s)}')
        dt = time.time() - t
        if dt > 20: print(f'    ({dt:.0f}s)', flush=True)
    print('Pipeline finished.')

if __name__ == '__main__':
    main()
