"""Sparse patches from original PLAYER weapon selectors; preserve base armor pixels."""
from pathlib import Path
import pickle,json,sys
import numpy as np
from PIL import Image
from convert_barbarian import array
R=Path(__file__).resolve().parents[1];base=pickle.loads((R/'build/held-base.pkl').read_bytes());pal=base['palette'];qpal=Image.new('P',(1,1));rgb=[]
for i,c in enumerate(pal):
 if not i:c=pal[1]
 rgb += [(c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31]
qpal.putpalette(rgb)
records=[(0,0,0,0,0,0)];blob=bytearray();lookup={};indices=[[] for _ in range(8)];maxpixels=4080;changed=0;preview=Image.new('RGB',(768,480),(40,45,50));shown=0
for ci,color in enumerate(('blue','orange','red','green')):
 for n in range(1,305):
  fid=ci*304+n-1;_,bx,by,bw,bh=base['frames'][fid];old=Image.open(R/f'assets/{color}/{n}.png').convert('RGBA').resize((192,120),Image.Resampling.LANCZOS);oa=np.array(old);baseblock=np.frombuffer(base['pixels'][fid],np.uint8).reshape(bh,bw)
  for slot,weapon in enumerate((2,9,19,10,38,30,37,42)):
   path=R/f'assets/held-{color}-{weapon}/{n}.png'
   if not path.exists():
    if '--partial' in sys.argv:continue
    raise FileNotFoundError(path)
   im=Image.open(path).convert('RGBA').resize((192,120),Image.Resampling.LANCZOS);na=np.array(im);alpha=na[:,:,3]>=100;ys,xs=np.where(alpha);nx,ny,nr,nb=int(xs.min())-96,int(ys.min())-88,int(xs.max())+1-96,int(ys.max())+1-88
   x,y=min(bx,nx),min(by,ny);w,h=max(bx+bw,nr)-x,max(by+bh,nb)-y;maxpixels=max(maxpixels,w*h)
   before=np.zeros((h,w),np.uint8);before[by-y:by-y+bh,bx-x:bx-x+bw]=baseblock;target=before.copy()
   mask=np.any(oa!=na,axis=2)&((oa[:,:,3]>=100)|alpha);q=np.array(im.convert('RGB').quantize(palette=qpal,dither=Image.Dither.NONE));q[q==0]=1;q[~alpha]=0
   crop=(slice(y+88,y+88+h),slice(x+96,x+96+w));m=mask[crop];target[m]=q[crop][m]
   same_color=np.take(pal,before)==np.take(pal,target);target[same_color]=before[same_color]
   positions=np.flatnonzero(before.ravel()!=target.ravel());stream=bytearray();cursor=0;j=0
   while j<len(positions):
    start=int(positions[j]);end=start+1;j+=1
    while j<len(positions) and positions[j]==end and end-start<255:end+=1;j+=1
    skip=start-cursor;stream.extend((skip&255,skip>>8,end-start));stream.extend(target.ravel()[start:end].tobytes());cursor=end
   if not stream:rid=0
   else:
    key=(x,y,w,h,bytes(stream));rid=lookup.get(key)
    if rid is None:rid=len(records);lookup[key]=rid;records.append((x,y,w,h,len(blob),len(stream)));blob.extend(stream)
    changed+=len(positions)
   indices[slot].append(rid)
   # Independent roundtrip of sparse patches, including erased starter pixels.
   check=before.ravel().copy();off=0;cursor=0
   while off<len(stream):skip=stream[off]|stream[off+1]<<8;count=stream[off+2];off+=3;cursor+=skip;check[cursor:cursor+count]=list(stream[off:off+count]);off+=count;cursor+=count
   assert np.array_equal(check.reshape(h,w),target),(color,n,weapon)
   if ci==0 and n in (1,30,109,125) and weapon in (2,9,19,10,38,30,37,42):
    canvas=Image.new('RGBA',(192,120));out=np.zeros((h,w,4),np.uint8)
    for v in np.unique(target):
     if v:out[target==v]=rgb[int(v)*3:int(v)*3+3]+[255]
    canvas.paste(Image.fromarray(out),(x+96,y+88));preview.paste(canvas,((shown%4)*192,(shown//4)*120),canvas);shown+=1
(R/'build/held-patches.pkl').write_bytes(pickle.dumps((records,bytes(blob),indices,maxpixels)))
import zlib
print('deflate',sum(len(zlib.compress(bytes(blob[r[4]:r[4]+r[5]]),9))-6 for r in records if r[5]),'max stream',max(r[5] for r in records))
print('records',len(records),'payload',len(blob),'indexbytes',sum(len(a)*2 for a in indices),'metabytes',len(records)*12,'maxpixels',maxpixels,'changed',changed)
if '--partial' in sys.argv:sys.exit()
h='#ifndef KNIGHT_WEAPON_PATCHES_H\n#define KNIGHT_WEAPON_PATCHES_H\n#include <string.h>\n#include "castle_art.h"\n'
h+=f'#define KNIGHT_WEAPON_FRAME_PIXELS_MAX {maxpixels}\n'
h+='typedef struct {int8_t x,y;uint8_t w,h;uint32_t offset;uint16_t length;} KnightWeaponPatch;\n'
h+=array('knight_weapon_patch_data','uint8_t',blob)
h+='static const KnightWeaponPatch knight_weapon_patches[]={\n'+''.join('{'+','.join(map(str,r))+'},\n' for r in records)+'};\n'
h+='static const uint16_t knight_weapon_patch_index[6][1216]={\n'+''.join('{'+','.join(map(str,v))+'},\n' for v in indices)+'};\n'
h+='''static inline const KnightWeaponPatch *weapon_patch(unsigned id,unsigned slot){
 return &knight_weapon_patches[(id<1216&&slot>0&&slot<4)?knight_weapon_patch_index[slot-1][id]:0];
}
static inline ArtFrame weapon_frame(unsigned id,unsigned slot){
 ArtFrame f=knight_frames[id];const KnightWeaponPatch *p=weapon_patch(id,slot);
 if(p->length){f.x=p->x;f.y=p->y;f.w=p->w;f.h=p->h;}return f;
}
static inline int weapon_apply(unsigned id,unsigned slot,uint8_t *out){
 const KnightWeaponPatch *p=weapon_patch(id,slot);if(!p->length)return 1;
 const ArtFrame *base=&knight_frames[id];unsigned dx=base->x-p->x,dy=base->y-p->y;
 for(int row=base->h-1;row>=0;row--)memmove(out+(row+dy)*p->w+dx,out+row*base->w,base->w);
 memset(out,0,dy*p->w);
 for(unsigned row=dy;row<dy+base->h;row++){memset(out+row*p->w,0,dx);memset(out+row*p->w+dx+base->w,0,p->w-dx-base->w);}
 memset(out+(dy+base->h)*p->w,0,(p->h-dy-base->h)*p->w);
 unsigned offset=0,cursor=0,size=p->w*p->h;const uint8_t *data=knight_weapon_patch_data+p->offset;
 while(offset<p->length){
  if(p->length-offset<3)return 0;
  cursor+=data[offset]|data[offset+1]<<8;unsigned count=data[offset+2];offset+=3;
  if(cursor+count>size||offset+count>p->length)return 0;
  memcpy(out+cursor,data+offset,count);offset+=count;cursor+=count;
 }return 1;
}
#endif
'''
(R/'build/knight_weapon_patches.h').write_text(h);preview.save(R/'generated/held-weapon-preview.png')
(R/'generated/held-weapon-audit.json').write_text(json.dumps(dict(source='PLAYER.swf f_Weapon sprite243',weapons=[1,2,9,19,10,38,30,37,42],frames=1216,patches=len(records),data_bytes=len(blob),total_bytes=len(blob)+len(records)*12+7296,max_pixels=maxpixels,roundtrips=7296),indent=2))


# Factor colors and emit final bounded runtime codec after source comparisons.
import subprocess
for script in ("factor_held_weapons.py","encode_held_weapons.py","held_weapon_references.py"):
 subprocess.run([sys.executable,str(R/"tools"/script)],check=True)
