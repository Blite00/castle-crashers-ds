"""Additional original chapter music in NitroFS, never compiled into ARM9 arrays."""
import array,hashlib,json,subprocess
from pathlib import Path
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1];G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'music')
TRACKS=[('war_intro','INTRO',4000,False,'war'),('war_boss','MACHINER',4000,True,'war'),('blacksmith','NG86480',5512,True,'blacksmith'),('weapons','WEAPONFROG',5512,True,'weapons'),('worldmap','FLUTEY',8000,True,'worldmap'),('barbarian_boss','NG64061',4000,True,'boss'),('goldgrab','goldgrab',8000,True,'boss'),('boss_duel','NG21749',2700,True,'boss-duel'),('boss_victory','Waterflame_RaceVictory',8000,True,'boss-victory'),('smith_hammer','../sounds/sound_blacksmith',8000,False,'blacksmith'),('forest','Waterflame_Jumper',5512,True,'forest'),('deer','NG36657',3000,True,'deer'),('deer_victory','Waterflame_RaceVictory',5600,True,'deer-victory'),('thieves','NG57239',4400,True,'thieves'),('thieves_boss','Waterflame_TheShow',5512,True,'thieves-boss'),('flood','007_FLOODED_CASTLE',4000,True,'flood'),('flood_boss','015_CORN_BOSS',4000,True,'flood-boss')]
banks={};rows=[];audit=[]
for name,source,rate,loop,bank in TRACKS:
 path=G/(source+'.xma');raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(path),'-vn','-ac','1','-ar',str(rate),'-f','f32le','-']);samples=[x*32768 for x in array.array('f',raw)]
 fade=min(rate//50,len(samples)//2)
 for i in range(fade):samples[i]*=i/fade;samples[-i-1]*=i/fade
 data,decoded,gain=normalize_and_encode(speaker_compress(samples,rate));blob=banks.setdefault(bank,bytearray());blob.extend(bytes((-len(blob))%32));off=len(blob);blob.extend(data);rows.append((off,len(data),rate,int(loop)))
 audit.append(dict(name=name,bank=bank,source=str(path),source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),seconds=len(samples)/rate,rate=rate,offset=off,bytes=len(data),loop=loop,levels=levels(decoded)))
out=R/'nitrofiles/audio'
for name,data in banks.items():(out/f'chapter-{name}.bin').write_bytes(data)
h='#ifndef CHAPTER_AUDIO_H\n#define CHAPTER_AUDIO_H\n#include <stdint.h>\ntypedef struct{uint32_t offset,length;uint16_t rate;uint8_t loop;} ChapterAudioClip;\n'
h+='enum{'+','.join('CHAPTER_AUDIO_'+n.upper() for n,*_ in TRACKS)+'};\n'
h+='static const ChapterAudioClip chapter_audio_clips[]={'+','.join('{%d,%d,%d,%d}'%row for row in rows)+'};\n'
for name,data in banks.items():h+=f'#define CHAPTER_AUDIO_{name.upper().replace('-', '_')}_BYTES {len(data)}u\n'
h+='#endif\n';(R/'generated/chapter_audio.h').write_text(h);(R/'generated/chapter-audio-audit.json').write_text(json.dumps(audit,indent=2));print({n:len(b) for n,b in banks.items()})
