from pathlib import Path
import pickle,zlib,collections,heapq
import numpy as np
R=Path(__file__).resolve().parents[1];records,blob,indices,maxpixels=pickle.loads((R/'build/held-patches.pkl').read_bytes());base=pickle.loads((R/'build/held-base.pkl').read_bytes())
def basecanvas(fid):
 _,x,y,w,h=base['frames'][fid];out=np.zeros((120,192),np.uint8);out[y+88:y+88+h,x+96:x+96+w]=np.frombuffer(base['pixels'][fid],np.uint8).reshape(h,w);return out

def ops(rid):
 x,y,w,h,off,length=records[rid];s=blob[off:off+length];d={};p=c=0
 while p<len(s):
  c+=s[p]|s[p+1]<<8;n=s[p+2];p+=3
  for j,v in enumerate(s[p:p+n]):d[(y+88+(c+j)//w)*192+x+96+(c+j)%w]=v
  p+=n;c+=n
 return d

def pack(d):
 stream=bytearray();keys=sorted(d);j=0;cursor=0
 while j<len(keys):
  start=keys[j];end=start+1;j+=1
  while j<len(keys) and keys[j]==end and end//192==start//192 and end-start<255:end+=1;j+=1
  skip=start-cursor
  while skip>=128:stream.append((skip&127)|128);skip>>=7
  stream.extend((skip,end-start));stream.extend(d[k] for k in range(start,end));cursor=end
 return bytes(stream)
streams=[b''];lookup={b'':0};NS=10;common=[[] for _ in range(NS)];fixes=[[[] for _ in range(NS)] for _ in range(4)];bounds=[[] for _ in range(NS)];total=0

def add(s):
 if s not in lookup:lookup[s]=len(streams);streams.append(s)
 return lookup[s]
for slot in range(NS):
 for n in range(304):
  originals=[];targets=[];pal=np.array(base['palette']);canonical={};remap=[]
  for c in base['palette']:
   if c not in canonical:canonical[c]=len(remap)
   remap.append(canonical[c])
  for color in range(4):
   fid=color*304+n;before=basecanvas(fid);target=before.ravel().copy();own=ops(indices[slot+1][fid])
   for pos,v in own.items():target[pos]=v
   originals.append(before.ravel());targets.append(target)
  original_colors=np.take(pal,np.array(originals));target_colors=np.take(pal,np.array(targets));positions=np.flatnonzero(np.any(original_colors!=target_colors,axis=0));shared={}
  for pos in positions:
   colors=collections.Counter(int(t[pos]) for t in target_colors);chosen,num=colors.most_common(1)[0]
   unchanged_cost=int(np.count_nonzero(original_colors[:,pos]!=target_colors[:,pos]));shared_cost=1+4-num
   if shared_cost<unchanged_cost:shared[int(pos)]=canonical[chosen]
  common[slot].append(add(pack(shared)));bound=[]
  for color in range(4):
   before=originals[color];target=targets[color];pred=before.copy()
   for pos,v in shared.items():pred[pos]=v
   diff=np.flatnonzero(np.take(pal,pred)!=np.take(pal,target));d={int(pos):remap[int(target[pos])] for pos in diff};fixes[color][slot].append(add(pack(d)))
   for pos,v in d.items():pred[pos]=v
   assert np.array_equal(np.take(pal,pred),np.take(pal,target))
   yy,xx=np.where((target.reshape(120,192)!=0)|(before.reshape(120,192)!=0));bound.append((int(xx.min()),int(yy.min()),int(xx.max())+1,int(yy.max())+1))
  x,y=min(b[0] for b in bound),min(b[1] for b in bound);w,h=max(b[2] for b in bound)-x,max(b[3] for b in bound)-y;bounds[slot].append((x-96,y-88,w,h));maxpixels=max(maxpixels,w*h)
print('factoredstreams',len(streams),'raw',sum(map(len,streams)),'max',max(map(len,streams)),'deflate',sum(len(zlib.compress(s,9))-6 for s in streams if s),'maxpixels',maxpixels)
(R/'build/held-factored.pkl').write_bytes(pickle.dumps((streams,common,fixes,bounds,maxpixels)))
counts=collections.Counter(b for s in streams for b in s);heap=[(v,k,k) for k,v in counts.items()];heapq.heapify(heap);seq=256
while len(heap)>1:
 a=heapq.heappop(heap);b=heapq.heappop(heap);heapq.heappush(heap,(a[0]+b[0],seq,(a[2],b[2])));seq+=1
lengths={}
def walk(n,depth):
 if isinstance(n,int):lengths[n]=depth
 else:walk(n[0],depth+1);walk(n[1],depth+1)
walk(heap[0][2],0);print('huffman',sum((sum(lengths[b] for b in s)+7)//8 for s in streams),'maxbits',max(lengths.values()))
