from pathlib import Path
import struct
for lv in [14,15]:
 b=Path(f'assets/extracted/LEVEL{lv}-full.swf').read_bytes();p=8+(5+(b[8]>>3)*4+7)//8+4;out=bytearray(b[:p])
 while p+2<=len(b):
  st=p;t=struct.unpack_from('<H',b,p)[0];p+=2;typ=t>>6;n=t&63
  if n==63:n=struct.unpack_from('<I',b,p)[0];p+=4
  if typ==148:
   symbol=struct.unpack_from('<H',b,p)[0];payload=struct.pack('<HHHH',symbol,1,64,0);out+=struct.pack('<H',(39<<6)|len(payload))+payload
   print(lv,'source customtag148 symbol',symbol,'replaced ONLY for readable scene audit')
  else:out+=b[st:p+n]
  p+=n
  if typ==0:break
 struct.pack_into('<I',out,4,len(out));Path(f'assets/extracted/LEVEL{lv}-audit.swf').write_bytes(out)
