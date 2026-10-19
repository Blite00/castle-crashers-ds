"""Host C compiler used by converters and host tests: $CC, the portable
MSYS2 gcc next to the project (../.tools), or gcc/cc on PATH."""
import os, shutil
from pathlib import Path

def path(root=None):
    root = Path(root or Path(__file__).resolve().parents[1])
    if os.environ.get('CC'):
        return Path(shutil.which(os.environ['CC']) or os.environ['CC'])
    msys = [Path(os.environ['MSYS2_ROOT'])] if os.environ.get('MSYS2_ROOT') else []
    msys += [root.parent / '.tools/msys64', root / '.tools/msys64', Path('C:/msys64')]
    for p in [m / 'ucrt64/bin/gcc.exe' for m in msys]:
        if p.is_file():
            return p
    found = shutil.which('gcc') or shutil.which('cc') or shutil.which('clang')
    if not found:
        raise SystemExit('No host C compiler found. Windows: install MSYS2 and run  pacman -S mingw-w64-ucrt-x86_64-gcc  '
                         '(or set MSYS2_ROOT); macOS: xcode-select --install; Linux: install gcc. Or set CC.')
    return Path(found)
