"""One-shot sounds of the Thieves Forest cutscenes (LEVEL4 clips: owl, deer, bear, intro), as DS ADPCM in the chapter bank.

The original names are s_Explosion3/4, s_Fart1/2, s_PrincessScream3, s_PrincessHelp1 and s_MeatyPunch4; the sound files are matched by
name and the Explosion4 file is a best guess (the MAIN sound table is not in the extracted scripts)."""
from pathlib import Path
import array,subprocess,json,hashlib
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1]
G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'sounds')
RATE=11025
SOUNDS=[('explosion3','sound_explosion_3.xma',0.9),('explosion4','sound_bigexplosion1.xma',1.1),('fart1','sound_fart1.xma',0.9),('fart2','sound_fart2.xma',0.9),
        ('scream3','sound_princess_scream3.xma',1.4),('help1','sound_princess_help1.xma',1.4),('meaty4','sound_meatypunch4.xma',0.6)]
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
h=['#ifndef THIEVES_SFX_H','#define THIEVES_SFX_H','#include <stdint.h>',f'#define THIEVES_SFX_RATE {RATE}','enum {'+','.join('THIEVES_SFX_'+n.upper() for n,_,_ in SOUNDS)+f',THIEVES_SFX_COUNT}};',
   f'static const uint32_t thieves_sfx_table[{len(table)}][2]={{'+','.join('{%d,%d}'%t for t in table)+'};',
   f'static const uint8_t thieves_sfx_data[{len(data)}]__attribute__((aligned(4)))={{']
h.extend(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32));h.append('};');h.append('#endif')
(R/'generated/thieves_sfx.h').write_text('\n'.join(h)+'\n');(R/'generated/thieves-sfx-manifest.json').write_text(json.dumps(report,indent=2))
print(len(data),'bytes',[(r['cue'],r['bytes']) for r in report])
