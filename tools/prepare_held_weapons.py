"""Export genuine held weapon selectors over the existing original knight poses."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
original=E.parse(R/'assets/extracted/player.xml').getroot();selectors={}
for t in original.find('tags'):
 if t.get('type')!='DefineSpriteTag':continue
 actions=b''.join(bytes.fromhex(a.get('actionBytes','')) for a in t.find('subTags') if a.get('type')=='DoActionTag')
 if b'f_Weapon\0' in actions:selectors[t.get('spriteId')]=t
print('Weapon selector sprites',list(selectors))
for color in ('blue','orange','red','green'):
 for weapon in (2,9,19,10,38,30,37,42,54,55,62):
  r=E.parse(R/f'assets/extracted/{color}.xml').getroot();tags=r.find('tags')
  for i,t in enumerate(list(tags)):
   if t.get('spriteId') in selectors:
    replacement=deepcopy(selectors[t.get('spriteId')]);freeze(replacement,weapon);tags.remove(t);tags.insert(i,replacement)
  E.ElementTree(r).write(R/f'assets/extracted/held-{color}-{weapon}.xml',encoding='utf-8',xml_declaration=True)
