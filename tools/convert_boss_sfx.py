"""One-shot sounds of the Barbarian Boss gate scene (LEVEL7 sprite 202): ground thumps, the roar and the explosions of the falling door, as DS ADPCM in the chapter bank.
The original names are s_Ground2, s_Roar2, s_Explosion2 and s_Explosion3; the files are matched by name (Ground2 and Explosion2 are best guesses: the MAIN sound table is not in the extracted scripts)."""
from pathlib import Path
import array,subprocess,json,hashlib
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1]
G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'sounds')
RATE=8000
SOUNDS=[('ground2','sound_normalexplosion1.xma',0.6),('roar2','sound_roar2.xma',1.6),('explosion2','sound_normalexplosion2.xma',0.9),('explosion3','sound_explosion_3.xma',1.1)]
data=bytearray();table=[];report=[]
for name,filename,seconds in SOUNDS:
    source=G/filename
    raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(source),'-vn','-ac','1','-ar',str(RATE),'-t',str(seconds),'-f','f32le','-'])
    samples=[x*32768 for x in array.array('f',raw)]
    encoded,decoded,gain=normalize_and_encode(speaker_compress(samples,RATE))
    while len(data)%4:data.append(0)
    table.append((len(data),len(encoded)));data.extend(encoded)
    report.append({'cue':name,'source':filename,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'bytes':len(encoded),'seconds':len(samples)/RATE,'decoded':levels(decoded)})
while len(data)%4:data.append(0)
h=['#ifndef BOSS_SFX_H','#define BOSS_SFX_H','#include <stdint.h>',f'#define BOSS_SFX_RATE {RATE}','enum {'+','.join('BOSS_SFX_'+n.upper() for n,_,_ in SOUNDS)+f',BOSS_SFX_COUNT}};',
   f'static const uint32_t boss_sfx_table[{len(table)}][2]={{'+','.join('{%d,%d}'%t for t in table)+'};',
   f'static const uint8_t boss_sfx_data[{len(data)}]__attribute__((aligned(4)))={{']
h.extend(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32));h.append('};');h.append('#endif')
(R/'generated/boss_sfx.h').write_text(chr(10).join(h)+chr(10));(R/'generated/boss-sfx-manifest.json').write_text(json.dumps(report,indent=2))
print(len(data),'bytes',[(r['cue'],r['bytes']) for r in report])
