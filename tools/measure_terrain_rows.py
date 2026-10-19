from pathlib import Path
import re,zlib,pickle
R=Path(__file__).resolve().parents[1];maps={}
for name in ('world','shop','barracks'):
 s=(R/f'generated/campaign_{name}.h').read_text();prefix='campaign_'+name
 def nums(n):return list(map(int,re.findall(r'\d+',re.search(r'\b'+n+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
 offs=nums(prefix+'_strip_offsets');blob=bytes(nums(prefix+'_strip_data'));strips=[zlib.decompress(blob[a:b],-15) for a,b in zip(offs,offs[1:])];maps[name]=strips
 print(name,'old',len(blob),end=' ')
 for rows in (16,32,64):
  blocks=[raw[y*32:(y+rows)*32] for raw in strips for y in range(0,448,rows)];size=sum(len(zlib.compress(b,9))-6 for b in blocks)+4*(len(blocks)+1);print(rows,size,'delta',size-len(blob),end='; ')
 print()
(R/'build/terrain-original-strips.pkl').write_bytes(pickle.dumps(maps))
