"""Decrypt the Castle Crashers archives this project reads and dump the SWFs
the converters parse. Everything is written to assets/extracted (git-ignored).

  python tools/extract_game_files.py GAME_DIR FFDEC_JAR [--only NAME ...]

* data/game/<pak>.pak and data/levels/levelN.pak -> <PAK>.swf
* levelN: LEVELN-full.swf has the declared SWF length repaired (the installed
  length is stale and FFDec would otherwise drop the trailing tags).
* data/bsps/bspN.pak -> bspN.bin (decrypted collision payload)
* XML dumps (FFDec -swf2xml) named the way the converters expect.
"""
import argparse, concurrent.futures, os, subprocess, sys
from pathlib import Path
R = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(R / 'tools' / 'vendor'))
import decrypt_asset as dec

OUT = R / 'assets' / 'extracted'
GAME_PAKS = ['player', 'main', 'menu', 'menu_vignettes', 'fx', 'sounds', 'lobby', 'map', 'mount2', 'mount3',
             'ebarboss', 'ewarmach', 'ebat', 'etroll', 'etrollx', 'eimp', 'animals', 'sky1', 'horse', 'amenu', 'select', 'logo',
             'attract']
LEVELS = [1, 2, 4, 7, 9, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 22, 23, 24, 28, 41, 46, 47, 49]
# xml name -> swf name (all inside assets/extracted)
XML = {
    'player': 'PLAYER', 'main': 'MAIN', 'fx': 'FX', 'menu': 'MENU', 'sounds': 'SOUNDS', 'mount2': 'MOUNT2', 'mount3': 'MOUNT3',
    'ebarboss': 'EBARBOSS', 'ewarmach': 'EWARMACH', 'ebat': 'EBAT', 'etroll': 'ETROLL', 'etrollx': 'ETROLLX',
    'eimp': 'EIMP', 'horse': 'HORSE', 'pet-common-animals': 'ANIMALS', 'map-full': 'MAP-full',
    'level2': 'LEVEL2', 'level19': 'LEVEL19-full', 'level46': 'LEVEL46', 'level47': 'LEVEL47', 'level49': 'LEVEL49',
    'level13': 'LEVEL13-full',
    'level4-full': 'LEVEL4-full', 'level7-full': 'LEVEL7-full', 'level11-full': 'LEVEL11-full',
    'level12-full': 'LEVEL12-full', 'level16-full': 'LEVEL16-full', 'level18-full': 'LEVEL18-full',
    'level22-full': 'LEVEL22-full', 'level23-full': 'LEVEL23-full',
    'level14-audit': 'LEVEL14-audit', 'level15-audit': 'LEVEL15-audit',
}


def full_copy(name):
    b = bytearray((OUT / f'{name}.swf').read_bytes())
    assert b[:3] == b'FWS', name
    b[4:8] = len(b).to_bytes(4, 'little')
    (OUT / f'{name}-full.swf').write_bytes(b)


def hub_audit(lv):
    """Same as tools/prepare_hub_audit.py: neutralise custom tag 148 so FFDec can read the scene."""
    import struct
    b = (OUT / f'LEVEL{lv}-full.swf').read_bytes()
    p = 8 + (5 + (b[8] >> 3) * 4 + 7) // 8 + 4
    out = bytearray(b[:p])
    while p + 2 <= len(b):
        st = p; t = struct.unpack_from('<H', b, p)[0]; p += 2; typ = t >> 6; n = t & 63
        if n == 63: n = struct.unpack_from('<I', b, p)[0]; p += 4
        if typ == 148:
            symbol = struct.unpack_from('<H', b, p)[0]
            payload = struct.pack('<HHHH', symbol, 1, 64, 0)
            out += struct.pack('<H', (39 << 6) | len(payload)) + payload
        else:
            out += b[st:p + n]
        p += n
        if typ == 0: break
    struct.pack_into('<I', out, 4, len(out))
    (OUT / f'LEVEL{lv}-audit.swf').write_bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('game', type=Path)
    ap.add_argument('ffdec')
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--jobs', type=int, default=max(1, min(4, (os.cpu_count() or 2) // 2)))
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    data = a.game / 'data'
    if not a.only:
        for p in GAME_PAKS:
            dec.decrypt_file(data / 'game' / f'{p}.pak', OUT)
        for lv in LEVELS:
            dec.decrypt_file(data / 'levels' / f'level{lv}.pak', OUT)
            full_copy(f'LEVEL{lv}')
        full_copy('MAP')
        for lv in (14, 15):
            hub_audit(lv)
        for bsp in sorted((data / 'bsps').glob('bsp*.pak')):
            name, enc = dec.unzip_to_cok6_nrec(bsp)
            (OUT / (bsp.stem + '.bin')).write_bytes(dec.blowfish_decrypt(name, enc))
        print('decrypted', len(GAME_PAKS), 'game archives,', len(LEVELS), 'levels and the BSP payloads')
    env = os.environ.copy(); env['APPDATA'] = str(R / 'build' / 'appdata')
    todo = [k for k in XML if not a.only or k in a.only]

    def dump(k):
        p = subprocess.run(['java', '-Xmx3g', '-jar', a.ffdec, '-swf2xml', str(OUT / (XML[k] + '.swf')), str(OUT / (k + '.xml'))],
                           env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode: raise SystemExit(f'FFDec failed on {k}: ' + p.stdout.decode(errors='replace')[-800:])
        return k
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for k in ex.map(dump, todo):
            print('xml', k, flush=True)


if __name__ == '__main__':
    main()
