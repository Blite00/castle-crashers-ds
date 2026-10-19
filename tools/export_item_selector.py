"""Export original PLAYER305 item icons and PLAYER269 selection pulse."""
from pathlib import Path
import xml.etree.ElementTree as E
from copy import deepcopy
import subprocess,os
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
r=E.parse(R/'assets/extracted/player.xml').getroot();tags=r.find('tags');defs=[t for t in tags if t.get('type','').startswith('Define')]
original=next(t for t in defs if t.get('spriteId')=='305')
for t in defs:
 if t.get('spriteId')=='289':freeze(t,3)
tags.clear();tags.extend(defs)
for i in range(19):
 src=original if i<11 else next(t for t in defs if t.get('spriteId')=='269')
 s=deepcopy(src);s.set('spriteId',str(3000+i));freeze(s,i+1 if i<11 else i-10)
 for t in list(s.find('subTags')):
  if t.get('name') in ('bombs','healthpots','beefies'):s.find('subTags').remove(t)
 tags.append(s);E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1')
 t=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(3000+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
 E.SubElement(t,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
 E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount','19')
for k,v in dict(Xmin=-960,Ymin=-960,Xmax=960,Ymax=960).items():r.find('displayRect').set(k,str(v))
xml=R/'assets/extracted/item-selector.xml';E.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
for args in [['-xml2swf',str(xml),str(xml.with_suffix('.swf'))],['-ignorebackground','-export','frame',str(R/'assets/item-selector'),str(xml.with_suffix('.swf'))]]:
 p=subprocess.run(['java','-Xmx2g','-jar',__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
print('Exported PLAYER305 icons and269 pulse')
