from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import sys
sys.path.insert(0,'tools')
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
spec=[]
for item in (9,82):
 for state in (2,3,5,6,7):spec.append((item,state))

r=E.parse(R/'assets/extracted/level19.xml').getroot();ts=r.find('tags');d={int(t.get('spriteId')):t for t in ts if t.get('spriteId')};orig140=deepcopy(d[140]);orig106=deepcopy(d[106]);price=deepcopy(next(t for t in ts if t.get('characterID')=='121'))
for id,t in d.items():freeze(t,3 if id==129 else 1)
# Replace source store's generic wooden weapon with the actual purchased PLAYER weapon2.
player=E.parse(R/'assets/extracted/player.xml').getroot();defs={}
for t in player.find('tags'):
 for key in ('spriteId','shapeId','characterID','fontId','bitmapId'):
  if t.get(key) and t.get('type','').startswith('Define'):defs[int(t.get(key))]=(key,t);break
weapon=deepcopy(defs[243][1]);freeze(weapon,2);defs[243]=('spriteId',weapon);needed=set()
def gather(id):
 if id in needed or id not in defs:return
 needed.add(id)
 for child in defs[id][1].iter():
  if child.get('characterId'):gather(int(child.get('characterId')))
gather(243)
for id in needed:
 key,t=defs[id];t=deepcopy(t);t.set(key,str(id+3000))
 for child in t.iter():
  if child.get('characterId') and int(child.get('characterId')) in needed:child.set('characterId',str(int(child.get('characterId'))+3000))
 ts.append(t)
body=d[105].find('subTags');body.clear()
p=E.SubElement(body,'item',type='PlaceObject2Tag',characterId='3243',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(body,'item',type='ShowFrameTag')
for t in list(ts):
 if not t.get('type','').startswith('Define'):ts.remove(t)
for k,(item,state) in enumerate(spec):
 board=deepcopy(orig140);board.set('spriteId',str(1000+k));freeze(board,state)
 loot=deepcopy(orig106);loot.set('spriteId',str(1200+k));freeze(loot,item)
 text=deepcopy(price);text.set('characterID',str(1400+k));text.set('initialText','12' if item==9 else '32')
 for p in board.find('subTags'):
  if p.get('characterId')=='106':p.set('characterId',str(1200+k))
  if p.get('characterId')=='121':p.set('characterId',str(1400+k))
 ts.extend((board,loot,text))
for key,v in [('Xmin',-1600),('Ymin',-3200),('Xmax',2400),('Ymax',800)]:r.find('displayRect').set(key,str(v))
for k in range(len(spec)):
 E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1')
 p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(1000+k),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag')
r.set('frameCount',str(len(spec)));E.ElementTree(r).write(R/'assets/extracted/shop-boards.xml')
print(spec)

