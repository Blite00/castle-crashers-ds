"""Pack mutually exclusive original art bytes into validated chapter banks.

Production headers retain exact array types/sizeof through fixed bank views.
Native reference builds keep original initializers unless CHAPTER_ASSETS_TEST.
"""
from pathlib import Path
import re,struct,zlib,json,argparse
R=Path(__file__).resolve().parents[1]
BANKS=[('arenas',[
 ('arena_terrain_packed.h','arena_terrain_packed_data'),('arena_occlusion_packed.h','arena_occlusion_packed_data'),
 ('arena_background.h','arena_layer_pixels'),('arena_king.h','king_layer_pixels'),('arena_volcano.h','volcano_layer_pixels')]),
 ('keep',[
 ('campaign_world.h','campaign_world_row_data'),('campaign_world.h','campaign_world_fg_data'),
 ('campaign_shop.h','campaign_shop_row_data'),('campaign_barracks.h','campaign_barracks_row_data'),
 ('barracks_actor_art.h','barracks_actor_pixels'),('barracks_choreography.h','barracks_exit_pixels'),
 ('barracks_door_art.h','barracks_door_pixels'),('campaign_breakables.h','campaign_breakable_data'),
 ('cinematic_art.h','cinema_bg_pixels'),('cinematic_art.h','cinema_pixels'),
 ('neutral_npc_art.h','neutral_npc_pixels'),('shop_board_art.h','shop_board_pixels'),('dance_art.h','dance_pixels')]),
 ('war',[('war_art.h','campaign_war_row_data'),('war_art.h','campaign_war_fg_data'),('bridge_art.h','campaign_bridge_row_data'),('bridge_art.h','campaign_bridge_fg_data'),('war_machine_art.h','war_machine_data'),('war_alternate_art.h','war_alternate_data'),('thief_art.h','thief_pixels'),('war_live_art.h','war_live_pixels'),('war_cannon_art.h','war_cannon_data')]),
 ('hubs',[('blacksmith_art.h','campaign_blacksmith_row_data'),('blacksmith_art.h','campaign_blacksmith_fg_data'),('frog_art.h','campaign_frog_row_data'),('frog_art.h','campaign_frog_fg_data'),('ark_art.h','campaign_ark_row_data'),('ark_art.h','campaign_ark_fg_data'),('original_worldmap.h','original_worldmap_data'),('hub_ambient_art.h','hub_ambient_pixels')]),
 ('boss',[('boss_art.h','campaign_boss_row_data'),('boss_art.h','campaign_boss_fg_data'),('boss_actor_art.h','boss_actor_data'),('buried_art.h','buried_bank_data'),('boss_door_art.h','boss_door_pixels'),('boss_sfx.h','boss_sfx_data')]),
 ('forest',[('forest_croc_art.h','forest_croc_data'),('rider_art.h','rider_pixels'),('forest_art.h','campaign_forest_row_data'),('forest_art.h','campaign_forest_fg_data'),('forest_thief_art.h','forest_thief_pixels'),('forest_props_art.h','forest_prop_data'),('forest_props_art.h','forest_prop_tiles_data'),('forest_props_art.h','forest_prop_frames_data'),('forest_props_art.h','forest_prop_palette_data'),('forest_art.h','campaign_forest_palette_data'),('forest_art.h','campaign_forest_row_offsets_data'),('forest_art.h','campaign_forest_fg_layers_data')]),
 ('thieves',[('thieves_art.h','campaign_thieves_row_data'),('thieves_art.h','campaign_thieves_fg_data'),('thieves_art.h','campaign_thieves_palette_data'),('thieves_art.h','campaign_thieves_row_offsets_data'),('thieves_art.h','campaign_thieves_fg_layers_data'),('thieves_thief_art.h','thieves_thief_pixels'),('troll_art.h','troll_pixels'),('imp_art.h','imp_pixels'),('mother_art.h','mother_pixels'),('thieves_scene_art.h','thieves_scene_pixels'),('thieves_sfx.h','thieves_sfx_data')]),
 ('deer',[('deer_art.h','campaign_deer_row_data'),('deer_art.h','campaign_deer_fg_data'),('deer_art.h','campaign_deer_palette_data'),('deer_art.h','campaign_deer_row_offsets_data'),('deer_art.h','campaign_deer_fg_layers_data'),('deer_actor_art.h','deer_actor_pixels'),('deer_sfx.h','deer_sfx_data')]),
 ('flood',[('flood_art.h','campaign_flood_row_data'),('flood_art.h','campaign_flood_fg_data'),('flood_art.h','campaign_flood_palette_data'),('flood_art.h','campaign_flood_row_offsets_data'),('flood_art.h','campaign_flood_fg_layers_data'),('flood_actor_art.h','flood_actor_pixels'),('flood_sfx.h','flood_sfx_data')]),
 ('camp',[('camp_art.h','campaign_camp_row_data'),('camp_art.h','campaign_camp_fg_data'),('camp_art.h','campaign_camp_palette_data'),('camp_art.h','campaign_camp_row_offsets_data'),('camp_art.h','campaign_camp_fg_layers_data'),('camp_geometry.h','camp_floor_runs_data'),('bear_art.h','bear_pixels'),('bear_art.h','bear_palette_data'),('bear_art.h','bear_frames_data'),('camp_actor_art.h','camp_actor_pixels'),('camp_actor_art.h','camp_actor_palette_data'),('camp_actor_art.h','camp_actor_frames_data')])]
