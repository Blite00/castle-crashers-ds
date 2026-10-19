"""Repair the exported SWF's stale declared length without altering its tags."""
from pathlib import Path
R=Path(__file__).resolve().parents[1]
b=bytearray((R/'assets/extracted/LEVEL13.swf').read_bytes())
assert b[:3]==b'FWS', 'Expected uncompressed original level SWF'
b[4:8]=len(b).to_bytes(4,'little')
(R/'assets/extracted/LEVEL13-full.swf').write_bytes(b)
