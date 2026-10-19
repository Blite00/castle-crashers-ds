from pathlib import Path
import subprocess,os
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata')
for args in [['-xml2swf','assets/extracted/hub-ambient.xml','assets/extracted/hub-ambient.swf'],['-ignorebackground','-export','frame','assets/hub-ambient','assets/extracted/hub-ambient.swf']]:
 subprocess.run(['java','-jar',__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')]+args,env=env,check=True)
