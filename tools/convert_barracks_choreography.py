"""Import LEVEL2's authored camera and departing-knight timeline, plus original walk poses."""
from pathlib import Path
import json,re,sys
import xml.etree.ElementTree as E
from PIL import Image
from prepare_frames import freeze
from convert_barbarian import pack,array
R=Path(__file__).resolve().parents[1]

def prepare():
 r=E.parse(R/'assets/extracted/level2.xml').getroot();tags=r.find('tags');d={t.get('spriteId'):t for t in tags if t.get('spriteId')}
 state={};frames=[]
 for t in d['223'].find('subTags'):
  typ=t.get('type');depth=int(t.get('depth','0'))
  if typ.startswith('PlaceObject'):
   if t.find('matrix') is not None:state[depth]=t.find('matrix').attrib.copy()
  elif typ.startswith('RemoveObject'):state.pop(depth,None)
  elif typ=='ShowFrameTag':frames.append([[round((int(state[z]['translateX'])+25162)/40),round((int(state[z]['translateY'])+10892)/40)] if z in state else [-32768,0] for z in [1,17,33,49,65,81,97,113]])
 guide=[];position=(0,0)
 for t in d['366'].find('subTags'):
  if t.get('depth')=='1' and t.find('matrix') is not None:
   m=t.find('matrix');position=(round((int(m.get('translateX'))+8921)/40),round((int(m.get('translateY'))+11349)/40))
  if t.get('type')=='ShowFrameTag':guide.append(position)
 (R/'generated/barracks-choreography.json').write_text(json.dumps({'exit_sprite':223,'exit_starts':425,'exit_positions':frames,'camera_sprite':366,'camera_guide':guide},indent=2))
 for t in list(tags):
  if not t.get('type','').startswith('Define'):tags.remove(t)
 for selector in ['207','217','129']:freeze(d[selector],6)
 freeze(d['220'],3)
 for sid in ['13','16','59','84']:
  d[sid].find('subTags').clear();E.SubElement(d[sid].find('subTags'),'item',type='ShowFrameTag');d[sid].set('frameCount','1')
 p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId='222',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false');E.SubElement(p,'matrix',type='MATRIX',hasScale='false',hasRotate='false',translateX='2560',translateY='3200')
 for _ in range(16):E.SubElement(tags,'item',type='ShowFrameTag')
 r.set('frameCount','16')
 for k,v in dict(Xmin=0,Ymin=0,Xmax=5120,Ymax=3840).items():r.find('displayRect').set(k,str(v))
 E.ElementTree(r).write(R/'assets/extracted/barracks-exit.xml')

def convert():
 source=(R/'generated/barracks_actor_art.h').read_text()
 pal=list(map(int,re.search(r'barracks_actor_palette\[64\] = \{(.*?)\};',source,re.S)[1].replace('\n','').strip(',').split(',')))
 colors=[component for c in pal[1:] for component in ((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31)]
 q=Image.new('P',(1,1));q.putpalette(colors+colors[:3]*((768-len(colors))//3))
 data=[];frames=[];sheet=Image.new('RGBA',(128*8,96*2))
 for i in range(16):
  im=Image.open(R/f'assets/barracks-exit/{i+1}.png').convert('RGBA').resize((128,96),Image.Resampling.LANCZOS)
  a=im.getchannel('A').point(lambda x:255 if x>=100 else 0);box=a.getbbox();assert box
  x,y,right,bottom=box;crop=im.crop(box);indices=crop.convert('RGB').quantize(palette=q,dither=Image.Dither.NONE)
  pixels=bytes((v+1 if v<63 else 1) if alpha>=100 else 0 for v,alpha in zip(indices.tobytes(),crop.getchannel('A').tobytes()));assert max(pixels)<64
  encoded=pack(pixels);frames.append((x-64,y-80,right-x,bottom-y,len(data),len(encoded)));data.extend(encoded);sheet.paste(im,((i%8)*128,(i//8)*96))
 audit=json.loads((R/'generated/barracks-choreography.json').read_text())
 h='#ifndef BARRACKS_CHOREOGRAPHY_H\n#define BARRACKS_CHOREOGRAPHY_H\n'
 h+=array('barracks_exit_pixels','uint8_t',data)
 h+='static const BarracksActorFrame barracks_exit_frames[]={'+','.join('{'+','.join(map(str,m))+'}' for m in frames)+'};\n'
 h+='static const int16_t barracks_exit_positions[167][8][2]={'+','.join('{'+','.join('{'+','.join(map(str,p))+'}' for p in frame)+'}' for frame in audit['exit_positions'])+'};\n'
 h+='static const int16_t barracks_camera_guide[497][2]={'+','.join('{'+','.join(map(str,p))+'}' for p in audit['camera_guide'])+'};\n'
 h+='''static inline int barracks_exit_decode(unsigned frame,uint8_t *out){
 if(frame>=16)return 0;
 const BarracksActorFrame *f=&barracks_exit_frames[frame];
 const uint8_t *p=barracks_exit_pixels+f->offset,*end=p+f->length;unsigned n=0,size=f->w*f->h;
 while(p<end&&n<size){unsigned token=*p++,count=(token&127)+1;if(count>size-n)return 0;
  if(token&128){if(p==end)return 0;memset(out+n,*p++,count);}else{if((unsigned)(end-p)<count)return 0;memcpy(out+n,p,count);p+=count;}n+=count;
 }return n==size&&p==end;
}
#endif
'''
 (R/'generated/barracks_choreography.h').write_text(h);sheet.save(R/'generated/barracks-exit-preview.png')
 print('exit artwork',len(data),'source paths',167*8*4+497*4,'max frame',max(f[2]*f[3] for f in frames))

if __name__=='__main__':
 if '--prepare' in sys.argv:prepare()
 else:convert()

# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
