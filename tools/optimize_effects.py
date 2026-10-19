"""Deduplicate identical original effect and readiness-alpha blocks losslessly."""
from pathlib import Path
import re,json
R=Path(__file__).resolve().parents[1]
def optimize():
 p=R/'generated/effects_art.h';s=p.read_text()
 def nums(name):return list(map(int,re.findall(r'-?\d+',re.search(r'\b'+name+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)[1])))
 def replace(name,values):
  nonlocal s
  s=re.sub(r'(\b'+name+r'\[[^]]*\]\s*=\s*\{).*?(\};)',lambda m:m[1]+'\n'+','.join(map(str,values))+'\n'+m[2],s,flags=re.S)
 pixels=nums('effect_pixels');alpha=nums('effect_ready_alpha');offsets=nums('effect_ready_offsets');flat=nums('effect_frames');frames=[flat[i:i+5] for i in range(0,len(flat),5)]
 out=[];seen={}
 for f in frames:
  block=tuple(pixels[f[0]:f[0]+f[3]*f[4]])
  if block not in seen:seen[block]=len(out);out.extend(block)
  f[0]=seen[block];assert tuple(out[f[0]:f[0]+f[3]*f[4]])==block
 newalpha=[];seen={};new_offsets=[]
 for offset,id in zip(offsets,list(range(48,64))+list(range(96,112))):
  f=frames[id];block=tuple(alpha[offset:offset+f[3]*f[4]])
  if block not in seen:seen[block]=len(newalpha);newalpha.extend(block)
  new_offsets.append(seen[block]);assert tuple(newalpha[seen[block]:seen[block]+len(block)])==block
 replace('effect_pixels',out);replace('effect_ready_alpha',newalpha);replace('effect_ready_offsets',new_offsets)
 s=re.sub(r'(\beffect_frames\[[^]]*\]\s*=\s*\{).*?(\};)',lambda m:m[1]+'\n'+''.join('{'+','.join(map(str,f))+'},\n' for f in frames)+m[2],s,flags=re.S);p.write_text(s)
 audit=R/'generated/effects-audit.json';report=json.loads(audit.read_text());report.update(pixel_bytes=len(out),ready_alpha_bytes=len(newalpha),lossless_dedup=True);audit.write_text(json.dumps(report,indent=2))
 print('Effects lossless bytes',len(pixels)+len(alpha),'->',len(out)+len(newalpha),'saved',len(pixels)+len(alpha)-len(out)-len(newalpha))
if __name__=='__main__':optimize()
