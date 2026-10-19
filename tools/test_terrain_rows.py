"""Independent zlib references and native bounds tests for partial terrain decoding."""
from pathlib import Path
import os,re,zlib,subprocess
R=Path(__file__).resolve().parents[1]
for name in ('world','shop','barracks'):
 prefix='campaign_'+name;s=(R/f'generated/{prefix}.h').read_text()
 def values(n):return list(map(int,re.findall(r'\d+',re.search(r'\b'+n+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
 offsets=values(prefix+'_row_offsets');data=bytes(values(prefix+'_row_data'));raw=b''.join(zlib.decompress(data[a:b],-15) for a,b in zip(offsets,offsets[1:]));assert len(raw)%14336==0;(R/f'build/{prefix}-row-reference.bin').write_bytes(raw)
gcc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(gcc.parent)+os.pathsep+env['PATH']
subprocess.run([str(gcc),'-O2','-std=c11','-Wall','-Wextra','-Werror','-Igenerated','tests/terrain_rows_test.c','-o','build/terrain-rows-test.exe'],cwd=R,env=env,check=True)
subprocess.run([str(R/'build/terrain-rows-test.exe')],cwd=R,env=env,check=True)
