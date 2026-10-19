"""FFDec frame exports for the LEVEL2 barracks actors listed by prepare_barracks_actors.py
(assets/barracks-actors-spec.json) and the barracks furniture sheet."""
from pathlib import Path
import json, os, subprocess
R = Path(__file__).resolve().parents[1]
env = os.environ.copy(); env['APPDATA'] = str(R / 'build/appdata')
jar = os.environ.get('FFDEC_JAR', 'C:/Program Files (x86)/FFDec/ffdec-cli.jar')

def ffdec(*args):
    subprocess.run(['java', '-Xmx3g', '-jar', jar, *args], env=env, check=True, cwd=R)

for name, _sid, frames in json.loads((R / 'assets/barracks-actors-spec.json').read_text()):
    part = 'barracks-' + name
    ffdec('-xml2swf', f'assets/extracted/{part}.xml', f'assets/extracted/{part}.swf')
    ffdec('-ignorebackground', '-select', ','.join(map(str, frames)), '-export', 'frame', f'assets/{part}', f'assets/extracted/{part}.swf')
ffdec('-xml2swf', 'assets/extracted/barracks-furniture.xml', 'assets/extracted/barracks-furniture.swf')
ffdec('-ignorebackground', '-export', 'frame', 'assets/barracks-furniture', 'assets/extracted/barracks-furniture.swf')
