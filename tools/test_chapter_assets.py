"""Exercise chapter format/reference identity and native runtime failure paths."""
from pathlib import Path
import subprocess,sys,os
R=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,str(R/'tools/pack_chapter_assets.py'),'--check'],check=True,cwd=R)
p=R/'build/chapter-test';p.mkdir(exist_ok=True);original=(R/'nitrofiles/chapters/keep.bin').read_bytes()
for name,offset in [('version',8),('wrong-bank',12),('size',16),('payload',200)]:
 b=bytearray(original);b[offset]^=1;(p/f'{name}.bin').write_bytes(b)
(p/'short.bin').write_bytes(original[:5]);(p/'extra.bin').write_bytes(original+b'X');(p/'truncated.bin').write_bytes(original[:-1])
gcc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(gcc.parent)+os.pathsep+env['PATH'];exe=R/'build/chapter-assets-test.exe'
subprocess.run([str(gcc),'-O2','-std=c11','-Wall','-Wextra','-Werror','-DCHAPTER_ASSETS_TEST','-Isource','-Igenerated','tests/chapter_assets_test.c','source/chapter_assets.c','-o',str(exe)],check=True,cwd=R,env=env)
subprocess.run([str(exe)],check=True,cwd=R,env=env)
