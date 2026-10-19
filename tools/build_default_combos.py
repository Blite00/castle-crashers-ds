"""Rebuild the appended combo bank from cached original source exports."""
from pathlib import Path
import subprocess,sys
R=Path(__file__).resolve().parents[1]
for script in ('measure_default_combos.py','factor_default_combos.py','encode_default_combos.py'):
 subprocess.run([sys.executable,str(R/'tools'/script)],check=True,cwd=R)
