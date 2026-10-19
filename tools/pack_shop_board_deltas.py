"""Store sparse XORs for shop states where matching potion geometry repeats."""
from pathlib import Path
import re,zlib
R=Path(__file__).resolve().parents[1]
def sparse(raw):
 out=bytearray();i=0
 while i<len(raw):
  skip=raw[i]==0;j=i+1
  while j<len(raw) and (raw[j]==0)==skip and j-i<128:j+=1
  out.append((128 if skip else 0)+(j-i)-1)
  if not skip:out.extend(raw[i:j])
  i=j
 return out
def optimize():
 p=R/'generated/shop_board_art.h';s=p.read_text()
 if 'SHOP_BOARD_SPARSE_XOR' in s:return
 m=re.search(r'shop_board_pixels\[\]=\{(.*?)\};',s,re.S);blob=bytes(map(int,re.findall(r'\d+',m[1])));mm=re.search(r'shop_board_frames\[\]=\{(.*?)\};',s,re.S);meta=[tuple(map(int,t.split(','))) for t in re.findall(r'\{([^}]+)\}',mm[1])];raw=[zlib.decompress(blob[o:o+n],-15) for o,n,*_ in meta];out=bytearray();new=[];seen={}
 for i,(o,n,x,y,w,h) in enumerate(meta):
  packed=blob[o:o+n];flag=0
  if i>=5 and meta[i][2:]==meta[i-5][2:]:
   delta=sparse(bytes(a^b for a,b in zip(raw[i],raw[i-5])))
   if len(delta)<len(packed) and raw[i]!=raw[i-5]:packed=delta;flag=32768
  key=(bytes(packed),flag)
  if key not in seen:seen[key]=len(out);out.extend(packed)
  new.append((seen[key],len(packed)|flag,x,y,w,h))
 s=s[:mm.start()]+'shop_board_frames[]={'+','.join('{'+','.join(map(str,t))+'}' for t in new)+'};'+s[mm.end():]
 m=re.search(r'shop_board_pixels\[\]=\{(.*?)\};',s,re.S);s=s[:m.start()]+'shop_board_pixels[]={'+','.join(map(str,out))+'};'+s[m.end():]
 start=s.index('static inline int shop_board_decode');s=s[:start]+'''#define SHOP_BOARD_SPARSE_XOR 1
static inline int shop_board_decode(unsigned frame,uint8_t *out){
 if(frame>=10||!out)return 0;
 const ShopBoardFrame *f=&shop_board_frames[frame];
 if(!(f->length&32768))return asset_inflate(shop_board_pixels+f->offset,f->length,out,f->w*f->h);
 if(frame<5)return 0;
 const ShopBoardFrame *base=f-5;
 if(!asset_inflate(shop_board_pixels+base->offset,base->length,out,base->w*base->h))return 0;
 unsigned pos=0,size=f->w*f->h,at=f->offset,end=at+(f->length&32767);
 while(at<end){
  unsigned token=shop_board_pixels[at++],n=(token&127)+1;
  if(n>size-pos)return 0;
  if(token&128)pos+=n;
  else {if(n>end-at)return 0;while(n--)out[pos++]^=shop_board_pixels[at++];}
 }
 return pos==size;
}
#endif
''';p.write_text(s);print('Shop exact sparseXOR saved',len(blob)-len(out),'bytes')
if __name__=='__main__':optimize()


# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
