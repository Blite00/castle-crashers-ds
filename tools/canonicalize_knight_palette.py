"""Losslessly canonicalize duplicate RGB555 knight palette indices."""
from pathlib import Path
import ast,pickle,re,sys,subprocess
R=Path(__file__).resolve().parents[1]
def optimize():
 subprocess.run([sys.executable,str(R/'tools/cache_held_base.py')],check=True)
 b=pickle.loads((R/'build/held-base.pkl').read_bytes());p=R/'tools/convert_assets.py';tree=ast.parse(p.read_text());ns={};exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef)],type_ignores=[]),str(p),'exec'),ns)
 canonical={};remap=[]
 for i,c in enumerate(b['palette']):
  if c not in canonical:canonical[c]=i
  remap.append(canonical[c])
 data=[];meta=[];seen={}
 for f,raw in zip(b['frames'],b['pixels']):
  merged=bytes(remap[v] for v in raw);assert all(b['palette'][x]==b['palette'][y] for x,y in zip(raw,merged));key=(f[3],f[4],merged)
  if key not in seen:seen[key]=len(data);data.extend(ns['pack'](merged))
  meta.append((seen[key],*f[1:]))
 data,meta,bases=ns['knight_color_delta'](data,meta,304);data,meta,lengths,tables,trees=ns['knight_huffman_encode'](data,meta)
 h=R/'generated/castle_art.h';s=h.read_text();old=int(re.search(r'knight_pixels\[(\d+)\]',s)[1]);start=s.index('static const uint8_t knight_pixels');end=s.index('/* Returns zero',start)
 replacement=ns['array']('knight_pixels','uint8_t',data)+'static const ArtFrame knight_frames[]={\n'+''.join('{'+','.join(map(str,f))+'},\n' for f in meta)+'};\n'+ns['knight_codec_tables'](ns['array'],lengths,tables,trees)+ns['array']('knight_frame_bases','uint16_t',bases)
 h.write_text(s[:start]+replacement+s[end:]);print('Lossless knight palette data',old,'->',len(data),'saved',old-len(data))
if __name__=='__main__':optimize()
