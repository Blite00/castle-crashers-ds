"""Lossless byte RLE for menu coverage masks, directly drawn without scratch."""
from pathlib import Path
import re
R=Path(__file__).resolve().parents[1]
def pack(a):
 o=bytearray();i=0
 while i<len(a):
  n=1
  while i+n<len(a) and a[i+n]==a[i] and n<128:n+=1
  if n>=3:o.extend((127+n,a[i]));i+=n;continue
  j=i;i+=n
  while i<len(a) and i-j<128:
   n=1
   while i+n<len(a) and a[i+n]==a[i] and n<3:n+=1
   if n>=3:break
   i+=min(n,128-(i-j))
  o.append(i-j-1);o.extend(a[j:i])
 return o
def optimize():
 p=R/'generated/menu_art.c';s=p.read_text()
 if 'menu_draw_alpha(' in s:return
 saved=0
 for name,meta in [('menu_alpha','menu_labels'),('menu_font_alpha','menu_glyphs')]:
  m=re.search(r'const uint8_t '+name+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);raw=bytes(map(int,re.findall(r'\d+',m[1])))
  mm=re.search(r'const MenuLabel '+meta+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S);records=[tuple(map(int,v)) for v in re.findall(r'\{(\d+),(\d+),(\d+)\}',mm[1])];out=bytearray();new=[]
  for off,w,h in records:
   blob=pack(raw[off:off+w*h]);new.append((len(out),w,h));out.extend(blob)
   dec=bytearray();k=0
   while len(dec)<w*h:
    t=blob[k];k+=1;n=(t&127)+1
    if t&128:dec.extend([blob[k]]*n);k+=1
    else:dec.extend(blob[k:k+n]);k+=n
   assert dec==raw[off:off+w*h]
  (R/f'build/{name}-raw.bin').write_bytes(raw)
  (R/f'build/{name}-meta.txt').write_text(repr(records))
  s=s[:mm.start()]+'const MenuLabel '+meta+'['+str(len(new))+']={'+','.join('{%d,%d,%d}'%v for v in new)+'};'+s[mm.end():]
  m=re.search(r'const uint8_t '+name+r'\[[^]]*\]\s*=\s*\{(.*?)\};',s,re.S)
  s=s[:m.start()]+'static const uint8_t '+name+'['+str(len(out))+']={'+','.join(map(str,out))+'};'+s[m.end():]
  print(name,len(raw),'->',len(out));saved+=len(raw)-len(out)
 s+='''
void menu_draw_alpha(uint16_t *pixels,unsigned id,int font,int x,int y,uint16_t color){
 if(!pixels||id>=(font?95u:10u))return;
 const MenuLabel *m=font?menu_glyphs+id:menu_labels+id;
 const uint8_t *p=(font?menu_font_alpha:menu_alpha)+m->offset;
 unsigned pos=0,total=(unsigned)m->w*m->h,col=0,row=0;
 while(pos<total){
  unsigned t=*p++,n=(t&127)+1,repeat=t&128,a=repeat?*p++:0;
  if(repeat&&!a){pos+=n;row=pos/m->w;col=pos%m->w;continue;}
  for(unsigned i=0;i<n;i++){
   if(!repeat)a=*p++;
   int px=x+(int)col,py=y+(int)row;
   if(a&&(unsigned)px<256&&(unsigned)py<192){
    unsigned bg=pixels[py*256+px];
    unsigned r=((bg&31)*(255-a)+(color&31)*a+127)/255;
    unsigned g=(((bg>>5)&31)*(255-a)+((color>>5)&31)*a+127)/255;
    unsigned b=(((bg>>10)&31)*(255-a)+((color>>10)&31)*a+127)/255;
    pixels[py*256+px]=(uint16_t)(0x8000|r|(g<<5)|(b<<10));
   }
   pos++;if(++col==m->w){col=0;row++;}
  }
 }
}
''';p.write_text(s)
 p=R/'generated/menu_art.h';h=p.read_text();h=re.sub(r'extern const uint8_t menu_(?:font_)?alpha\[[^]]*\];','',h);h=h.replace('#endif','void menu_draw_alpha(uint16_t *pixels,unsigned id,int font,int x,int y,uint16_t color);\n#endif');p.write_text(h)
 print('Coverage bytes saved',saved)
if __name__=='__main__':optimize()
