from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import os,subprocess
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1];r=E.parse(R/'assets/extracted/player.xml').getroot();tags=r.find('tags');defs=[t for t in tags if t.get('type','').startswith('Define')]
icon=next(t for t in defs if t.get('spriteId')=='305');popup=next(t for t in defs if t.get('spriteId')=='316')
tags.clear();tags.extend(defs)
for i,frame in enumerate([9,2]):
 s=deepcopy(icon);s.set('spriteId',str(3000+i));freeze(s,frame)
 for t in list(s.find('subTags')):
  if t.get('name') in ('healthpots','bombs','beefies'):s.find('subTags').remove(t)
 tags.append(s);s=deepcopy(popup);s.set('spriteId',str(3010+i))
 for t in list(s.find('subTags')):
  if t.get('characterId') in ('315','311'):s.find('subTags').remove(t)
  elif t.get('characterId')=='305':t.set('characterId',str(3000+i))
 tags.append(s);E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1')
 t=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(3010+i),depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
 E.SubElement(t,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='0',translateY='0')
 for j in range(9):E.SubElement(tags,'item',type='ShowFrameTag')
r.set('frameCount','18')
for k,v in dict(Xmin=-1600,Ymin=-1600,Xmax=1600,Ymax=1600).items():r.find('displayRect').set(k,str(v))
xml=R/'assets/extracted/item-popups.xml';E.ElementTree(r).write(xml,encoding='utf-8',xml_declaration=True)
env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
for args in [['-xml2swf',str(xml),str(xml.with_suffix('.swf'))],['-ignorebackground','-export','frame',str(R/'assets/item-popups'),str(xml.with_suffix('.swf'))]]:
 p=subprocess.run(['java','-Xmx2g','-jar',__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
print('Exported original potion/shovel selection pulses')
