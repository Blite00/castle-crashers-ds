import xml.etree.ElementTree as E
r=E.parse('assets/extracted/player.xml').getroot();ts=r.find('tags');frame=1;active={};label=''
for t in ts:
 typ=t.get('type')
 if typ=='FrameLabelTag':label=t.get('name','');
 if typ.startswith('PlaceObject') and t.get('placeFlagHasCharacter')=='true':active[t.get('depth')]=t.get('characterId')
 elif typ.startswith('RemoveObject'):active.pop(t.get('depth'),None)
 elif typ=='ShowFrameTag':
  if label in ['stand','sleep','getup','roll']:print(frame,label,active)
  label='';frame+=1
