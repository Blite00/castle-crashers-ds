"""Source camera/control timelines; these movies contain guides, not actor art."""
from pathlib import Path
from copy import deepcopy
import xml.etree.ElementTree as E
import json
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
def guides(level,sid,count):
 r=E.parse(R/f'assets/extracted/level{level}-full.xml').getroot();s=next(t for t in r.find('tags') if t.get('spriteId')==str(sid));out=[]
 for f in range(1,count+1):
  q=deepcopy(s);freeze(q,f)
  p=next((t for t in q.find('subTags') if t.get('name')=='guide'),None)
  if p is None:
   # Original bridge releases its camera at43 then removes the guide.
   # Retain the last position for callers inspecting the tail after release.
   assert out
   out.append(out[-1]);continue
  m=p.find('matrix');out.append((round(int(m.get('translateX','0'))/40),round(int(m.get('translateY','0'))/40)))
 return out
machine=guides(11,364,126);bridge=guides(12,85,46);base=machine[1];machine=[(x-base[0],y-base[1])for x,y in machine]
h='''#ifndef WAR_CHOREOGRAPHY_H
#define WAR_CHOREOGRAPHY_H
#include <stdint.h>
/* Original control-only clips363/364/85. sourceframe=timer+1. */
#define WAR_HORSE_STORY_FRAMES 245u
#define WAR_MACHINE_STORY_FRAMES 126u
#define WAR_BRIDGE_STORY_FRAMES 46u
typedef struct {int x,y;unsigned relative;} WarStoryCamera;
'''
h+='static const int16_t war_machine_camera_offset[126][2]={'+','.join('{%d,%d}'%v for v in machine)+'};\n'
h+='static const int16_t war_bridge_camera_offset[46][2]={'+','.join('{%d,%d}'%v for v in bridge)+'};\n'
h+='''/* Horse relative motion is authored f_HorseScroll (20sourcepx/tick), then
 * source speed_y=-100 and gravity+3 afterframe128. Caller must release the
 * scripted camera at sourceframe238, and uses its captured entry origin. */
static inline WarStoryCamera war_horse_camera(unsigned timer){
 unsigned f=timer+1;if(f>245)f=245;
 int horizontal=f<2?0:(int)(f<128?f-2:125)*10;
 int n=f>128?(int)f-128:0;
 WarStoryCamera c={horizontal,(-100*n+3*n*(n-1)/2)/2,1};return c;
}
static inline WarStoryCamera war_machine_story_camera(unsigned timer){
 if(timer>=126)timer=125;
 WarStoryCamera c={war_machine_camera_offset[timer][0],war_machine_camera_offset[timer][1],1};return c;
}
static inline WarStoryCamera war_bridge_story_camera(unsigned timer){
 if(timer>=46)timer=45;
 /* Original scene94 movie85 placement(-344.65,-392.5), origin(-1024,-800). */
 WarStoryCamera c={340+war_bridge_camera_offset[timer][0],204+war_bridge_camera_offset[timer][1],0};return c;
}
static const uint8_t war_horse_runner_source_frames[11]={15,21,27,33,39,45,51,57,63,69,75};
static const int8_t war_horse_runner_y_offsets[11]={0,10,-5,5,15,0,7,-7,2,-4,12};
/* Original fieldwall destruction order; sourceframe2 appears once broken. */
static const uint8_t war_machine_wall_source_frames[4]={77,75,76,78};
#endif
'''
(R/'generated/war_choreography.h').write_text(h)
(R/'generated/war-choreography-audit.json').write_text(json.dumps(dict(source='LEVEL11 sprite363/364; LEVEL12 sprite85',camera_only=True,machine_source_range=[1,126],bridge_source_range=[1,46],horse_camera='runtime f_HorseScroll then ballistic guide; camera reset238',machine_camera='relative to f_CenterCinemaCam entry position',horse_events={'runner_frames':[15,21,27,33,39,45,51,57,63,69,75],'cannon':113,'nuke':128,'camera_reset':238,'stop':245},machine_events={'spawn':45,'music_intro':59,'walls':{'2':75,'3':76,'1':77,'4':78},'release':126},bridge_events={'wait':2,'fadeout':34,'release':43,'fadein':44,'stop':46}),indent=2))
print('Authored guide tables:',len(machine),'machine,',len(bridge),'bridge; horse runtime trajectory exposed')
