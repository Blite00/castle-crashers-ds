#include "campaign.h"
#include "campaign_boss.h"
#include "boss_gold.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static void begin(Campaign*c,Arena*a,int coop){memset(c,0,sizeof(*c));arena_lobby_init(a);a->p[0].ready=1;a->p[1].ready=coop;campaign_start(c,a,coop);campaign_boss_start(c,a);}
static void step(Campaign*c,Arena*a,unsigned n){uint16_t keys[2]={0};while(n--)campaign_step(c,a,keys);}
static void force_mode(Campaign*c,Arena*a,unsigned mode,unsigned before){c->phase=BOSS_FIGHT;c->war.mode=mode;c->war.mode_timer=before;c->war.summon_timer=60000;for(int i=0;i<3;i++)c->war.barrel_timer[i]=255;c->war.machine.x=500;c->war.machine.y=315;c->war.machine.facing=-1;a->p[0].x=440;a->p[0].y=315;a->p[0].z=0;a->p[0].invulnerable=0;a->p[0].hp=campaign_health(c,0);}
static void burial_check(void){
 Campaign c;Arena a;begin(&c,&a,1);force_mode(&c,&a,BOSS_SLAM,36);a.p[0].x=370;a.p[1].x=850;
 step(&c,&a,1);assert(campaign_boss_buried(&c,0)==8&&!campaign_boss_buried(&c,1));
 int x=a.p[0].x,y=a.p[0].y,hp=a.p[0].hp;uint16_t keys[2]={ATTACK|ATTACK_EDGE|RIGHT,0};
 for(int i=0;i<29;i++){campaign_step(&c,&a,keys);assert(a.p[0].x==x&&a.p[0].y==y&&a.p[0].hp==hp&&campaign_boss_buried(&c,0)==8);}
 keys[0]=LEFT;campaign_step(&c,&a,keys);assert(campaign_boss_buried(&c,0)==8); /* Wrong first direction. */
 for(int n=0;n<8;n++){
  keys[0]=n&1?LEFT:RIGHT;campaign_step(&c,&a,keys);assert(campaign_boss_buried(&c,0)==7u-n);
  CampaignPacket p;Campaign guest={0};Arena replica;campaign_pack(&p,&c,&a,n);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&replica);
  assert(campaign_boss_buried_frame(&guest,0)==campaign_boss_buried_frame(&c,0));
  CampaignPacket bad=p;bad.actors[0].animation=42;assert(!campaign_packet_valid(&bad));
 }
 step(&c,&a,4);assert(!campaign_boss_buried_frame(&c,0)&&a.p[0].z>0&&a.p[0].invulnerable>0);
 c.break_hp[14]=3;c.break_hit_seq[15][0]=32;c.war.machine.hp=0;step(&c,&a,1);assert(c.phase==BOSS_DEATH&&!c.break_hp[14]&&!c.break_hit_seq[15][0]);
 c.break_hit_seq[15][0]=33;campaign_boss_start(&c,&a);assert(!campaign_boss_buried_frame(&c,0)&&!c.break_hit_seq[15][0]);
 begin(&c,&a,0);force_mode(&c,&a,BOSS_SLAM,36);a.p[0].x=370;a.p[0].z=50;a.p[0].vz=0;step(&c,&a,1);assert(!campaign_boss_buried(&c,0));
}
static void locomotion_check(void){
 Campaign c;Arena a;begin(&c,&a,0);step(&c,&a,289);
 assert(c.phase==BOSS_INTRO&&c.war.machine.x==608&&c.war.machine.pose==POSE_IDLE&&c.war.machine.attack_kind==BOSS_IDLE);
 force_mode(&c,&a,BOSS_IDLE,0);step(&c,&a,1);
 assert(c.war.machine.x==500&&c.war.machine.pose==POSE_IDLE&&c.war.machine.attack_kind==BOSS_IDLE);
 a.p[0].x=300;step(&c,&a,12);assert(c.war.machine.x==488&&c.war.machine.pose==POSE_WALK);
 a.p[0].x=700;step(&c,&a,12);assert(c.war.machine.x==500&&c.war.machine.pose==POSE_WALK&&c.war.machine.facing==1);
 force_mode(&c,&a,BOSS_IDLE,0);a.p[0].x=450;a.p[0].y=285;step(&c,&a,12);
 assert(c.war.machine.y==309&&c.war.machine.pose==POSE_WALK);
 CampaignPacket packet;Campaign guest={0};Arena replica;campaign_pack(&packet,&c,&a,1);assert(campaign_packet_valid(&packet));campaign_unpack(&packet,&guest,&replica);
 assert(guest.war.machine.x==c.war.machine.x&&guest.war.machine.y==309&&guest.war.machine.attack_kind==BOSS_WALK);
}
/* Source f_BarbarianBoss scheduler (raw LEVEL7 bytecode). */
static void scheduler_check(void){
 Campaign c;Arena a;
 /* Reaching his spot he punches when the 60-tick punch timer is ready, then waits for it again. */
 begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);c.war.barrel_timer[1]=0;step(&c,&a,1);
 assert(c.war.mode==BOSS_PUNCH&&c.war.barrel_timer[1]==60);
 step(&c,&a,40);assert(c.war.mode==BOSS_IDLE);step(&c,&a,10);assert(c.war.mode==BOSS_IDLE&&c.war.barrel_timer[1]>=40); /* Timers pause during the attack. */
 step(&c,&a,60);assert(c.war.mode==BOSS_PUNCH);
 /* After the action timer expires he slams toward a hero in front (225 reach, 10 lane), then uses spikes next time. */
 begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);a.p[0].x=290;c.war.barrel_timer[0]=1;c.war.barrel=0;step(&c,&a,1);
 assert(c.war.mode==BOSS_SLAM&&c.war.barrel==1&&c.war.barrel_timer[0]==90);
 begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);a.p[0].x=250;c.war.barrel_timer[0]=1;c.war.barrel=1;step(&c,&a,1);
 assert(c.war.mode==BOSS_SPIKES&&c.war.barrel==0);
 /* Out of reach or lane: no attack, the timer stays expired. */
 begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);a.p[0].x=100;c.war.barrel_timer[0]=1;c.war.barrel=0;step(&c,&a,1);assert(c.war.mode!=BOSS_SLAM&&!c.war.barrel_timer[0]);
 /* At or below half health he may breathe fire instead of punching, never above half. */
 unsigned fired=0,punched=0;
 for(int seed=0;seed<40;seed++){
  begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);c.war.barrel_timer[1]=0;c.war.barrel_timer[2]=0;c.war.machine.hp=c.war.gate_hp/2;c.rng=seed*7919u;step(&c,&a,1);
  fired+=c.war.mode==BOSS_FIRE;punched+=c.war.mode==BOSS_PUNCH;
 }
 assert(fired>5&&punched>5);
 begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);c.war.barrel_timer[1]=0;c.war.barrel_timer[2]=0;step(&c,&a,1);assert(c.war.mode==BOSS_PUNCH);
 /* Reinforcement caps: 1 above 70% health, 2 at or below 70%, 3 at or below 40%. */
 for(int stage=0;stage<3;stage++){
  begin(&c,&a,0);force_mode(&c,&a,BOSS_IDLE,0);c.war.machine.hp=stage==0?c.war.gate_hp:stage==1?c.war.gate_hp*7/10:c.war.gate_hp*4/10;
  a.p[0].x=100;a.p[0].y=250;
  for(int t=0;t<900;t++){c.war.summon_timer=0;c.war.barrel_timer[0]=c.war.barrel_timer[1]=c.war.barrel_timer[2]=255;step(&c,&a,1);}
  unsigned alive=0;for(int i=0;i<6;i++)alive+=c.enemies[i].ready&&c.enemies[i].hp>0;
  assert(alive==(unsigned)(stage+1));
 }
}
int main(void){
 scheduler_check();
 locomotion_check();
 burial_check();
 Campaign c;Arena a;begin(&c,&a,0);assert(a.map_id==11&&c.war.machine.hp==350);assert(stage_position_open_map(11,a.p[0].x,a.p[0].y));
 {Campaign host;Arena actors;begin(&host,&actors,1);host.phase=BOSS_FIGHT;actors.clash_timer=90;actors.clash_score[0]=3;actors.clash_score[1]=2;actors.p[1].pose=POSE_CLASH;host.enemies[0]=(Fighter){.x=310,.y=315,.ready=1,.hp=50,.pose=POSE_CLASH,.facing=-1};
  CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,&host,&actors,3);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&remote);assert(remote.clash_timer&&remote.p[1].pose==POSE_CLASH&&remote.clash_score[0]==3&&remote.clash_score[1]==2&&guest.enemies[0].pose==POSE_CLASH);}
 step(&c,&a,178);assert(c.phase==BOSS_INTRO&&!c.war.machine.ready);step(&c,&a,1);assert(c.war.machine.ready);step(&c,&a,120);assert(c.phase==BOSS_FIGHT&&c.war.machine.x<=608);
 force_mode(&c,&a,BOSS_PUNCH,24);int hp=a.p[0].hp;step(&c,&a,1);assert(a.p[0].hp==hp);step(&c,&a,1);assert(a.p[0].hp<hp);hp=a.p[0].hp;step(&c,&a,5);assert(a.p[0].hp==hp);
 force_mode(&c,&a,BOSS_PUNCH,25);a.p[0].y=355;hp=a.p[0].hp;step(&c,&a,1);assert(a.p[0].hp==hp); /* depth dodge */
 force_mode(&c,&a,BOSS_PUNCH,25);a.p[0].x=560;hp=a.p[0].hp;step(&c,&a,1);assert(a.p[0].hp==hp); /* wrong side */
 force_mode(&c,&a,BOSS_PUNCH,25);c.enemies[0]=(Fighter){.x=440,.y=315,.hp=1,.ready=1,.facing=1};a.p[0].x=600;step(&c,&a,1);assert(!c.enemies[0].hp&&c.enemies[0].ready); /* Boss damage_all, corpse remains. */
 force_mode(&c,&a,BOSS_IDLE,0);c.war.machine.hp=0;step(&c,&a,1);assert(c.phase==BOSS_DEATH&&(c.completed&4));step(&c,&a,73);assert(c.phase==BOSS_TREASURE&&!c.war.machine.ready);step(&c,&a,320);assert(c.phase==BOSS_PRINCESS&&(c.war.flags&BOSS_CHEST_OPEN));assert(c.pending_count==0);BossGold gold;unsigned available=0;for(unsigned i=0;i<BOSS_GOLD_COUNT;i++)available+=campaign_boss_gold(&c,i,&gold);assert(available>20);
 c.phase=BOSS_RELEASE;c.timer=32;step(&c,&a,1);assert(c.phase==BOSS_KISS);step(&c,&a,400);assert(c.phase==BOSS_COMPLETE&&a.winner==1);assert(abs(a.p[c.group].x-(BOSS_PRINCESS_X-BOSS_KISS_OFFSET_X))<=2&&abs(a.p[c.group].y-(BOSS_PRINCESS_Y+1))<=2);
 begin(&c,&a,1);assert(c.war.machine.hp==450);c.phase=BOSS_RELEASE;c.timer=32;a.shots[0]=(Projectile){.life=10,.owner=8};step(&c,&a,1);assert(c.phase==BOSS_DUEL&&a.countdown==83&&!a.shots[0].life);assert(a.p[0].hp==campaign_health(&c,0)&&a.p[1].hp==campaign_health(&c,1));step(&c,&a,83);a.p[0].hp=0;step(&c,&a,1);assert(c.phase==BOSS_KISS&&c.group==1&&!a.winner);step(&c,&a,400);assert(c.phase==BOSS_COMPLETE&&a.winner==1);assert(abs(a.p[c.group].x-(BOSS_PRINCESS_X-BOSS_KISS_OFFSET_X))<=2&&abs(a.p[c.group].y-(BOSS_PRINCESS_Y+1))<=2);
 begin(&c,&a,1);c.phase=BOSS_DUEL;c.spawned=1;a.countdown=0;a.p[0].hp=a.p[1].hp=0;step(&c,&a,1);assert(c.phase==BOSS_DUEL&&a.countdown==83&&a.p[0].hp>0&&a.p[1].hp>0&&!a.winner);
 /* Sustained real physics and live guest validation with all five attacks. */
 begin(&c,&a,1);step(&c,&a,299);unsigned seen=0;
 for(unsigned tick=0;tick<12000;tick++){
  for(int h=0;h<2;h++)a.p[h].hp=campaign_health(&c,h);
  c.war.machine.hp=180;uint16_t keys[2]={(tick/80)%2?RIGHT:LEFT,(tick/90)%2?UP:DOWN};campaign_step(&c,&a,keys);
  seen|=1u<<c.war.mode;CampaignPacket packet;campaign_pack(&packet,&c,&a,tick);assert(campaign_packet_valid(&packet));
 }
 assert(seen&(1u<<BOSS_PUNCH));assert(seen&(1u<<BOSS_SPIKES));assert(seen&(1u<<BOSS_FIRE));
 /* Every source drop exists concurrently, flies from the chest and can be
    collected without queue loss; all seed-dependent landing sites are legal. */
 begin(&c,&a,1);c.phase=BOSS_PRINCESS;c.war.machine.ready=0;c.war.machine.hp=0;c.war.hit_seq[0]=c.war.hit_seq[1]=0;
 for(unsigned seed=0;seed<256;seed++){c.npc_ticks=seed;for(unsigned i=0;i<BOSS_GOLD_COUNT;i++){BossGold item={0};assert(campaign_boss_gold(&c,i,&item));assert(stage_position_open_map(BOSS_MAP,item.x,item.y));}}
 c.npc_ticks=73;c.phase=BOSS_TREASURE;c.timer=65;BossGold first={0};assert(campaign_boss_gold(&c,0,&first)&&first.age==0);assert(first.x==c.war.machine.x+c.war.machine.facing*30&&first.y==c.war.machine.y);
 c.phase=BOSS_PRINCESS;c.profiles[0].gold=c.profiles[1].gold=0;a.p[1].x=850;a.p[1].y=330;unsigned expected=0;
 for(unsigned i=0;i<BOSS_GOLD_COUNT;i++){BossGold item={0};assert(campaign_boss_gold(&c,i,&item));expected+=item.value;}
 for(unsigned i=0;i<BOSS_GOLD_COUNT;i++){BossGold item={0};if(campaign_boss_gold(&c,i,&item)){a.p[0].x=item.x;a.p[0].y=item.y;step(&c,&a,1);}}
 assert(c.profiles[0].gold==expected&&c.profiles[1].gold==0&&c.pending_count==0);assert(c.war.hit_seq[0]==65535&&c.war.hit_seq[1]==8191);
 CampaignPacket goldpacket;campaign_pack(&goldpacket,&c,&a,77);assert(campaign_packet_valid(&goldpacket));Campaign goldguest={0};Arena goldarena={0};campaign_unpack(&goldpacket,&goldguest,&goldarena);assert(goldguest.war.hit_seq[0]==65535&&goldguest.war.hit_seq[1]==8191&&goldguest.npc_ticks==73);
 /* Source rope hit accepts melee/arrow, not magic or arrows above its body. */
 begin(&c,&a,0);c.phase=BOSS_PRINCESS;c.war.machine.ready=0;c.war.hit_seq[0]=65535;c.war.hit_seq[1]=8191;
 a.shots[0]=(Projectile){.x=BOSS_PRINCESS_X,.y=BOSS_PRINCESS_Y,.z=20,.life=30,.owner=0,.kind=SHOT_FIRE};step(&c,&a,1);assert(c.phase==BOSS_PRINCESS);
 a.shots[0]=(Projectile){.x=BOSS_PRINCESS_X,.y=BOSS_PRINCESS_Y,.z=150,.life=30,.owner=0,.kind=SHOT_ARROW};step(&c,&a,1);assert(c.phase==BOSS_PRINCESS);
 a.shots[0]=(Projectile){.x=BOSS_PRINCESS_X,.y=BOSS_PRINCESS_Y,.z=20,.life=30,.owner=0,.kind=SHOT_ARROW};step(&c,&a,1);assert(c.phase==BOSS_RELEASE);
 /* Guaranteed food belongs to spawn time, survives later boss HP changes,
    appears only when that corpse vanishes, then its byte is cleared. */
 begin(&c,&a,0);c.phase=BOSS_FIGHT;c.war.machine.ready=1;c.war.summon_timer=0;a.p[0].x=450;a.p[0].y=315;step(&c,&a,1);
 assert(c.enemies[0].ready&&c.break_hp[0]>=2&&c.break_hp[0]<=6);unsigned food=c.break_hp[0];
 c.war.machine.hp=70;c.enemies[0].hp=0;c.enemies[0].pose=POSE_DEAD;c.enemies[0].z=0;c.combat.death_age[0]=42;step(&c,&a,1);assert(c.break_hp[0]==food&&!c.drops[0].kind);step(&c,&a,1);
 assert(!c.enemies[0].ready&&!c.break_hp[0]&&c.drops[0].kind==DROP_FOOD&&c.drops[0].value==(10|((food-2)<<5)));
 c.war.summon_timer=0;step(&c,&a,1);assert(c.enemies[0].ready&&!c.break_hp[0]);
 /* The same intent also drains during cinematic corpse cleanup. */
 memset(c.drops,0,sizeof(c.drops));c.pending_count=0;c.enemies[0].hp=0;c.enemies[0].pose=POSE_DEAD;c.enemies[0].z=0;c.combat.death_age[0]=43;c.break_hp[0]=6;
 campaign_cinematic_tick(&c,&a);assert(!c.enemies[0].ready&&!c.break_hp[0]&&c.drops[0].kind==DROP_FOOD&&c.drops[0].value==(10|(4<<5)));
 /* End-to-end co-op rescue: actual boss hit, death, chest, rope hit,
    release, required versus countdown, and actual opposing-player damage.
    A downed but connected partner is revived for the mandatory duel. */
 begin(&c,&a,1);step(&c,&a,299);c.war.machine.hp=1;
 a.shots[0]=(Projectile){.x=c.war.machine.x-10,.y=c.war.machine.y,.z=20,.life=30,.owner=0,.kind=SHOT_ARROW};step(&c,&a,1);assert(c.phase==BOSS_DEATH);
 a.p[1].hp=0;a.p[1].pose=POSE_DEAD;step(&c,&a,73);assert(c.phase==BOSS_TREASURE);step(&c,&a,320);assert(c.phase==BOSS_PRINCESS);
 a.shots[0]=(Projectile){.x=BOSS_PRINCESS_X,.y=BOSS_PRINCESS_Y,.z=20,.life=30,.owner=0,.kind=SHOT_ARROW};step(&c,&a,1);assert(c.phase==BOSS_RELEASE);step(&c,&a,32);assert(c.phase==BOSS_RELEASE);step(&c,&a,1);assert(c.phase==BOSS_DUEL&&a.p[1].hp==campaign_health(&c,1)&&a.countdown==83);
 step(&c,&a,82);assert(c.phase==BOSS_DUEL&&a.countdown==1);step(&c,&a,1);assert(c.phase==BOSS_DUEL&&!a.countdown);
 /* Simultaneous knockout must replay, never select the rope cutter. */
 a.p[0].hp=a.p[1].hp=0;step(&c,&a,1);assert(c.phase==BOSS_DUEL&&a.countdown==83&&a.p[0].hp>0&&a.p[1].hp>0);step(&c,&a,83);
 a.p[0].x=470;a.p[0].y=315;a.p[1].x=500;a.p[1].y=315;a.p[1].hp=1;
 a.shots[0]=(Projectile){.x=495,.y=315,.z=20,.vx=8,.life=30,.owner=0,.kind=SHOT_ARROW};step(&c,&a,1);assert(c.phase==BOSS_KISS&&c.group==0&&a.p[1].hp==0);
 step(&c,&a,50);assert(c.group==0&&a.p[1].hp==0&&a.p[1].pose==POSE_DEAD);
 /* Loss of guest readiness cannot silently bypass versus mode. Main stops
    the real wireless room on disconnect before another simulation tick. */
 begin(&c,&a,1);c.phase=BOSS_RELEASE;c.timer=32;a.p[1].ready=0;step(&c,&a,2);assert(c.phase==BOSS_RELEASE);a.p[1].ready=1;step(&c,&a,1);assert(c.phase==BOSS_DUEL);a.p[1].ready=0;step(&c,&a,100);assert(c.phase==BOSS_DUEL);
 /* A winner's old jump must settle, then grounded horizontal placement aligns the
    kiss. The shared timer cannot start kiss artwork while still travelling. */
 begin(&c,&a,1);c.phase=BOSS_DUEL;a.countdown=0;a.p[1].hp=0;a.p[1].z=80;a.p[1].vz=10;a.p[0].x=650;a.p[0].y=330;a.p[0].z=70;a.p[0].vz=8;c.war.shot_timer=999;c.war.machine.hp=0;c.war.machine.ready=0;
 step(&c,&a,1);assert(c.phase==BOSS_KISS&&c.war.shot_timer==0);int firstz=a.p[0].z;int settled=0;
 for(unsigned t=0;t<500&&c.phase!=BOSS_COMPLETE;t++){
  step(&c,&a,1);Fighter*winner=&a.p[c.group];
  if(winner->z==0)settled=1;
  if(c.timer>=65){assert(c.war.shot_timer==2&&winner->z==0);assert(abs(winner->x-(BOSS_PRINCESS_X-BOSS_KISS_OFFSET_X))<=2&&abs(winner->y-(BOSS_PRINCESS_Y+1))<=2);}
  CampaignPacket motion;campaign_pack(&motion,&c,&a,t);assert(campaign_packet_valid(&motion));Campaign guest={0};Arena other={0};campaign_unpack(&motion,&guest,&other);assert(other.p[c.group].z==winner->z);
 }
 assert(firstz>0&&settled&&c.phase==BOSS_COMPLETE&&a.p[0].z==0&&a.p[1].z==0&&a.p[1].hp==0);
 /* Full snapshot exchange through every boss state, including defeat/winner. */
 for(unsigned phase=BOSS_INTRO;phase<=BOSS_COMPLETE;phase++){begin(&c,&a,1);c.phase=phase;c.war.machine.ready=phase<=BOSS_DEATH;c.war.machine.hp=phase>=BOSS_DEATH?0:450;c.war.machine.animation=142;c.war.machine.attack_kind=BOSS_FIRE;c.group=1;c.war.flags=7;c.completed=7;CampaignPacket p;campaign_pack(&p,&c,&a,55);assert(campaign_packet_valid(&p));Campaign guest={0};Arena other={0};campaign_unpack(&p,&guest,&other);assert(other.map_id==11&&guest.phase==phase&&guest.group==1&&guest.completed==7&&guest.war.machine.hp==c.war.machine.hp);}
 puts("Boss: source cue timing, HP scaling, directional/depth dodge, finite damage, treasure, solo rescue, co-op duel, winner kiss and snapshot PASS");return 0;
}
