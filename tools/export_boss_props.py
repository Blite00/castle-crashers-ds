from pathlib import Path
import subprocess,os,json
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
for name,root,count,frames in json.loads((R/'assets/boss-actor-spec.json').read_text())[7:]:
 n='boss-prop-'+name;xml=R/f'assets/extracted/{n}.xml';swf=xml.with_suffix('.swf');out=R/f'assets/{n}'
 for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(out),str(swf)]]:
  p=subprocess.run(['java','-Xmx2g','-jar',jar]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
 target=R/f'assets/boss-actor-{name}';target.mkdir(exist_ok=True)
 for i,f in enumerate(frames): (target/f'{f}.png').write_bytes((out/f'{i+1}.png').read_bytes())
 print(name,len(frames),flush=True)
