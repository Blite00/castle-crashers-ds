from pathlib import Path
import subprocess,os,json,concurrent.futures
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def go(s):
 n='war-live-'+s['name'];xml=R/f'assets/extracted/{n}.xml';swf=xml.with_suffix('.swf')
 for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(R/f'assets/{n}'),str(swf)]]:
  p=subprocess.run(['java','-Xmx2g','-jar',jar]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1000:]
 print(n,'exported',flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as e:list(e.map(go,json.loads((R/'assets/war-live-spec.json').read_text())))
