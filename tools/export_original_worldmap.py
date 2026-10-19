"""Re-export the adapted-color MAP source artwork with FFDec."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[1]
env = os.environ.copy()
env["APPDATA"] = str(ROOT / "build/appdata")
jar = Path(__import__('os').environ.get('FFDEC_JAR','C:/Program Files (x86)/FFDec/ffdec-cli.jar'))
xml = ROOT / "assets/extracted/original-worldmap.xml"
swf = xml.with_suffix(".swf")
for args in (
    ["-xml2swf", str(xml), str(swf)],
    ["-ignorebackground", "-export", "frame", str(ROOT / "assets/original-worldmap"), str(swf)],
):
    subprocess.run(["java", "-jar", str(jar), *args], env=env, check=True)
