from pathlib import Path
import re,sys,hashlib
R=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(R/'tools'))
from optimize_menu_alpha import pack
p=R/'generated/menu_art.c';s=p.read_text();m=re.search(r'static const uint8_t menu_alpha\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);a=bytes(map(int,re.findall(r'\d+',m[1])));mm=re.search(r'const MenuLabel menu_labels\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);meta=[tuple(map(int,v)) for v in re.findall(r'\{(\d+),(\d+),(\d+)\}',mm[1])];blob=bytearray();new=[];raw=bytearray();original=bytearray()
for id,(off,w,h) in enumerate(meta):
 out=bytearray();i=off
 while len(out)<w*h:
  t=a[i];i+=1;n=(t&127)+1
  if t&128:out.extend([a[i]]*n);i+=1
  else:out.extend(a[i:i+n]);i+=n
 original.extend(out)
 if id<3:out=bytearray(w*h)
 raw.extend(out);new.append((len(blob),w,h));blob.extend(pack(out))
assert raw[sum(w*h for _,w,h in meta[:3]):]==original[sum(w*h for _,w,h in meta[:3]):]
s=s[:mm.start()]+'const MenuLabel menu_labels[10]={'+','.join('{%d,%d,%d}'%v for v in new)+'};'+s[mm.end():]
m=re.search(r'static const uint8_t menu_alpha\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);s=s[:m.start()]+'static const uint8_t menu_alpha['+str(len(blob))+']={'+','.join(map(str,blob))+'};'+s[m.end():];p.write_text(s)
print('Savings',len(a)-len(blob),'newhash',hashlib.sha256(raw).hexdigest())
