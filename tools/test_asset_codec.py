"""Validate generated DEFLATE decoder against Python zlib, including bounded failures."""
from pathlib import Path
import os,random,struct,subprocess,zlib
R=Path(__file__).resolve().parents[1]
rng=random.Random(41);cases=[]
for n in [0,1,2,3,4,32,511,14336,134832]:
 for kind in range(3):
  data=bytes(rng.randrange(256) for _ in range(n)) if kind==0 else bytes([kind])*n if kind==1 else bytes(i%129 for i in range(n))
  for level,strategy in [(9,zlib.Z_DEFAULT_STRATEGY),(9,zlib.Z_FIXED),(0,zlib.Z_DEFAULT_STRATEGY)]:
   c=zlib.compressobj(level,wbits=-15,strategy=strategy);cases.append((c.compress(data)+c.flush(),data))
(R/'build').mkdir(exist_ok=True)
with (R/'build/inflate-cases.bin').open('wb') as f:
 for enc,data in cases:f.write(struct.pack('<II',len(enc),len(data))+enc+data)
cc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(cc.parent)+os.pathsep+env.get('PATH','')
subprocess.run([str(cc),'-std=c11','-O2','-Wall','-Wextra','-Werror','-fstack-usage','-Itools','tools/asset_codec_test.c','-o','build/asset-codec-test.exe'],cwd=R,env=env,check=True)
subprocess.run([str(R/'build/asset-codec-test.exe')],cwd=R,env=env,check=True)
