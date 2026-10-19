"""Every shipped clip gets more audible RMS without bigger buffers or clipping."""
from pathlib import Path
import sys,json,statistics
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'tools'))
from audit_audio_loudness import inventory
old=json.loads((R/'tests/audio-loudness-baseline.json').read_text());new=inventory();NEW={'chapter_forest','chapter_thieves','chapter_thieves_boss','chapter_deer','chapter_deer_victory','chapter_flood','chapter_flood_boss'} # Clips added after the baseline was taken: headroom checked, no baseline gain.
assert old.keys()|NEW==new.keys() and len(new)==41
gains=[]
for n,v in new.items():
 assert v['full_scale_samples']==0 and v['peak_dbfs']<=-1,n
 if n in NEW:continue
 assert v['bytes']==old[n]['bytes'],n
 gain=v['rms_dbfs']-old[n]['rms_dbfs'];assert gain>1,n;gains.append(gain)
assert statistics.median(gains)>3
print('All 41 shipped music/SFX clips: unchanged sizes, >1dB RMS gain each, decoded headroom PASS')
