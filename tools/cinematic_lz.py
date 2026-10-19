"""Private byte-LZ format decoded by source/cinematic.c; independent of terrain codec."""
from collections import defaultdict,deque

def pack(data):
    from asset_codec import pack as encode
    return encode(data)

def unpack(data):
    from asset_codec import unpack as decode
    return decode(data)
