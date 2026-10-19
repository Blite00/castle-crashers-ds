"""Run tools/pipeline.py steps. Resumable: --from N, --to N, --list."""
import argparse, os, subprocess, sys, time
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import pipeline
FFDEC = None
FFMPEG = os.environ.get('FFMPEG', 'ffmpeg')

def expand(step, game):
    return [str(a).replace('{game}', str(game)).replace('{ffdec}', str(FFDEC)).replace('{ffmpeg}', FFMPEG) for a in step]

def run(step, game, ffdec, java='java'):
    kind, *args = expand(step, game)
    env = os.environ.copy()
    env['APPDATA'] = str(ROOT / 'build' / 'appdata')
    env['FFDEC_JAR'] = str(ffdec)
    env['CC_GAME_DIR'] = str(game)
    env['CC_DEFER_CHAPTER_BANKS'] = '1'
    env['FFMPEG'] = FFMPEG
    env.setdefault('PYTHONIOENCODING', 'utf-8')
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
