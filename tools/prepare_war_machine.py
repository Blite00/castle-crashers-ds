"""Retain authored EWARMACH motion and classic faces; hide collision guides only."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
original=E.parse(R/'assets/extracted/ewarmach.xml').getroot()
spec=[('idle',2,1,[1]),('walk',3,16,[1,5,9,13]),('jack',4,121,[1,8,16,24,32,42,54,66,75,90,106,121]),('die',5,189,[1,9,21,37,55,75,101,126,156,188])]
for name,rootframe,count,samples in spec:
 r=deepcopy(original);ts=r.find('tags');fake=E.Element('item',type='DefineSpriteTag',frameCount=r.get('frameCount'));sub=E.SubElement(fake,'subTags')
 for t in ts:
  if not t.get('type','').startswith('Define'):sub.append(deepcopy(t))
 freeze(fake,rootframe)
 defs=[t for t in ts if t.get('type','').startswith('Define')]
 for t in defs:
  if t.get('type')=='DefineSpriteTag':
   sid=int(t.get('spriteId'))
   if sid in (42,45,48,14,60):freeze(t,1)
   if sid==27 and name=='idle':freeze(t,1)
   if sid==30:t.find('subTags').clear();E.SubElement(t.find('subTags'),'item',type='ShowFrameTag');t.set('frameCount','1')
 ts.clear();ts.extend(defs)
 for t in fake.find('subTags'):
  if t.get('type')!='ShowFrameTag':ts.append(deepcopy(t))
 for _ in range(count):E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount',str(count))
 for k,v in [('Xmin',-7680),('Xmax',7680),('Ymin',-11200),('Ymax',1600)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/war-machine-{name}.xml')
(R/'assets/war-machine-spec.json').write_text(json.dumps(spec,indent=2))
