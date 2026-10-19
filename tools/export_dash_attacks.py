from pathlib import Path
import subprocess,os,concurrent.futures
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');jar=__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')
def go(pair):
 c,w=pair;n=f'dash-attack-{c}-{w}';xml=R/f'assets/extracted/{n}.xml';swf=xml.with_suffix('.swf');out=R/f'assets/{n}'
 if len(list(out.glob('*.png')))>=24:return
 for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(out),str(swf)]]:
  p=subprocess.run(['java','-jar',jar]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1000:]
 print(n,len(list(out.glob('*.png'))),flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as ex:list(ex.map(go,[(c,w) for c in ('blue','orange','red','green') for w in (1,2,9,19,10,38,30,37,42)]))