for _name,_entries in BANKS:_entries.append(('pet_extra_art.h','pet_extra_'+_name)) # every chapter carries the three extra pets' art
def legacy(s):
 return re.sub(r'/\* CHAPTER_ASSET_BEGIN (\w+) \*/\n#if.*?\n#else\n(.*?)\n#endif\n/\* CHAPTER_ASSET_END \1 \*/',lambda m:m[2],s,flags=re.S)
def build(check=False,force=False):
 if not check and not force and __import__('os').environ.get('CC_DEFER_CHAPTER_BANKS'):return  # setup pipeline packs once at the end
 loaded={};manifest=[];banks=[];wrappers={}
 for bankid,(bankname,entries) in enumerate(BANKS):
  blob=bytearray();chunks=[]
  for filename,name in entries:
   if filename not in loaded:loaded[filename]=legacy((R/'generated'/filename).read_text())
   pattern=r'static const uint8_t\s+'+re.escape(name)+r'\s*\[(\d*)\]\s*(?:__attribute__\(\(aligned\(4\)\)\)\s*)?=\s*\{(.*?)\};'
   m=re.search(pattern,loaded[filename],re.S)
   if not m:raise RuntimeError('Missing uint8 array '+name)
   vals=list(map(int,re.findall(r'\d+',m[2])));assert not m[1] or int(m[1])==len(vals);data=bytes(vals)
   blob.extend(b'\0'*((-len(blob))&3));offset=len(blob);blob.extend(data)
   macro='CHAPTER_OFFSET_'+name.upper();manifest.append(f'#define {macro} {offset}u\n#define CHAPTER_LENGTH_{name.upper()} {len(data)}u\n')
   wrap=f'/* CHAPTER_ASSET_BEGIN {name} */\n#if defined(ARM9) || defined(CHAPTER_ASSETS_TEST)\n#include "chapter_assets.h"\n#include "chapter_asset_manifest.h"\n#define {name} (*(const uint8_t (*)[{len(data)}])(chapter_asset_buffer+{macro}))\n#else\n'+m[0]+f'\n#endif\n/* CHAPTER_ASSET_END {name} */'
   wrappers.setdefault(filename,[]).append((m[0],wrap));chunks.append(dict(name=name,offset=offset,size=len(data),sha256=__import__('hashlib').sha256(data).hexdigest()))
  blob.extend(b'\0'*((-len(blob))&3));crc=zlib.crc32(blob);assert len(blob)<=1040000
  header=struct.pack('<8sIIIIII',b'CCDSBNK1',1,bankid,len(blob),crc,0,0);assert len(header)==32
  p=R/f'nitrofiles/chapters/{bankname}.bin';content=header+blob
  if check:assert p.read_bytes()==content,bankname+' bank is stale'
  else:p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(content)
  banks.append(dict(id=bankid,name=bankname,size=len(blob),crc32=crc,chunks=chunks));print(bankname,len(blob),hex(crc))
 h='#ifndef CHAPTER_ASSET_MANIFEST_H\n#define CHAPTER_ASSET_MANIFEST_H\n#define CHAPTER_ASSET_BANK_COUNT '+str(len(BANKS))+'u\n'+''.join(manifest)
 h+='static const unsigned chapter_asset_bank_sizes['+str(len(BANKS))+']={'+','.join(str(b['size'])+'u' for b in banks)+'};\n'
 h+='static const unsigned chapter_asset_bank_crcs['+str(len(BANKS))+']={'+','.join(str(b['crc32'])+'u' for b in banks)+'};\n#endif\n'
 if not check:
  (R/'generated/chapter_asset_manifest.h').write_text(h)
  (R/'generated/chapter-asset-audit.json').write_text(json.dumps(dict(capacity=1040000,header_bytes=32,banks=banks),indent=2))
  for filename,changes in wrappers.items():
   s=loaded[filename]
   for before,after in changes:assert s.count(before)==1;s=s.replace(before,after)
   (R/'generated'/filename).write_text(s)
 else:assert (R/'generated/chapter_asset_manifest.h').read_text()==h
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args();build(args.check,force=True)
