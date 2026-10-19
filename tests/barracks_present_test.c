/* Compile once normally and once with RENDER_TEST_OVERLAY. Their output hashes
 * must match: the entire intro is presented from its opaque actor page, while
 * live exploration composes the committed terrain plus transparent overlay. */
#include "render.h"
#include "campaign.h"
#include "stage.h"
#include "barracks_rail.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint16_t terrain[512*512/2],palette[256],pixels[256*192+2];
static unsigned terrain_hash(void){unsigned h=2166136261u;for(unsigned i=0;i<512*512/2;i++)h=(h^terrain[i])*16777619u;for(unsigned i=0;i<256;i++)h=(h^palette[i])*16777619u;return h;}
static void record(FILE *out){
 unsigned hash=2166136261u;const ArenaCamera *c=arena_render_background_camera();
 for(unsigned y=0;y<192;y++)for(unsigned x=0;x<256;x++){
  unsigned color=pixels[y*256+x];
  if(!(color&0x8000)){
   unsigned bx=(c->x+x*c->width/256)&511,by=c->y+y*c->height/192;
   unsigned pair=terrain[by*256+bx/2];color=palette[(pair>>((bx&1)*8))&255];
  }
  hash=(hash^color)*16777619u;
 }
 assert(fwrite(&hash,sizeof hash,1,out)==1);
 assert(pixels[256*192]==0x1234&&pixels[256*192+1]==0x5678);
}
int main(int argc,char **argv){
 assert(argc==2);FILE *out=fopen(argv[1],"wb");assert(out);
 Arena a;arena_init(&a);a.map_id=5;a.round=5;a.p[0].x=336;a.p[0].y=284;a.p[1].ready=0;
 Campaign c={0};c.phase=KEEP_BARRACKS_INTRO;c.profiles[0].level=1;
 arena_render_campaign(&c);memset(terrain,0x5a,sizeof terrain);arena_upload_background(terrain,palette,5);
 unsigned original=terrain_hash();pixels[256*192]=0x1234;pixels[256*192+1]=0x5678;
 for(unsigned t=0;t<486;t++){
  a.tick=t;c.timer=t;arena_render(&a,pixels);assert(arena_render_is_opaque());
  /* No hidden commits: changing RAM strips may never modify active VRAM. */
  assert(terrain_hash()==original);
  const uint16_t *left=arena_dirty_left(),*right=arena_dirty_right();
  for(unsigned y=0;y<192;y++){
   assert(left[y]==0&&right[y]==256);
   for(unsigned x=0;x<256;x++)assert(pixels[y*256+x]&0x8000);
  }
  record(out);
 }
 /* While the front page still covers the screen, upload terrain, then expose
    both the prepared camera and transparent page in the same presentation. */
 c.phase=KEEP_BARRACKS_TRAVEL;c.timer=0;
 for(unsigned t=0;t<30;t++){
  a.tick++;a.p[0].x+=2;arena_render(&a,pixels);assert(!arena_render_is_opaque());
  arena_present_background();record(out);
  for(unsigned i=320*256;i<512*256;i++)assert(terrain[i]==0x5a5a);
 }
 /* Compare rail pixels with/without the hero, retaining legitimate
    furniture in front. Force identical camera framing for each comparison. */
 static uint16_t with_hero[256*192];
 for(int z=0;z<=40;z+=20){
  a.p[0].x=1000;a.p[0].y=243;a.p[0].z=z;a.p[0].pose=POSE_WALK;a.p[0].ready=1;a.round++;
  arena_render(&a,pixels);arena_present_background();
  const ArenaCamera *bg=arena_render_background_camera();
  for(int y=0;y<192;y++)for(int x=0;x<256;x++){
   unsigned bx=(bg->x+x*bg->width/256)&511,by=bg->y+y*bg->height/192;
   unsigned pair=terrain[by*256+bx/2];
   with_hero[y*256+x]=(pixels[y*256+x]&0x8000)?pixels[y*256+x]:palette[(pair>>((bx&1)*8))&255];
  }
  record(out);a.p[0].ready=0;a.round++;arena_render(&a,pixels);arena_present_background();
  const ArenaCamera *camera=arena_render_camera();bg=arena_render_background_camera();unsigned covered=0;
  for(int y=0;y<192;y++)for(int x=0;x<256;x++){
   int wx=camera->x+x*camera->width/256,wy=camera->y+y*camera->height/192;
   if(!barracks_rail_pixel(wx,wy))continue;
   unsigned bx=(bg->x+x*bg->width/256)&511,by=bg->y+y*bg->height/192;
   unsigned pair=terrain[by*256+bx/2];
   unsigned color=(pixels[y*256+x]&0x8000)?pixels[y*256+x]:palette[(pair>>((bx&1)*8))&255];
   assert(with_hero[y*256+x]==color);covered++;
  }
  assert(covered>30);
 }
 fclose(out);puts("Barracks opaque intro:486 images, no live terrain writes, full damage and transparent handoff and rail contact passed");return 0;
}
