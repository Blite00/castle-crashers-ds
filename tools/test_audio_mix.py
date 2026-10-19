"""Offline two-player voice traces, including the runtime master peak budget."""
import sys,json,math,array,subprocess
from pathlib import Path
import numpy as np
from convert_audio import normalize_and_encode,decode
R=Path(__file__).resolve().parents[1];G=(Path(__import__('os').environ.get('CC_GAME_DIR','G:/SteamLibrary/steamapps/common/CastleCrashers'))/'data');report=json.loads((R/'generated/audio_manifest.json').read_text());env=json.loads((R/'generated/audio-envelopes.json').read_text());clips={}
for name in ('campaign','swing','hit','magic_blue','magic_orange'):
 row=next(v for v in report['tracks'] if v['name']==name);rate=row['sample_rate'];src=G/row['source']
 raw=subprocess.check_output([__import__('os').environ.get('FFMPEG','ffmpeg'),'-v','error','-i',str(src),'-vn','-ac','1','-ar',str(rate),'-f','f32le','-']);samples=[x*32768 for x in array.array('f',raw)];fade=min(rate//50,len(samples)//2)
 for i in range(fade):samples[i]*=i/fade;samples[-i-1]*=i/fade
 old=np.array(normalize_and_encode(samples)[1],dtype=float)/32768;new=np.array(decode((R/f'generated/audio_{name}.adpcm').read_bytes()),dtype=float)/32768
 n=next((r for r in env['rows'] if r[0]==row['encoded_bytes'] and r[2]==rate),None);peaks=env['peaks'][n[1]:n[1]+n[3]] if n else None
 clips[name]=(rate,old,new,peaks)
def trace(kind):
 events={0:[(8,'campaign',112,64)]}
 if kind!='idle':
  for p in range(2):
   for t in range(60+8*p,570,18 if kind=='busy' else 30):events.setdefault(t,[]).append((9+p,'swing',127,64+p*10));events.setdefault(t+4,[]).append((11+p,'hit',127,64+p*10))
   for t in range(110+p*75,530,150):events.setdefault(t,[]).append((13+p,'magic_blue' if p==0 else 'magic_orange',127,64+p*10))
 voices={};outs=[[],[]];masters=[]
 for tick in range(600):
  for ch,n,v,p in events.get(tick,[]):voices[ch]=(tick,n,v,p)
  left=right=0
  for ch,(st,n,v,p) in list(voices.items()):
   rate,o,a,e=clips[n];age=tick-st
   if ch!=8 and age/60>len(a)/rate+3/60:del voices[ch];continue
   peak=e[min(max(age-2,0),len(e)-1)] if e else 228
   vol=round(v*9/14) if ch==8 else (128 if v==127 else v)
   left+=((vol*(128 if ch==8 else 128-p)*peak+255)//256)**2;right+=((vol*(128 if ch==8 else p)*peak+255)//256)**2
  root=math.isqrt(max(left,right));root+=root*root<max(left,right);master=min(127,2026171//max(1,(root*13+9)//10));m=1 if master==127 else master/128;masters.append(master)
  count=(tick+1)*32000//60-tick*32000//60;t=np.arange(count)/32000
  blocks=[np.zeros((count,2)),np.zeros((count,2))]
  for ch,(st,n,v,p) in voices.items():
   rate,o,a,e=clips[n];idx=((t+(tick-st)/60)*rate).astype(int)
   for which,samples in enumerate((o,a)):
    ix=idx%len(samples) if ch==8 else np.minimum(idx,len(samples)-1);signal=samples[ix].copy()
    if ch!=8:signal[idx>=len(samples)]=0
    vol=(128 if v==127 else v)/128
    if which and ch==8:vl=vr=round(v*9/14)/128
    else:vl=vol*(128-p)/128;vr=vol*p/128
    blocks[which][:,0]+=signal*vl;blocks[which][:,1]+=signal*vr
  blocks[1]*=m
  for i in (0,1):outs[i].append(blocks[i])
 out=[np.concatenate(v) for v in outs];rms=[math.sqrt(np.mean(np.clip(v[32000:],-1,1)**2)) for v in out]
 result=dict(trace=kind,mix_gain_db=round(20*math.log10(rms[1]/rms[0]),3),old_clip_samples=int(np.sum(abs(out[0])>=1)),new_clip_samples=int(np.sum(abs(out[1])>=1)),new_peak=float(abs(out[1]).max()),minimum_master=min(masters),mean_master=round(float(np.mean(masters)),2))
 return result
result=[trace(n) for n in ('idle','normal','busy')];print(json.dumps(result,indent=2));(R/'generated/audio-mix-audit.json').write_text(json.dumps(result,indent=2))
assert all(r['new_clip_samples']<=max(4,r['old_clip_samples']) and r['mix_gain_db']>0 for r in result)
