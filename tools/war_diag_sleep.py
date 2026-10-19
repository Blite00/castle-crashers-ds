import xml.etree.ElementTree as E
r=E.parse('assets/extracted/player.xml').getroot()
for sid in ['1886','1885']:
 s=next(x for x in r.find('tags') if x.get('spriteId')==sid); print('SPRITE',sid,'frames',s.get('frameCount'));frame=1
 for t in s.find('subTags'):
  typ=t.get('type')
  if typ=='ShowFrameTag':frame+=1
  elif typ.startswith('PlaceObject') and t.get('placeFlagHasCharacter')=='true':print('character',frame,t.get('characterId'),'depth',t.get('depth'))
  elif typ=='FrameLabelTag':print('label',frame,t.get('name'))
  elif typ=='DoActionTag':print('action',frame,repr(bytes.fromhex(t.get('actionBytes','')))[:110])
r=E.parse('assets/extracted/level11-full.xml').getroot()
for t in r.iter():
 if t.get('name') in ['bed1','bed2','bed3','bed4']:
  m=t.find('matrix');print(t.get('name'),t.get('characterId'),m.get('translateX'),m.get('translateY'))
