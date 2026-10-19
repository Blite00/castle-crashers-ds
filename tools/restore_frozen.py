"""Copy the project's frozen converter inputs (data/frozen) into assets/.

These are small tables that the converters read but no longer regenerate, such
as a palette chosen once from a quantization run. They hold numbers only
(colors, offsets), no artwork, audio or text from the game."""
from pathlib import Path
import shutil
R = Path(__file__).resolve().parents[1]
(R / 'assets').mkdir(exist_ok=True)
for f in sorted((R / 'data' / 'frozen').iterdir()):
    shutil.copyfile(f, R / 'assets' / f.name)
    print('frozen input', f.name)
