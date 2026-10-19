"""FFDec exports for convert_cinematics.py: the CLIPS frames of each <name>-actors
movie into assets/<name>-actors and frame 1 of each <name>-bg into assets/<name>-atlas."""
from pathlib import Path
import os, subprocess, sys
R = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(R / 'tools'))
from convert_cinematics import CLIPS
env = os.environ.copy(); env['APPDATA'] = str(R / 'build/appdata')
jar = os.environ.get('FFDEC_JAR', 'C:/Program Files (x86)/FFDec/ffdec-cli.jar')

def ffdec(*args):
    subprocess.run(['java', '-Xmx3g', '-jar', jar, *args], env=env, check=True, cwd=R)

for name, _sid, _total, _origin, frames in CLIPS:
    for part in (f'{name}-actors', f'{name}-bg'):
        ffdec('-xml2swf', f'assets/extracted/{part}.xml', f'assets/extracted/{part}.swf')
    ffdec('-ignorebackground', '-select', ','.join(map(str, frames)), '-export', 'frame', f'assets/{name}-actors', f'assets/extracted/{name}-actors.swf')
    ffdec('-ignorebackground', '-select', '1', '-export', 'frame', f'assets/{name}-atlas', f'assets/extracted/{name}-bg.swf')
