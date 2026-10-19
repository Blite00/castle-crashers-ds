"""Small conservative suffix-peak bounds for decaying sound effects."""
from pathlib import Path
import json,re,math
from convert_audio import decode
R=Path(__file__).resolve().parents[1];clips=[]
for row in json.loads((R/'generated/audio_manifest.json').read_text())['tracks']:
 if not row['loop']:clips.append((row['sample_rate'],(R/f"generated/audio_{row['name']}.adpcm").read_bytes()))
for name,body in re.findall(r'static const unsigned char (\w+)\[\].*?=\{(.*?)\};',(R/'generated/pickup_audio.h').read_text(),re.S):clips.append((16000,bytes(map(int,re.findall(r'\d+',body)))))
audit=json.loads((R/'generated/cinematic-audio-audit.json').read_text());banks=[(R/f'nitrofiles/audio/{n}.bin').read_bytes() for n in ('cinematic','barracks')]
for row in audit['cues']:
 if row['cue']!='intro':clips.append((row['rate'],banks[row['bank']][row['offset']:row['offset']+row['bytes']]))
for row in json.loads((R/'generated/chapter-audio-audit.json').read_text()):
 if row['name']=='smith_hammer':
  b=(R/f"nitrofiles/audio/chapter-{row['bank']}.bin").read_bytes();clips.append((row['rate'],b[row['offset']:row['offset']+row['bytes']]))
merged={}
for rate,raw in clips:
 pcm=decode(raw);peaks=[math.ceil(max(map(abs,pcm[i*rate//60:min(len(pcm),(i+1)*rate//60)]),default=0)*255/32768) for i in range(math.ceil(len(pcm)*60/rate))]
 for i in range(len(peaks)-2,-1,-1):peaks[i]=max(peaks[i],peaks[i+1])
 key=(rate,len(raw));prior=merged.get(key,[0]*len(peaks));assert len(prior)==len(peaks);merged[key]=[max(a,b) for a,b in zip(prior,peaks)]
blob=[];rows=[]
for (rate,size),peaks in sorted(merged.items()):rows.append((size,len(blob),rate,len(peaks)));blob+=peaks
h='#ifndef AUDIO_ENVELOPES_H\n#define AUDIO_ENVELOPES_H\n#include <stdint.h>\ntypedef struct{uint32_t bytes;uint16_t offset,rate,count;} AudioEnvelope;\n'
h+='static const uint8_t audio_envelope_peaks[]={'+','.join(map(str,blob))+'};\n'
h+='static const AudioEnvelope audio_envelopes[]={'+','.join('{'+','.join(map(str,row))+'}' for row in rows)+'};\n#endif\n'
(R/'generated/audio_envelopes.h').write_text(h);(R/'generated/audio-envelopes.json').write_text(json.dumps(dict(rows=rows,peaks=blob)));print('Audio envelope bytes',len(blob),'records',len(rows))
