"""Lossless RGB555 run encoding; draw directly with no scratch framebuffer."""
from pathlib import Path
import re,struct
R=Path(__file__).resolve().parents[1]
def optimize():
 p=R/'generated/hud_art.c';s=p.read_text();match=re.search(r'const uint16_t hud_pixels\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)
 if not match:return
 raw=list(map(int,re.findall(r'\d+',match[1])));m=re.search(r'const HudImage hud_images\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);flat=list(map(int,re.findall(r'\d+',m[1])));frames=[flat[i:i+3] for i in range(0,len(flat),3)];out=[];metadata=[]
 (R/'build/hud-pixels-reference.bin').write_bytes(struct.pack('<'+str(len(raw))+'H',*raw));(R/'build/hud-meta-reference.bin').write_bytes(struct.pack('<21H',*flat))
 for off,w,h in frames:
  data=raw[off:off+w*h];metadata.append((len(out),w,h));i=0;check=[]
  while i<len(data):
   run=1
   while i+run<len(data) and data[i+run]==data[i] and run<32767:run+=1
   if run>=2:out.extend([32768|run,data[i]]);check.extend([data[i]]*run);i+=run
   else:
    start=i;i+=1
    while i<len(data) and i-start<32767 and not(i+1<len(data) and data[i]==data[i+1]):i+=1
    out.append(i-start);out.extend(data[start:i]);check.extend(data[start:i])
  assert check==data
 s='#include "hud_art.h"\nstatic const uint16_t hud_packed['+str(len(out))+']={'+','.join(map(str,out))+'};\nconst HudImage hud_images[7]={'+','.join('{'+','.join(map(str,f))+'}' for f in metadata)+'};\n'
 s+='''void hud_draw_image(uint16_t *pixels,unsigned id,int x,int y,int width){
 if(id>=7||!pixels)return;
 const HudImage *f=&hud_images[id];if(width>f->w)width=f->w;if(width<=0)return;
 const uint16_t *src=hud_packed+f->offset;unsigned position=0,column=0,row=0,size=f->w*f->h;
 while(position<size){
  unsigned token=*src++,count=token&32767;uint16_t repeated=0;
  if(!count||count>size-position)return;
  if(token&32768){repeated=*src++;if(!repeated){position+=count;column+=count;row+=column/f->w;column%=f->w;continue;}}
  for(unsigned n=0;n<count;n++,position++){
   uint16_t color=token&32768?repeated:*src++;
   int dx=x+(int)column,dy=y+(int)row;
   if(color&&column<(unsigned)width&&dx>=0&&dx<256&&dy>=0&&dy<192)pixels[dy*256+dx]=color;
   if(++column==f->w){column=0;row++;}
  }
 }
}
''';p.write_text(s)
 p=R/'generated/hud_art.h';s=p.read_text();s=re.sub(r'extern const uint16_t hud_pixels\[[^]]*\];','void hud_draw_image(uint16_t *pixels,unsigned id,int x,int y,int width);',s);p.write_text(s)
 print('HUD lossless bytes',len(raw)*2,'->',len(out)*2,'saved',(len(raw)-len(out))*2)
if __name__=='__main__':optimize()
