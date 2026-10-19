"""Read every shipped decoded ADPCM clip, including NitroFS cue banks."""
from pathlib import Path
import json,re,sys
from convert_audio import decode,levels
R=Path(__file__).resolve().parents[1]
def inventory():
 clips={}
 for p in (R/'generated').glob('audio_*.adpcm'):clips[p.stem]=p.read_bytes()
 h=(R/'generated/pickup_audio.h').read_text()
 for name,body in re.findall(r'static const unsigned char (\w+)\[\].*?=\{(.*?)\};',h,re.S):clips[name]=bytes(map(int,re.findall(r'\d+',body)))
 for row in json.loads((R/'generated/chapter-audio-audit.json').read_text()):
  b=(R/f"nitrofiles/audio/chapter-{row['bank']}.bin").read_bytes();clips['chapter_'+row['name']]=b[row['offset']:row['offset']+row['bytes']]
 audit=json.loads((R/'generated/cinematic-audio-audit.json').read_text());banks=[(R/f'nitrofiles/audio/{n}.bin').read_bytes() for n in ('cinematic','barracks')]
 for row in audit['cues']:clips['cinematic_'+row['cue']]=banks[row['bank']][row['offset']:row['offset']+row['bytes']]
 return {name:dict(bytes=len(raw),**levels(decode(raw))) for name,raw in clips.items()}
if __name__=='__main__':
 result=inventory();p=R/'tests/audio-loudness-baseline.json'
 if '--baseline' in sys.argv:p.write_text(json.dumps(result,indent=2))
 else:
  old=json.loads(p.read_text());out={n:dict(v,rms_gain_db=round(v['rms_dbfs']-old[n]['rms_dbfs'],3)) for n,v in result.items()};(R/'generated/audio-loudness-audit.json').write_text(json.dumps(out,indent=2));print([(n,v['rms_gain_db']) for n,v in out.items()]);assert all(v['bytes']==old[n]['bytes'] and v['full_scale_samples']==0 and v['peak_dbfs']<=-1 for n,v in out.items())

