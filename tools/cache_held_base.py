"""Cache current base pixels through the native decoder, independent of its codec."""
from pathlib import Path
import os,subprocess,struct,pickle
R=Path(__file__).resolve().parents[1];build=R/'build';build.mkdir(exist_ok=True)
c='''#include <stdio.h>
#include <stdint.h>
#include "castle_art.h"
int main(int argc,char **argv){if(argc!=2)return 1;FILE *f=fopen(argv[1],"wb");if(!f)return 2;uint8_t pixels[KNIGHT_FRAME_PIXELS_MAX];fwrite(knight_palette,2,256,f);for(unsigned id=0;id<1216;id++){const ArtFrame *a=&knight_frames[id];int16_t meta[]={a->x,a->y,(int16_t)a->w,(int16_t)a->h};if(!knight_decode_frame(id,pixels))return 3;fwrite(meta,2,4,f);fwrite(pixels,1,a->w*a->h,f);}return fclose(f)!=0;}
'''
(build/'held-base-dump.c').write_text(c);gcc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(gcc.parent)+os.pathsep+env['PATH']
subprocess.run([str(gcc),'-O2','-std=c11','-Wall','-Wextra','-Werror','-I'+str(R/'generated'),str(build/'held-base-dump.c'),'-o',str(build/'held-base-dump.exe')],check=True,env=env)
subprocess.run([str(build/'held-base-dump.exe'),str(build/'held-base.bin')],check=True,env=env)
b=(build/'held-base.bin').read_bytes();palette=list(struct.unpack_from('<256H',b));offset=512;frames=[];pixels=[]
for _ in range(1216):
 x,y,w,h=struct.unpack_from('<4h',b,offset);offset+=8;frames.append((0,x,y,w,h));pixels.append(b[offset:offset+w*h]);offset+=w*h
assert offset==len(b)
(build/'held-base.pkl').write_bytes(pickle.dumps(dict(palette=palette,frames=frames,pixels=pixels)));print('Cached1216 baseframes using current native decoder')
