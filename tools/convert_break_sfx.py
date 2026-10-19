"""Scenery cues: s_GrassCut (sound_cutgrass) for tufts and the chest landing sound for chests, as DS ADPCM."""
from pathlib import Path
import array,subprocess,json,hashlib
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1]
G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'sounds')
RATE=11025
header=['#ifndef BREAK_SFX_H','#define BREAK_SFX_H',f'#define BREAK_SFX_RATE {RATE}'];report=[]
for name,filename,seconds in [('grass','sound_cutgrass.xma',0.5),('chest','sound_chestland.xma',0.7)]:
    source=G/filename
    raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(source),'-vn','-ac','1','-ar',str(RATE),'-t',str(seconds),'-f','f32le','-'])
    samples=[x*32768 for x in array.array('f',raw)]
    encoded,decoded,gain=normalize_and_encode(speaker_compress(samples,RATE))
    header.append(f'static const unsigned char break_sfx_{name}[] __attribute__((aligned(32)))={{')
    header.extend(','.join(map(str,encoded[i:i+32]))+',' for i in range(0,len(encoded),32));header.append('};')
    report.append({'cue':name,'source':filename,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'bytes':len(encoded)})
header.append('#endif');(R/'generated/break_sfx.h').write_text('\n'.join(header)+'\n');(R/'generated/break-sfx-manifest.json').write_text(json.dumps(report,indent=2));print([(r['cue'],r['bytes']) for r in report])
