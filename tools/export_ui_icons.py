from pathlib import Path
import subprocess,os
R=Path(__file__).resolve().parents[1];env=os.environ.copy();env['APPDATA']=str(R/'build/appdata');xml=R/'assets/extracted/ui-attribute-icons.xml';swf=xml.with_suffix('.swf')
for args in [['-xml2swf',str(xml),str(swf)],['-ignorebackground','-export','frame',str(R/'assets/ui-attribute-icons'),str(swf)]]:
 p=subprocess.run(['java','-Xmx2g','-jar',__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar')]+args,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);assert p.returncode==0,p.stdout.decode(errors='replace')[-1500:]
print('Exported original MENU Strength/Defense/Magic/Agility shapes367/366/364/365')
