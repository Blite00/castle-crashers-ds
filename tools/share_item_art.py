"""Share original item art between HUD and effects without duplicated ROM/RAM arrays."""
from pathlib import Path
import re,hashlib
R=Path(__file__).resolve().parents[1]
p=R/'generated/item_art.h';s=p.read_text();definitions=[]
for name in ('item_palette','item_pixels','item_frames'):
 m=re.search(r'static const (\w+) '+name+r'\[\]\s*=\s*\{.*?\};',s,re.S)
 if not m:continue
 definition=m[0].replace('static const','const',1);definitions.append(definition)
 s=s[:m.start()]+'extern const '+m[1]+' '+name+'[];'+s[m.end():]
if definitions:
 assert len(definitions)==3
 (R/'generated/item_art.c').write_text('#include "item_art.h"\n'+'\n'.join(definitions)+'\n');p.write_text(s)
 print('Shared item art: removed duplicate3947pixel +512palette bytes, unchangeddata',hashlib.sha256(''.join(definitions).encode()).hexdigest())
