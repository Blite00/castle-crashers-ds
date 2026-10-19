import re,zlib
from pathlib import Path
R=Path(__file__).resolve().parents[1]
s=(R/'generated/campaign_world.h').read_text()
def arr(name):return list(map(int,re.findall(r'\d+',re.search(r'\b'+name+r'\[[^;]*?\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
d=bytes(arr('campaign_world_row_data'));off=arr('campaign_world_row_offsets');pix=bytearray(2448*448)
for strip in range(77):
 for c in range(7):
  k=strip*7+c;raw=zlib.decompress(d[off[k]:off[k+1]],-15)
  for y in range(64):
   n=min(32,2448-strip*32);start=(c*64+y)*2448+strip*32;pix[start:start+n]=raw[y*32:y*32+n]
fg=bytes(arr('campaign_world_fg_data'));m=re.search(r'campaign_world_fg_layers\[\]\s*=\s*\{(.*?)\};',s,re.S);layers=[tuple(map(int,row.split(','))) for row in re.findall(r'\{([^}]+)\}',m[1])]
for x,y,w,h,depth,o,n in layers:
 raw=zlib.decompress(fg[o:o+n],-15)
 for yy in range(h):
  for xx in range(w):
   if raw[yy*w+xx]:pix[(y+yy)*2448+x+xx]=raw[yy*w+xx]
blob=bytearray();ofs=[0]
for strip in range(77):
 for c in range(7):
  raw=bytearray()
  for y in range(64):
   n=min(32,2448-strip*32);start=(c*64+y)*2448+strip*32;line=pix[start:start+n];raw.extend(line);raw.extend([line[-1]]*(32-n))
  blob.extend(zlib.compress(raw,9)[2:-4]);ofs.append(len(blob))

def replace_array(text,name,values):
 m=re.search(r'((?:static )?const \w+ '+name+r')\[[^]]*\]\s*=\s*\{.*?\};',text,re.S)
 assert m,name
 return text[:m.start()]+m[1]+'['+str(len(values))+']={\n'+','.join(map(str,values))+'\n};'+text[m.end():]
s=replace_array(s,'campaign_world_row_data',blob)
s=replace_array(s,'campaign_world_row_offsets',ofs)
(R/'generated/campaign_world.h').write_text(s)
(R/'assets/campaign-world-reference.bin').write_bytes(pix)
print('Exact foreground prebake bytes saved',len(d)-len(blob))

import json,hashlib
metadata=R/'assets/campaign-world-metadata.json'
if metadata.exists():
 info=json.loads(metadata.read_text());info['terrain_bytes']=len(blob);info['terrain_sha256']=hashlib.sha256(pix).hexdigest();info['foreground_baked_exact']=True;metadata.write_text(json.dumps(info,indent=2))

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
