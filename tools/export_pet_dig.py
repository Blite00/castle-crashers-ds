"""Export genuine ANIMALS and PLAYER1102 using isolated FFDec inputs."""
from pathlib import Path
import xml.etree.ElementTree as E
import subprocess,os,concurrent.futures
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
COLORS=('blue','orange','red','green')
def prepare(name,source,poses,animal=None):
 r=E.parse(R/source).getroot();tags=r.find('tags');defs=[t for t in tags if t.get('type','').startswith('Define')]
 for t in defs:
  if animal and t.get('spriteId')=='50':freeze(t,animal)
  if animal and t.get('spriteId')=='75':freeze(t,5)
 tags.clear();tags.extend(defs)
 for sid,count in poses:
  E.SubElement(tags,'item',type='RemoveObject2Tag',depth='1',forceWriteAsLong='false')
  p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId=str(sid),depth='1',forceWriteAsLong='false',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false',placeFlagHasClipActions='false',placeFlagHasClipDepth='false',placeFlagHasColorTransform='false',placeFlagHasName='false',placeFlagHasRatio='false',ratio='0',clipDepth='0')
  E.SubElement(p,'matrix',type='MATRIX',hasRotate='false',hasScale='false',translateX='0',translateY='0',rotateSkew0='0.0',rotateSkew1='0.0',scaleX='1.0',scaleY='1.0',nTranslateBits='1',nRotateBits='0',nScaleBits='0')
  for _ in range(count):E.SubElement(tags,'item',type='ShowFrameTag',forceWriteAsLong='false')
 r.set('frameCount',str(sum(n for _,n in poses)))
 rect=r.find('displayRect');rect.set('Xmin','-3840');rect.set('Xmax','3840');rect.set('Ymin','-3520');rect.set('Ymax','1280')
 E.ElementTree(r).write(R/f'assets/extracted/{name}.xml',encoding='utf-8',xml_declaration=True)
 return name

def export(name):
 env=os.environ.copy();env['APPDATA']=str(R/'build/pet-art-appdata')
 xml=R/f'assets/extracted/{name}.xml';swf=xml.with_suffix('.swf');out=R/f'assets/{name}'
 for args in (['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(out),str(swf)]):
  p=subprocess.run(['java','-jar',__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
  assert p.returncode==0,p.stdout.decode(errors='replace')[-2000:]
 print(name,len(list(out.glob('*.png'))),flush=True)
if __name__=='__main__':
 jobs=[prepare(f'pet-common-{pet}','assets/extracted/pet-common-animals.xml',[(51,14),(60,49)]+([(75,16)] if pet==24 else []),pet) for pet in (17,24)]
 jobs += [prepare(f'dig-common-{color}',f'assets/extracted/{color}.xml',[(1102,76)]) for color in COLORS]
 with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(export,jobs))
