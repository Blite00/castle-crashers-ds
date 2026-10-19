from pathlib import Path
import re
R=Path(__file__).resolve().parents[1]
def pack(b):
 out=bytearray();i=0
 while i<len(b):
  run=1
  while i+run<len(b) and b[i+run]==b[i] and run<128:run+=1
  if run>=3:out.extend((128+run-1,b[i]));i+=run
  else:
   start=i;i+=run
   while i<len(b) and i-start<128:
    run=1
    while i+run<len(b) and b[i+run]==b[i] and run<128:run+=1
    if run>=3:break
    i+=min(run,128-(i-start))
   out.append(i-start-1);out.extend(b[start:i])
 return out
def convert():
 p=R/'generated/campaign_weapon_art.c';s=p.read_text()
 match=re.search(r'const uint8_t campaign_weapon_pixels\[6\]\[1024\]=\{(.*?)\};',s,re.S)
 if not match:return
 raw=bytes(map(int,re.findall(r'\d+',match[1])));assert len(raw)==6144
 offsets=[];data=bytearray()
 for i in range(6):offsets.append(len(data));data.extend(pack(raw[i*1024:(i+1)*1024]))
 offsets.append(len(data))
 new='static const uint8_t weapon_icon_data[]={'+','.join(map(str,data))+'};\nstatic const uint16_t weapon_icon_offsets[]={'+','.join(map(str,offsets))+'};\n'
 new+='int campaign_weapon_decode(unsigned slot,uint8_t *out){if(slot>=6)return 0;unsigned at=weapon_icon_offsets[slot],end=weapon_icon_offsets[slot+1],n=0;while(n<1024){if(at>=end)return 0;unsigned token=weapon_icon_data[at++],count=(token&127)+1;if(count>1024-n)return 0;if(token&128){if(at>=end)return 0;memset(out+n,weapon_icon_data[at++],count);}else{if(count>end-at)return 0;memcpy(out+n,weapon_icon_data+at,count);at+=count;}n+=count;}return at==end;}\n'
 s=s[:match.start()]+new+s[match.end():];p.write_text('#include <string.h>\n'+s)
 p=R/'generated/campaign_weapon_art.h';s=p.read_text().replace('extern const uint8_t campaign_weapon_pixels[6][1024];','int campaign_weapon_decode(unsigned slot,uint8_t *out);');p.write_text(s)
 (R/'build/weapon-icon-reference.bin').write_bytes(raw)
 print('Weapon icons saved',6144-len(data)-10,'bytes')
if __name__=='__main__':convert()
