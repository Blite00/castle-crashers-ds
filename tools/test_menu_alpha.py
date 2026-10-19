"""Independent Python decode/hash of original coverage, then native blend+guard tests."""
from pathlib import Path
import re,hashlib,subprocess,os
R=Path(__file__).resolve().parents[1];os.chdir(R);(R/'build').mkdir(exist_ok=True)
s=(R/'generated/menu_art.c').read_text()
for name,expected in [('menu_alpha','1ff712cb7a7360d23a835db54ff6c98924d6bb7a160fd2b786c8a28ffa1fa451'),('menu_font_alpha','bbd06f81f481ad33a891d6b2264d3dbfd891e1510eb468a344a38b18e1e2da3a')]:
 m=re.search(r'const uint8_t '+name+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);a=bytes(map(int,re.findall(r'\d+',m[1])));raw=bytearray();i=0
 while i<len(a):
  t=a[i];i+=1;n=(t&127)+1
  if t&128:raw.extend([a[i]]*n);i+=1
  else:raw.extend(a[i:i+n]);i+=n
 assert hashlib.sha256(raw).hexdigest()==expected
 (R/f'build/{name}-raw.bin').write_bytes(raw)
gcc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(gcc.parent)+os.pathsep+env['PATH']
subprocess.run([str(gcc),'-O2','-std=c11','-Wall','-Wextra','-Werror','-Igenerated','tests/menu_alpha_codec_test.c','generated/menu_art.c','-o','build/menu-alpha-test.exe'],check=True,env=env)
subprocess.run([str(R/'build/menu-alpha-test.exe')],check=True,env=env)
