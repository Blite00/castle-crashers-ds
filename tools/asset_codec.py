"""Lossless raw DEFLATE assets; generated decoder requires no heap or extra window."""
from pathlib import Path
import zlib
R=Path(__file__).resolve().parents[1]
(R/'generated/asset_inflate.h').write_bytes((R/'tools/asset_inflate.h').read_bytes())
def pack(data):
 c=zlib.compressobj(9,wbits=-15)
 return c.compress(bytes(data))+c.flush()
def unpack(data):
 return zlib.decompress(data,wbits=-15)
