"""Original pickup cues used by MAIN f_GetPickup, converted to DS ADPCM."""
from pathlib import Path
import array,subprocess,json,hashlib
from convert_audio import speaker_compress,normalize_and_encode,levels
R=Path(__file__).resolve().parents[1]
G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data'/'sounds')
header=['#ifndef PICKUP_AUDIO_H','#define PICKUP_AUDIO_H','#define PICKUP_AUDIO_RATE 16000'];report=[]
for name,filename in [('coin1','sound_coin1.xma'),('coin2','sound_coin2.xma'),('eat','sound_eat.xma')]:
 source=G/filename
 raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(source),'-vn','-ac','1','-ar','16000','-f','f32le','-'])
 samples=[x*32768 for x in array.array('f',raw)]
 encoded,decoded,gain=normalize_and_encode(speaker_compress(samples,16000))
 header.append(f'static const unsigned char pickup_audio_{name}[] __attribute__((aligned(32)))={{')
 header.extend(','.join(map(str,encoded[i:i+32]))+',' for i in range(0,len(encoded),32));header.append('};')
 report.append({'cue':name,'source':filename,'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'bytes':len(encoded),'seconds':len(samples)/16000,'decoded':levels(decoded)})
header.append('#endif');(R/'generated/pickup_audio.h').write_text('\n'.join(header)+'\n');(R/'generated/pickup-audio-manifest.json').write_text(json.dumps(report,indent=2));print(report)
