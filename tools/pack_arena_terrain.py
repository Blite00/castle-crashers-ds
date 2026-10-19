"""Losslessly pack the three arena terrain images; only the selected terrain is decoded into the caller's existing buffer."""
from pathlib import Path
from collections import defaultdict,deque
import ast,re
R=Path(__file__).resolve().parents[1]
module=ast.parse((R/'tools/convert_campaign_world.py').read_text())
exec(compile(ast.Module(body=[n for n in module.body if isinstance(n,ast.FunctionDef) and n.name in ('pack','unpack')],type_ignores=[]),'world_codec','exec'))
blob=bytearray();offsets=[0]
for filename,prefix in [('arena_background.h','arena'),('arena_king.h','king'),('arena_volcano.h','volcano')]:
 text=(R/'generated'/filename).read_text();match=re.search(r'static const uint8_t '+prefix+r'_composed_pixels\[\d+\]=\{(.*?)\};',text,re.S)
 data=bytes(map(int,re.findall(r'\d+',match.group(1))));assert len(data)==424*318
 encoded=pack(data);assert unpack(encoded)==data
 blob.extend(encoded);offsets.append(len(blob))
h='#ifndef ARENA_TERRAIN_PACKED_H\n#define ARENA_TERRAIN_PACKED_H\n#include <stdint.h>\n'
h+='static const unsigned arena_terrain_offsets[]={'+','.join(map(str,offsets))+'};\n'
h+='static const uint8_t arena_terrain_packed_data[]={\n'+''.join(','.join(map(str,blob[i:i+32]))+',\n' for i in range(0,len(blob),32))+'};\n'
h+='''#include \"asset_inflate.h\"
static inline int decode_arena_terrain(unsigned map,uint8_t *out){
 if(map>=3||!out)return 0;
 unsigned off=arena_terrain_offsets[map];
 return asset_inflate(arena_terrain_packed_data+off,arena_terrain_offsets[map+1]-off,out,424*318);
}
#endif
'''
(R/'generated/arena_terrain_packed.h').write_text(h)
print('Three lossless terrains:',len(blob),'bytes; shared decode buffer134832; RAM saved',3*134832-len(blob))

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
