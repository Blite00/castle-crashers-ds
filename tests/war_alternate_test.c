#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "war_alternate_art.h"
int main(void){
 unsigned char b[14338];
 for(unsigned i=0;i<WAR_ALTERNATE_COUNT;i++){
  const CampaignRouteLayer*f=&war_alternate_layers[i];
  unsigned n=(unsigned)f->w*f->h;assert(n<=14336);
  memset(b,0xa5,sizeof b);assert(war_alternate_decode(i,b+1));
  assert(b[0]==0xa5&&b[n+1]==0xa5);
  assert(f->x>=0&&f->x+f->w<=4608&&f->y>=0&&f->y+f->h<=448);
 }
 /* Each of the four cinema walls plays four source break keyframes (burst, planks, debris, sparse planks). */
 unsigned wall_depths[4]={261,282,301,315},key_seen[4][5]={{0}},gate_seen[7]={0},chips=0;
 for(unsigned i=0;i<WAR_ALTERNATE_COUNT;i++){
  const CampaignRouteLayer*f=&war_alternate_layers[i];
  for(unsigned w=0;w<4;w++)if(f->source==161&&(unsigned)f->depth==wall_depths[w]&&f->dynamic>=1&&f->dynamic<=4)key_seen[w][f->dynamic]++;
  if(f->source==133&&f->dynamic>=2&&f->dynamic<=6)gate_seen[f->dynamic]++;
  if(f->source==1180)chips++;
 }
 for(unsigned w=0;w<4;w++)for(unsigned k=1;k<=4;k++)assert(key_seen[w][k]>0);
 for(unsigned k=2;k<=6;k++)assert(gate_seen[k]>0); /* Source door frames 2..6 */
 assert(chips==4); /* Source door_shrapnel shapes */
 assert(!war_alternate_decode(WAR_ALTERNATE_COUNT,b));
 puts("Original broken fence slices decode with bounded scratch");return 0;
}
