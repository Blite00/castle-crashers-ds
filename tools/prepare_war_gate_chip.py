"""Original FX door_shrapnel (FX frame 112, sprite 1180): four wood chip shapes."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/fx.xml').getroot();ts=r.find('tags')
chip=next(t for t in ts if t.get('type')=='DefineSpriteTag' and t.get('spriteId')=='1180')
copies=[]
for k in range(1,5):
    c=deepcopy(chip);c.set('spriteId',str(30000+k));freeze(c,k);copies.append(c)
for t in list(ts):
    if not t.get('type','').startswith('Define'):ts.remove(t)
for c in copies:ts.append(c)
for k in range(1,5):
    p=E.SubElement(ts,'item',type='PlaceObject2Tag',characterId=str(30000+k),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',forceWriteAsLong='true')
    E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',nScaleBits='0',nRotateBits='0',nTranslateBits='16',scaleX='0.0',scaleY='0.0',rotateSkew0='0.0',rotateSkew1='0.0',translateX='400',translateY='400')
    E.SubElement(ts,'item',type='ShowFrameTag');E.SubElement(ts,'item',type='RemoveObject2Tag',depth='1')
for kk,v in [('Xmin',0),('Ymin',0),('Xmax',800),('Ymax',800)]:r.find('displayRect').set(kk,str(v))
r.set('frameCount','4');E.ElementTree(r).write(R/'assets/extracted/war-gate-chip.xml');print('door_shrapnel chips: 4')
