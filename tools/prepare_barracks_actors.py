from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E,json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
S=[('trainer',114,[1]),('drums',150,[3,10,17]),('guitar',184,[3,10,17]),('cello',202,[3,10,17]),('drinker',203,[1,122]),('barrel',165,[2,3,15,35]),('wounded',366,list(range(161,390,2)))]
for name,sid,frames in S:
 r=E.parse(R/'assets/extracted/level2.xml').getroot();ts=r.find('tags');d={int(t.get('spriteId')):t for t in ts if t.get('spriteId')};active=set()
 def descend(n):
  if n in active:return
  active.add(n)
  if n in d:
   for p in d[n].find('subTags'):
    if p.get('characterId') not in (None,'0'):descend(int(p.get('characterId')))
 descend(sid)
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
  elif t.get('spriteId'):
   n=int(t.get('spriteId'))
   if n in (13,16,59,84,329):t.find('subTags').clear();E.SubElement(t.find('subTags'),'item',type='ShowFrameTag');t.set('frameCount','1')
   elif n not in active:freeze(t,1)
 if sid in (150,184,202):
  freeze(d[sid],1)
  for _ in range(16):E.SubElement(d[sid].find('subTags'),'item',type='ShowFrameTag')
  d[sid].set('frameCount','17')
 if name=='drinker':
  # Source classic branch stops these skin selectors at frame1.
  for selector in (126,125,129):freeze(d[selector],1)
 if name=='wounded':
  for selector in (207,332,338,344,347):freeze(d[selector],6)
 for p in list(d[366].find('subTags')):
  if p.get('characterId') in ('307','308','310','311','312','313'):d[366].find('subTags').remove(p)
 w,h,x,y=(2560,896,8921,11349) if name=='wounded' else (256,192,2560,3200)
 # Wounded localmovie origin(-53.95,-146.55) offset by roomorigin(-500,-714).
 for k,v in [('Xmin',0),('Ymin',0),('Xmax',w*20),('Ymax',h*20)]:r.find('displayRect').set(k,str(v))
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX=str(x),translateY=str(y))
 for _ in range(max(frames)):E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount',str(max(frames)));E.ElementTree(r).write(R/f'assets/extracted/barracks-{name}.xml')
(R/'assets/barracks-actors-spec.json').write_text(json.dumps(S))
