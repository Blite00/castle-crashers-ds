from pathlib import Path
import subprocess,os
cc=__import__('host_cc').path();os.environ['PATH']=str(cc.parent)+os.pathsep+os.environ['PATH']
render=Path('source/render.c').read_text();render='unsigned war_diag_boss_tiles,war_diag_ram_tiles;\n'+render;render=render.replace('{if(!boss_actor_decode_tile','{war_diag_boss_tiles++;if(!boss_actor_decode_tile').replace('if(war_machine_decode_tile(id,world_foreground))','if((war_diag_ram_tiles++,war_machine_decode_tile(id,world_foreground)))');Path('build/war_diag_render.c').write_text(render)
sources='chapter_assets arena stage render weapon_art cinematic camera effects raster hud ui_skin menu campaign campaign_boss campaign_war campaign_forest campaign_hubs pets campaign_combat campaign_weapons bot protocol bottom_ui campaign_menu campaign_feedback item_selector'.split();assets='selector_art item_art loot_art hud_art menu_art'.split()
subprocess.run([str(cc),'-std=c11','-O2','-DRENDER_TEST_OVERLAY','-DCHAPTER_ASSETS_TEST','-Isource','-Igenerated','tools/war_diag_benchmark.c',*['build/war_diag_render.c' if s=='render' else f'source/{s}.c' for s in sources],*[f'generated/{s}.c' for s in assets],'-o','build/war_diag_benchmark.exe'],check=True)
with open('build/war_diag_benchmark.txt','w') as f:subprocess.run(['build/war_diag_benchmark.exe'],stdout=f,check=True)
print(Path('build/war_diag_benchmark.txt').read_text())
