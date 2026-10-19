"""Bake source LEVEL7 prop keyframes while honoring held nested timeline ends."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1];source=E.parse(R/'assets/extracted/level7-full.xml').getroot()
spec=[('chest',227,4,1,[1]),('chest-open',227,6,320,[1,9,17,25,33,41,49,57,65,81,105,137,169,201,241,281,319]),('princess',245,0,33,[1,2,6,10,14,18,22,26,30,33]),('kiss',245,34,142,[1,9,17,25,33,49,65,81,97,113,129,141]),('carried',111,0,269,[1,12,22,44,60,80,100,120,140,160,180,200,220,240,260,269]),('post',126,1,1,[1])]
for name,sid,hold,count,samples in spec:
 r=deepcopy(source);ts=r.find('tags');defs={int(t.get('spriteId')):t for t in ts if t.get('type')=='DefineSpriteTag'}
 for t in list(ts):
  if not t.get('type','').startswith('Define'):ts.remove(t)
 for ident in (13,17,120,222):
  defs[ident].find('subTags').clear();E.SubElement(defs[ident].find('subTags'),'item',type='ShowFrameTag');defs[ident].set('frameCount','1')
 nextid=[500]
 def bake(ident,frame,root=False):
  if ident not in defs:return ident
  src=defs[ident];f=min(frame,int(src.get('frameCount')));elapsed=frame
  if ident in (63,66):f=1
  if root and hold:f=hold
  births={};now=1
  for t in src.find('subTags'):
   if t.get('type')=='ShowFrameTag':
    if now==f:break
    now+=1
   elif t.get('type','').startswith('Place') and t.get('placeFlagHasCharacter')=='true':births[t.get('depth')]=now
   elif t.get('type','').startswith('Remove'):births.pop(t.get('depth'),None)
  clone=deepcopy(src);freeze(clone,f);new=nextid[0];nextid[0]+=1;clone.set('spriteId',str(new))
  for t in clone.find('subTags'):
   if t.get('characterId'):
    childframe=(elapsed if root and hold else f)-births.get(t.get('depth'),1)+1
    if root and hold:childframe=elapsed
    t.set('characterId',str(bake(int(t.get('characterId')),max(1,childframe))))
  ts.append(clone);return new
 # One frozen key per output frame; exported filenames are remapped to source frame numbers.
 for sample in samples:
  ident=bake(sid,sample,True);E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1');p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(ident),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0');E.SubElement(ts,'item',type='ShowFrameTag')
 r.set('frameCount',str(len(samples)))
 for k,v in [('Xmin',-10240),('Xmax',10240),('Ymin',-12800),('Ymax',2560)]:r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/f'assets/extracted/boss-prop-{name}.xml')
p=R/'assets/boss-actor-spec.json';base=json.loads(p.read_text())[:7];base.extend([[n,f,c,s] for n,i,f,c,s in spec]);p.write_text(json.dumps(base,indent=2))
