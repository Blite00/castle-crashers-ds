"""Registration audit for a new campaign level (docs/new-level-workflow.md, gate 1).

python tools/level_audit.py 14 WATER_MAP [bank]  list every place a level id has to appear and where it does not yet
python tools/level_audit.py                    audit the newest level that exists (the highest *_MAP in stage.h)

Earlier levels were wired by hand and the bugs came from the places that were forgotten: the packet validator that rejected the
new map id, the chapter bank chosen in two files, music chosen in main.c, the objective text on the bottom screen. This lists
those places from what the previous levels actually touched, so a new level cannot skip one silently."""
from pathlib import Path
import re,sys
R=Path(__file__).resolve().parents[1]
# (file, why a level must appear there)
TOUCH=[
 ('source/stage.h','the *_MAP constant and STAGE_MAP_COUNT'),
 ('source/stage.c','stage_position_open_map, stage_world_width, stage_camera_min_x (collision, size and the art edge)'),
 ('source/campaign.c','dispatch to the level step, group on exit, packet validation (map_id ceiling, phase range, spawn count)'),
 ('source/campaign_link.c','which chapter bank the guest loads for this map'),
 ('source/main.c','which chapter bank the host loads, and the level music'),
 ('source/audio.c','scene sounds and level ambience'),
 ('source/bottom_ui.c','objective text on the bottom screen'),
 ('source/render.c','level specific drawing (props, mounts, scripted scenery) and the left edge of the art'),
 ('source/protocol.c','projectile and fighter limits that differ per map'),
 ('tools/pack_chapter_assets.py','the chapter bank holding this level'),
 ('tests/level_contract_test.c','the level contract row (spawns, exit, art edge, wall columns, breakables)'),
]
CEILINGS=[r'>\s*THIEVES_MAP',r'<=\s*THIEVES_MAP',r'<\s*STAGE_MAP_COUNT',r'p->map_id>\s*1[0-9]']
def newest():
    s=(R/'source/stage.h').read_text();m=re.findall(r'#define\s+(\w+_MAP)\s+(\d+)',s)
    return max(((int(n),name) for name,n in m))
def main():
    number,name=(int(sys.argv[1]),sys.argv[2]) if len(sys.argv)>2 else newest()
    bank=sys.argv[3] if len(sys.argv)>3 else name.split('_')[0].lower()
    pat=re.compile(r'\b%s\b|(?:==|>=|<=|>|<)\s*%d\b|\b%d\b\s*(?:==|:)|case\s+%d\b|\{%d,"|[\'"]%s[\'"]'%(re.escape(name),number,number,number,number,re.escape(bank)))
    missing=0
    print(f'Level {name} = map {number}')
    for rel,why in TOUCH:
        p=R/rel
        if not p.exists():print(f'  MISSING FILE {rel}: {why}');missing+=1;continue
        hits=[i+1 for i,l in enumerate(p.read_text(errors="replace").split("\n")) if pat.search(l)]
        print(f'  {"ok     " if hits else "MISSING"} {rel}: {why}'+(f' (lines {", ".join(map(str,hits[:6]))}{"..." if len(hits)>6 else ""})' if hits else ''))
        missing+=0 if hits else 1
    print('Ceilings that name an earlier level and need to move to this one:')
    for p in sorted((R/'source').glob('*.c')):
        for i,l in enumerate(p.read_text(errors='replace').split('\n')):
            if any(re.search(c,l) for c in CEILINGS):print(f'  {p.name}:{i+1}: {l.strip()[:110]}')
    print('Result:',f'{missing} registration points missing' if missing else 'every registration point is present')
    return 1 if missing else 0
if __name__=='__main__':sys.exit(main())
