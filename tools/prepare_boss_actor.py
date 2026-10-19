"""Retain authored EBARBOSS motion and classic faces; hide collision guides only."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
original=E.parse(R/'assets/extracted/ebarboss.xml').getroot()
spec=[('idle',3,1,[1]),('walk',4,36,[1,7,13,19,25,31]),('punch',11,36,[1,7,13,19,25,31,36]),('spikes',13,68,[1,9,17,25,33,41,49,57,65]),('slam',14,91,[1,9,13,34,38,54,62,70,78,90]),('fire',16,143,[1,17,33,49,65,81,97,108,124,142]),('die',21,123,[1,9,17,25,33,49,65,73,97,121])]
for name,rootframe,count,samples in spec:
 r=deepcopy(original);ts=r.find('tags');fake=E.Element('item',type='DefineSpriteTag',frameCount=r.get('frameCount'));sub=E.SubElement(fake,'subTags')
 for t in ts:
  if not t.get('type','').startswith('Define'):sub.append(deepcopy(t))
 freeze(fake,rootframe)
 defs=[t for t in ts if t.get('type','').startswith('Define')]
 for t in defs:
  if t.get('type')=='DefineSpriteTag':
   sid=int(t.get('spriteId'))
   if sid in (7,9,10):t.find('subTags').clear();E.SubElement(t.find('subTags'),'item',type='ShowFrameTag');t.set('frameCount','1')
 ts.clear();ts.extend(defs)
 for t in fake.find('subTags'):
  if t.get('type')!='ShowFrameTag':ts.append(deepcopy(t))
 for _ in range(count):E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount',str(count))
 for k,v in [('Xmin',-10240),('Xmax',10240),('Ymin',-12800),('Ymax',2560)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/boss-actor-{name}.xml')
(R/'assets/boss-actor-spec.json').write_text(json.dumps(spec,indent=2))
