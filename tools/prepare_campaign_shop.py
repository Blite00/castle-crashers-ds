from pathlib import Path
import xml.etree.ElementTree as E
import sys
sys.path.insert(0,'tools');from prepare_frames import freeze
r=E.parse('assets/extracted/level19.xml').getroot();ts=r.find('tags')
for s in ts:
 if s.get('type')=='DefineSpriteTag':
  sid=int(s.get('spriteId'));freeze(s,1)
  if sid in (16,140,147,149,152):
   s.find('subTags').clear();E.SubElement(s.find('subTags'),'item',type='ShowFrameTag')
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
r.set('frameCount','1')
for k,v in [('Xmin',0),('Ymin',0),('Xmax',40960),('Ymax',17920)]:r.find('displayRect').set(k,str(v))
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='164',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='-8000',translateY='12000');E.SubElement(ts,'item',type='ShowFrameTag')
E.ElementTree(r).write('assets/extracted/campaign-shop.xml')

