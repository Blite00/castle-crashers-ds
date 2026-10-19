"""Original LEVEL2 actors and furniture; fast RLE into the shared renderer scratch."""
from pathlib import Path
import json,sys,zlib
from PIL import Image,ImageDraw
from convert_barbarian import pack,unpack
from convert_barbarian import array
R=Path(__file__).resolve().parents[1]
spec=json.loads((R/'assets/barracks-actors-spec.json').read_text()); images=[]; ranges={};spec.append(['furniture',0,list(range(1,1+len(json.loads((R/'assets/barracks-furniture-depths.json').read_text()))))])
last_wounded_foot=None
for name,sid,frames in spec:
 ranges[name]=(len(images),len(frames))
 for frame in frames:
  im=Image.open(R/f'assets/barracks-{name}/{frame}.png').convert('RGBA');im=im.resize((im.width//2,im.height//2),Image.Resampling.LANCZOS)
  alpha=im.getchannel('A').point(lambda a:255 if a>=100 else 0);box=alpha.getbbox()
  empty=box is None
  if empty:box=(64,80,65,81)
  x,y,r,b=box;cropped=im.crop(box)
  if name=='wounded':
   if empty and last_wounded_foot:x,y=last_wounded_foot[0],last_wounded_foot[1]-1
   elif not empty:last_wounded_foot=((x+r)//2,b)
  images.append((name,frame,cropped,x if name in ('wounded','furniture') else x-64,y if name in ('wounded','furniture') else y-80))
for name in ('drums','guitar','cello'):
 assert len({im.tobytes() for n,f,im,x,y in images if n==name})==3, f'{name}: nested animation was accidentally frozen'
assert max(im.width*im.height for n,f,im,x,y in images)<=32*448
atlas=Image.new('RGB',(256,len(images)*128),(32,40,45));preview=Image.new('RGB',atlas.size,(32,40,45))
for i,(name,frame,im,x,y) in enumerate(images):atlas.paste(im,(0,i*128),im);preview.paste(im,(0,i*128),im);ImageDraw.Draw(preview).text((130,i*128+5),f'{name} {frame}\n{x},{y}',fill='white')
q=Image.new('P',(1,1));q.putpalette(json.loads((R/'assets/barracks-actor-palette.json').read_text()));p=q.getpalette();pal=[0]+[0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,189,3)]
blob=bytearray();meta=[];shared={};uncached=0
for name,frame,im,x,y in images:
 color=im.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE);alpha=im.getchannel('A');data=bytes(v+1 if a>=100 else 0 for v,a in zip(color.tobytes(),alpha.tobytes()));compressed=zlib.compress(data,9)[2:-4] if name=="wounded" else pack(data);assert (zlib.decompress(compressed,-15) if name=="wounded" else unpack(compressed,0,len(data)))==data
 key=(name=="wounded",im.width,im.height,data);uncached+=len(compressed)
 if key not in shared:shared[key]=len(blob);blob+=compressed
 meta.append((x,y,im.width,im.height,shared[key],len(compressed)))
print('Measured actor bytes',len(blob)+128+len(meta)*16,'payload',len(blob),'dedup saved',uncached-len(blob),'frames',len(meta))
if '--measure' in sys.argv:sys.exit()
h='#ifndef BARRACKS_ACTOR_ART_H\n#define BARRACKS_ACTOR_ART_H\n#include <stdint.h>\n#include <string.h>\n#include "asset_inflate.h"\ntypedef struct {int16_t x,y; uint16_t w,h; uint32_t offset,length;} BarracksActorFrame;\n'
h+='#define BARRACKS_WOUNDED_SOURCE_FIRST 161\n#define BARRACKS_WOUNDED_SOURCE_STEP 2\n'
for name,(start,count) in ranges.items():h+=f'#define BARRACKS_{name.upper()}_FIRST {start}\n#define BARRACKS_{name.upper()}_COUNT {count}\n'
h+=f'#define BARRACKS_ACTOR_MAX_PIXELS {max(m[2]*m[3] for m in meta)}\n'+array('barracks_actor_palette','uint16_t',pal)+array('barracks_actor_pixels','uint8_t',blob)
h+='static const BarracksActorFrame barracks_actor_frames[]={\n'+''.join('{'+','.join(map(str,m))+'},\n' for m in meta)+'};\n'
audit=json.loads((R/'generated/barracks-source-audit.json').read_text());actors=[];barrel=0
for p in audit['placements']:
 name={114:'trainer',150:'drums',184:'guitar',202:'cello',204:'drinker',165:'barrel'}.get(p['sprite'])
 if name:
  if name=='barrel':barrel+=1
  actors.append((int(p['world_x']+0.5),round(p['world_y']),ranges[name][0],ranges[name][1],barrel if name=='barrel' else 0))

for i,depth in enumerate(json.loads((R/'assets/barracks-furniture-depths.json').read_text())):
 actors.append((0,depth,ranges['furniture'][0]+i,1,0))
h+='typedef struct {int16_t x,y; uint8_t first,count,barrel;} BarracksActor;\n'+f'#define BARRACKS_ACTOR_COUNT {len(actors)}\nstatic const BarracksActor barracks_actors[]={{\n'+''.join('{'+','.join(map(str,a))+'},\n' for a in actors)+'};\n'
h+='static inline int barracks_actor_decode(unsigned frame,uint8_t *out){\n if(frame>=sizeof(barracks_actor_frames)/sizeof(*barracks_actor_frames))return 0;\n const BarracksActorFrame *f=&barracks_actor_frames[frame];\n if(frame>=BARRACKS_WOUNDED_FIRST&&frame<BARRACKS_WOUNDED_FIRST+BARRACKS_WOUNDED_COUNT)return asset_inflate(barracks_actor_pixels+f->offset,f->length,out,f->w*f->h);\n const uint8_t *src=barracks_actor_pixels+f->offset,*end=src+f->length;unsigned n=0,size=f->w*f->h;\n while(n<size){\n  if(src>=end)return 0;\n  unsigned token=*src++,count=(token&127)+1;if(count>size-n)return 0;\n  if(token&128){if(src>=end)return 0;memset(out+n,*src++,count);}\n  else {if((unsigned)(end-src)<count)return 0;memcpy(out+n,src,count);src+=count;}\n  n+=count;\n }\n return src==end;\n}\n#endif\n'
(R/'generated/barracks_actor_art.h').write_text(h);preview.save(R/'generated/barracks-actors-preview.png');print('actor bytes',len(blob)+128,'max pixels',max(m[2]*m[3] for m in meta), 'actors',actors)


# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
