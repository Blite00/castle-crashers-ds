"""Menu cues: s_Toggle (sound_toggle_b) for moving and backing out, s_Select (sound_player_select) for confirming."""
from pathlib import Path
import array,subprocess,json,hashlib
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1]
G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'sounds')
RATE=16000
header=['#ifndef MENU_SFX_H','#define MENU_SFX_H',f'#define MENU_SFX_RATE {RATE}'];report=[]
for name,filename,seconds in [('move','sound_toggle_b.xma',0.4),('select','sound_player_select.xma',0.8)]:
    source=G/filename
    raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(source),'-vn','-ac','1','-ar',str(RATE),'-t',str(seconds),'-f','f32le','-'])
    samples=[x*32768 for x in array.array('f',raw)]
    encoded,decoded,gain=normalize_and_encode(speaker_compress(samples,RATE))
    header.append(f'static const unsigned char menu_sfx_{name}[] __attribute__((aligned(32)))={{')
    header.extend(','.join(map(str,encoded[i:i+32]))+',' for i in range(0,len(encoded),32));header.append('};')
    report.append({'cue':name,'source':filename,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'bytes':len(encoded),'decoded':levels(decoded)})
header.append('#endif');(R/'generated/menu_sfx.h').write_text('\n'.join(header)+'\n');(R/'generated/menu-sfx-manifest.json').write_text(json.dumps(report,indent=2));print([(r['cue'],r['bytes']) for r in report])
