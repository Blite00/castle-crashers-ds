"""Record the LEVEL2 barracks scene (sprite 378) placements in DS world units
(generated/barracks-source-audit.json), read by convert_barracks_actors.py."""
from pathlib import Path
import json
import xml.etree.ElementTree as E
R = Path(__file__).resolve().parents[1]
ORIGIN = (-500, -714); SCALE = 0.5
r = E.parse(R / 'assets/extracted/level2.xml').getroot()
scene = next(t for t in r.find('tags') if t.get('spriteId') == '378')
placements = []
for p in scene.find('subTags'):
    if p.get('type') == 'ShowFrameTag':
        break
    if not p.get('type', '').startswith('PlaceObject') or p.get('characterId') in (None, '0'):
        continue
    m = p.find('matrix')
    x = int(m.get('translateX', '0')) / 20 if m is not None else 0.0
    y = int(m.get('translateY', '0')) / 20 if m is not None else 0.0
    placements.append(dict(sprite=int(p.get('characterId')), name=p.get('name'), x=x, y=y,
                           world_x=(x - ORIGIN[0]) * SCALE, world_y=(y - ORIGIN[1]) * SCALE))
(R / 'generated').mkdir(exist_ok=True)
(R / 'generated/barracks-source-audit.json').write_text(json.dumps(dict(
    scene=378, source_origin=list(ORIGIN), world_scale=SCALE, world_size=[1280, 448], placements=placements), indent=1))
print('barracks placements', len(placements))
