from pathlib import Path
import pickle,struct
import numpy as np
R=Path(__file__).resolve().parents[1];records,blob,indices,maxpixels=pickle.loads((R/'build/held-patches.pkl').read_bytes());base=pickle.loads((R/'build/held-base.pkl').read_bytes());bounds=pickle.loads((R/'build/held-factored.pkl').read_bytes())[3];out=bytearray()
for slot in range(9):
 for fid,(_,bx,by,bw,bh) in enumerate(base['frames']):
  canvas=np.zeros((120,192),np.uint8);canvas[by+88:by+88+bh,bx+96:bx+96+bw]=np.frombuffer(base['pixels'][fid],np.uint8).reshape(bh,bw)
  if slot>=2:
   x,y,w,h,off,length=records[indices[slot-1][fid]];s=blob[off:off+length];p=c=0
   while p<len(s):
    c+=s[p]|s[p+1]<<8;n=s[p+2];p+=3
    for j,v in enumerate(s[p:p+n]):canvas[y+88+(c+j)//w,x+96+(c+j)%w]=v
    p+=n;c+=n
   x,y,w,h=bounds[slot-2][fid%304]
  else:x,y,w,h=bx,by,bw,bh
  hash=2166136261
  for v in canvas[y+88:y+88+h,x+96:x+96+w].ravel():hash=((hash^base['palette'][int(v)])*16777619)&0xffffffff
  out+=struct.pack('<bbBBI',x,y,w,h,hash)
(R/'generated/held-weapon-test-hashes.bin').write_bytes(out)
