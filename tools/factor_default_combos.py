"""Factor original default-combo keyframes into shared body/weapon/color layers."""
from pathlib import Path
import pickle,collections,zlib,json
import numpy as np
NW=9;R=Path(__file__).resolve().parents[1];bank=pickle.loads((R/'build/default-combos.pkl').read_bytes());samples=[0,3,6,9,12,17,20,23,26,31];images=[]
for f,raw in zip(bank['meta'],bank['raws']):
 _,x,y,w,h=f;a=np.zeros((120,192),np.uint8);a[y+88:y+88+h,x+96:x+96+w]=np.frombuffer(raw,np.uint8).reshape(h,w);images.append(a.ravel())
def patch(a,b):
 positions=np.flatnonzero(a!=b);s=bytearray();cursor=j=0
 while j<len(positions):
  start=int(positions[j]);end=start+1;j+=1
  while j<len(positions) and positions[j]==end and end-start<255 and end//192==start//192:end+=1;j+=1
  skip=start-cursor
  while skip>=128:s.append((skip&127)|128);skip>>=7
  s.extend((skip,end-start));s.extend(b[start:end]);cursor=end
 return bytes(s)
def apply(a,s):
 a=a.copy();p=c=0
 while p<len(s):
  skip=shift=0
  while True:
   v=s[p];p+=1;skip|=(v&127)<<shift;shift+=7
   if v<128:break
  n=s[p];p+=1;c+=skip;a[c:c+n]=list(s[p:p+n]);p+=n;c+=n
 return a
streams=[b''];seen={b'':0}
def add(s):
 if s not in seen:seen[s]=len(streams);streams.append(s)
 return seen[s]
base=[];colors=[[]for _ in range(4)];weapons=[[]for _ in range(NW)];fixes=[[[]for _ in range(NW)]for _ in range(4)];bounds=[]
for n in samples:
 a=images[n];base.append(add(patch(np.zeros_like(a),a)));affected=np.any(np.array([images[g*36+n]!=0 for g in range(4*NW)]),axis=0).reshape(120,192);ys,xs=np.where(affected);bounds.append((int(xs.min())-96,int(ys.min())-88,int(xs.max()-xs.min()+1),int(ys.max()-ys.min()+1)))
 cp=[];wp=[]
 for c in range(4):cp.append(patch(a,images[c*NW*36+n]));colors[c].append(add(cp[-1]))
 for w in range(NW):wp.append(patch(a,images[w*36+n]));weapons[w].append(add(wp[-1]))
 for c in range(4):
  for w in range(NW):
   pred=apply(apply(a,wp[w]),cp[c]);target=images[(c*NW+w)*36+n];fix=patch(pred,target);fixes[c][w].append(add(fix));assert np.array_equal(apply(pred,fix),target)
print('streams',len(streams),'raw',sum(map(len,streams)),'deflate',sum(len(zlib.compress(s,9))-6 for s in streams if s))
(R/'build/default-combo-factored.pkl').write_bytes(pickle.dumps((streams,base,colors,weapons,fixes,bounds,samples)))
