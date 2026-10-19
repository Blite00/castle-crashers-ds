"""Original level-up burst and New Combo text; no substitute font artwork."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
for name,file,sid in [('level','player',30),('level-label','player',29),('combo','main',342)]:
 r=E.parse(R/f'assets/extracted/{file}.xml').getroot();ts=r.find('tags')
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
  elif t.get('type')=='DefineSpriteTag':freeze(t,1)
  elif name.startswith('level') and t.get('type')=='DefineEditTextTag' and t.get('characterID') in ('26','28'):t.set('initialText','')
 # Direct original title; the game supplies its unlocked DS button sequence separately.
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag');r.set('frameCount','1')
 for k,v in [('Xmin',-5120),('Xmax',5120),('Ymin',-3840),('Ymax',3840)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/progression-{name}.xml')
