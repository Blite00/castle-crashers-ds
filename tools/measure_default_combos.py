from pathlib import Path
import ast,pickle,zlib
import numpy as np
from PIL import Image
R=Path.cwd();b=pickle.loads((R/'build/held-base.pkl').read_bytes());pal=b['palette'];canonical={};remap=[];q=Image.new('P',(1,1));rgb=[]
for i,c in enumerate(pal):
 if c not in canonical:canonical[c]=i
 remap.append(canonical[c]);cc=pal[1] if not i else c;rgb.extend(((cc&31)*255//31,((cc>>5)&31)*255//31,((cc>>10)&31)*255//31))
q.putpalette(rgb)
p=Path('tools/convert_assets.py');tree=ast.parse(p.read_text());ns={};exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef)],type_ignores=[]),str(p),'exec'),ns)
data=[];meta=[];raws=[];seen={}
for color in ('blue','orange','red','green'):
 for weapon in (1,2,9,19,10,38,30,37,42,54,55,62):
  for n in range(1,37):
   im=Image.open(R/f'assets/default-combo-{color}-{weapon}/{n}.png').convert('RGBA').resize((192,120),Image.Resampling.LANCZOS);a=np.array(im.getchannel('A'))>=100;v=np.array(im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE));v[v==0]=1;v[~a]=0;v=np.take(remap,v).astype(np.uint8);ys,xs=np.where(v);x,y,r,bb=int(xs.min()),int(ys.min()),int(xs.max())+1,int(ys.max())+1;raw=v[y:bb,x:r].tobytes();key=(r-x,bb-y,raw)
   if key not in seen:seen[key]=len(data);data.extend(ns['pack'](raw))
   meta.append((seen[key],x-96,y-88,r-x,bb-y));raws.append(raw)
delta,mm,bases=ns['knight_color_delta'](data,meta,36);packed,mm,lens,tables,trees=ns['knight_huffman_encode'](delta,mm)
print('unique',len(seen),'RLEraw',len(data),'deltaRLE',len(delta),'huffman',len(packed),'withtablesmetadata',len(packed)+576*16+3072,'maxpixels',max(f[3]*f[4] for f in meta))
(R/'build/default-combos.pkl').write_bytes(pickle.dumps(dict(meta=meta,raws=raws,palette=pal,data=data)))
