"""Canaries: exported source inputs and exact selected raster frames remain pinned."""
from pathlib import Path
import hashlib,json
R=Path(__file__).resolve().parents[1]
for kind in ('pet','dig'):
 audit=json.loads((R/f'generated/{kind}-art-audit.json').read_text())
 for (folder,n),digest in zip(audit['source_frames'],audit['source_png_sha256']):
  assert hashlib.sha256((R/f'assets/{folder}/{n}.png').read_bytes()).hexdigest()==digest,(folder,n)
 assert len(audit['source_frames'])==audit['frames']
 assert audit['max_pixels']<=3393
print('Pet/dig selected source PNG hash canaries PASS')
