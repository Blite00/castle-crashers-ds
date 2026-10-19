from pathlib import Path
import xml.etree.ElementTree as E,sys
sys.path.insert(0,'tools');from prepare_frames import freeze
R=Path('.')
for name,frame,count in [('closed',1,1),('open',14,32)]:
 r=E.parse('assets/extracted/level2.xml').getroot();tags=r.find('tags');d={int(t.get('spriteId')):t for t in tags if t.get('spriteId')}
 for t in list(tags):
  if not t.get('type','').startswith('Define'):tags.remove(t)
 freeze(d[317],frame)
 for _ in range(count-1):E.SubElement(d[317].find('subTags'),'item',type='ShowFrameTag')
 d[317].set('frameCount',str(count))
 p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId='317',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='43558',translateY='7010')
 for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag')
 r.set('frameCount',str(count))
 for k,v in [('Xmin',0),('Ymin',0),('Xmax',51200),('Ymax',17920)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(f'assets/extracted/barracks-door-{name}.xml')
