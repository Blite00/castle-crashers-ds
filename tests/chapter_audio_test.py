"""Original new-chapter ADPCM source/length/headroom canaries."""
from pathlib import Path
import json,hashlib,sys
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'tools'))
from convert_audio import decode,levels
for row in json.loads((R/'generated/chapter-audio-audit.json').read_text()):
 source=Path(row['source']);assert hashlib.sha256(source.read_bytes()).hexdigest()==row['source_sha256']
 bank=row.get('bank', 'war' if row['name'].startswith('war_') else row['name'])
 raw=(R/f'nitrofiles/audio/chapter-{bank}.bin').read_bytes();clip=raw[row['offset']:row['offset']+row['bytes']]
 assert len(clip)==row['bytes'] and not row['offset']%32 and not len(clip)%4
 decoded=decode(clip);assert levels(decoded)['peak_dbfs']<=-0.99
 assert abs(len(decoded)/row['rate']-row['seconds'])<.003
print('Chapter audio: original source hashes, full durations, ADPCM headers, alignment and headroom PASS')
