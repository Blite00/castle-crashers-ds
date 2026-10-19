from pathlib import Path
import xml.etree.ElementTree as E
r=E.parse('assets/extracted/fx.xml').getroot();ts=r.find('tags');s=next(t for t in ts if t.get('spriteId')=='41');print('dust41 frames',s.get('frameCount'))
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for k,v in [('Xmin',-1280),('Xmax',1280),('Ymin',-1920),('Ymax',640)]:r.find('displayRect').set(k,str(v))
p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId='41',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
for i in range(14):E.SubElement(ts,'item',type='ShowFrameTag')
r.set('frameCount','14');E.ElementTree(r).write('assets/extracted/run-dust.xml')
