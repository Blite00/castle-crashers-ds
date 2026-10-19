from pathlib import Path
import subprocess,os,concurrent.futures
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def go(name):
 n='boss-actor-'+name;xml=R/f'assets/extracted/{n}.xml';swf=xml.with_suffix('.swf');out=R/f'assets/{n}'
 for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(out),str(swf)]]:
  p=subprocess.run(['java','-Xmx2g','-jar',jar]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
 print(n,len(list(out.glob('*.png'))),flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as ex:list(ex.map(go,['idle','walk','punch','spikes','slam','fire','die']))
