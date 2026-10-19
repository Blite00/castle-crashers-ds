from pathlib import Path
import subprocess,os
r=Path.cwd();cc=__import__('host_cc').path();os.environ['PATH']=str(cc.parent)+os.pathsep+os.environ['PATH']
sources='arena stage render weapon_art cinematic camera effects raster hud ui_skin menu campaign campaign_boss campaign_war campaign_forest campaign_hubs pets campaign_combat campaign_weapons bot protocol bottom_ui campaign_menu campaign_feedback item_selector'.split();assets='selector_art item_art loot_art hud_art menu_art'.split()
subprocess.run([str(cc),'-std=c11','-O2','-Isource','-Igenerated','tools/war_diag_route.c',*[f'source/{s}.c' for s in sources],*[f'generated/{s}.c' for s in assets],'-o','build/war_diag_route.exe'],check=True)
with open('build/war_diag_route.txt','w') as f:subprocess.run(['build/war_diag_route.exe'],stdout=f,check=True)
