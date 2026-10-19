"""Compare culling with complete depth-layer rendering; temporary images only."""
from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parents[1];os.chdir(R)
gcc=__import__('host_cc').path(R);env=os.environ.copy();env['PATH']=str(gcc.parent)+os.pathsep+env['PATH']
sources=['tools/render_preview.c','source/arena.c','source/stage.c','source/render.c','tests/thieves_stubs.c','source/weapon_art.c','generated/selector_art.c','source/cinematic.c','source/camera.c','source/effects.c','source/raster.c','source/hud.c','source/ui_skin.c','source/menu.c','generated/item_art.c','generated/loot_art.c','generated/hud_art.c','generated/menu_art.c']
with tempfile.TemporaryDirectory(prefix='foreground-',dir=R/'build') as tmp:
 p=Path(tmp)
 for name,flags in [('cull',[]),('full',['-DRENDER_DISABLE_FOREGROUND_CULL'])]:
  subprocess.run([str(gcc),'-O2','-std=c11','-Wall','-Wextra','-Werror','-Isource','-Igenerated',*flags,*sources,'-o',str(p/(name+'.exe'))],env=env,check=True)
 for scenario in (24,25,26,27,28,30,31,36,40,41,42,43,49,50):
  for name in ('cull','full'):subprocess.run([str(p/(name+'.exe')),str(p/(name+'.ppm')),str(scenario)],env=env,check=True,stdout=subprocess.DEVNULL)
  assert (p/'cull.ppm').read_bytes()==(p/'full.ppm').read_bytes(),scenario
print('Foreground culling:14 Keep/co-op/effect scenes pixel-identical to complete depth layers')
