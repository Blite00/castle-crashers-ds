"""OriginalLEVEL2/13 cue banks embedded in NitroFS, loaded into idle render caches.
No reduced sampling beyond the established5512Hzmusic/8000Hzeffect imports.
"""
import argparse,array,hashlib,json,subprocess
from pathlib import Path
from convert_audio import speaker_compress,normalize_and_encode,levels
ROOT=Path(__file__).resolve().parents[1]
# IDs remain stable; actual SOUNDS.swf registration maps each effect name.
CUES=(
 ('intro','music/020_CREEPY_FROG.xma',5512,357/30,2,359,False,0),
 ('force','sounds/sound_darkforce.xma',8000,None,172,359,True,0),
 ('fly','sounds/sound_darkfly.xma',8000,None,347,None,False,0),
 ('scream','sounds/sound_princess_scream1.xma',8000,None,22,None,False,0),
 ('help','sounds/sound_princess_help1.xma',8000,None,90,None,False,0),
 ('door','sounds/sound_frost_icepop.xma',8000,None,161,None,False,1),
 ('block3','sounds/sound_shield3.xma',8000,None,196,None,False,1),
 ('ground2','sounds/sound_bodyfall4.xma',8000,None,231,None,False,1),
 ('ground5','sounds/sound_bodyfall5.xma',8000,None,293,None,False,1),
)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('game_directory',type=Path);p.add_argument('--ffmpeg',default='ffmpeg');a=p.parse_args()
 banks=[bytearray(),bytearray()];records=[];report=[]
 for name,rel,rate,duration,start,stop,loop,bank in CUES:
  source=a.game_directory/'data'/rel
  raw=subprocess.check_output([a.ffmpeg,'-v','error','-i',str(source),'-vn','-ac','1','-ar',str(rate),'-f','f32le','-'])
  samples=[v*32768 for v in array.array('f',raw)];source_seconds=len(samples)/rate
  if duration is not None:
   samples=samples[:round(duration*rate)];fade=rate//200
   for i in range(fade):samples[i]*=i/fade;samples[-1-i]*=i/fade
  encoded,decoded,gain=normalize_and_encode(speaker_compress(samples,rate))
  while len(banks[bank])%32:banks[bank].append(0)
  offset=len(banks[bank]);banks[bank].extend(encoded);records.append((offset,len(encoded),rate))
  report.append(dict(cue=name,source=rel,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),source_seconds=source_seconds,retained_seconds=len(samples)/rate,rate=rate,bytes=len(encoded),gain=gain,levels=levels(decoded),source_start_frame=start,source_stop_frame=stop,loop=loop,bank=bank,offset=offset))
 out=ROOT/'nitrofiles/audio';out.mkdir(parents=True,exist_ok=True)
 for name,bank in zip(['cinematic','barracks'],banks):(out/f'{name}.bin').write_bytes(bank)
 header='''#ifndef CINEMATIC_AUDIO_H
#define CINEMATIC_AUDIO_H
#include <stdint.h>
typedef struct {uint32_t offset,length;uint16_t rate;} CinematicAudioClip;
'''
 header+='enum {'+','.join('CIN_AUDIO_'+c[0].upper() for c in CUES)+'};\n'
 header+='static const CinematicAudioClip cinematic_audio_clips[]={\n'+''.join(' {'+','.join(map(str,r))+'},\n' for r in records)+'};\n'
 header+=f'#define CINEMATIC_AUDIO_MAIN_BYTES {len(banks[0])}u\n#define CINEMATIC_AUDIO_BARRACKS_BYTES {len(banks[1])}u\n#endif\n'
 (ROOT/'generated/cinematic_audio.h').write_text(header)
 audit=dict(storage='NitroFS originalADPCM banks; inactive renderer storage, zero audio heap/static PCM allocation',main_bytes=len(banks[0]),barracks_bytes=len(banks[1]),music_interval='Sprite264Music23 fromsource2 untilStopMusic359:357/30seconds. OriginalFadeOut(30) at347 is an external musicAPI, not visual.',mapping_source='assets/extracted/sounds.xml originalSOUNDS.swf customPush registration',shared_original_cues={'Block1':'existing castle_audio_block=sound_shield1','Punch1':'existing castle_audio_hit=sound_meatypunch4'},cues=report)
 (ROOT/'generated/cinematic-audio-audit.json').write_text(json.dumps(audit,indent=2));print(json.dumps(audit))
if __name__=='__main__':main()
