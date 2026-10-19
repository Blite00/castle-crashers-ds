"""Move the menu and arena music (converted by convert_audio.py) to NitroFS and
record their sizes in generated/resident_music.h."""
from pathlib import Path
R = Path(__file__).resolve().parents[1]
out = R / 'nitrofiles' / 'audio'; out.mkdir(parents=True, exist_ok=True)
sizes = {}
for name in ('menu', 'arena'):
    data = (R / 'generated' / f'audio_{name}.adpcm').read_bytes()
    (out / f'{name}.bin').write_bytes(data)
    sizes[name] = len(data)
(R / 'generated' / 'resident_music.h').write_text(
    '#ifndef RESIDENT_MUSIC_H\n#define RESIDENT_MUSIC_H\n'
    '/* Menu and arena music live in NitroFS (menu.bin, arena.bin) and load into the chapter buffer on demand. */\n'
    f'#define RESIDENT_MUSIC_MENU_BYTES {sizes["menu"]}u\n#define RESIDENT_MUSIC_ARENA_BYTES {sizes["arena"]}u\n#endif\n')
print('resident music', sizes)
