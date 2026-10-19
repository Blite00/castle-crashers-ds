#include <stdio.h>
#ifdef CHAPTER_ASSETS_TEST
#include "chapter_assets.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "arena.h"
#include "render.h"
#include "stage.h"
#include "campaign.h"
int main(int argc,char **argv){
    if(argc<2||argc>3)return 2;
    Arena a;arena_init(&a);a.p[0].x=186;a.p[1].x=237;a.p[0].y=a.p[1].y=285;
    a.p[0].pose=POSE_ATTACK;a.p[0].cooldown=8;a.p[0].animation=5;a.p[1].pose=POSE_BLOCK;
    int scenario=argc==3?atoi(argv[2]):0;
    if(scenario==1){a.p[0].x=ARENA_MIN_X;a.p[1].x=ARENA_MAX_X;}
    if(scenario==2){a.p[0].x=ARENA_MIN_X;a.p[1].x=ARENA_MIN_X+51;}
    if(scenario==3){a.p[0].x=ARENA_MAX_X-51;a.p[1].x=ARENA_MAX_X;}
    if(scenario==4){a.p[0].y=ARENA_MIN_Y;a.p[0].z=80;a.p[1].y=ARENA_MAX_Y;}
    if(scenario==5){a.p[0].x=200;a.p[0].y=185;a.p[0].pose=POSE_IDLE;a.p[0].cooldown=0;a.p[1].x=340;a.p[1].y=285;}
    if(scenario==6){a.p[0].x=200;a.p[0].y=285;a.p[0].pose=POSE_IDLE;a.p[0].cooldown=0;a.p[1].x=340;a.p[1].y=285;}
    if(scenario==7){a.p[0].freeze=20;a.p[1].burn=30;a.shots[0]=(Projectile){.x=200,.y=185,.z=14,.vx=5,.life=10};a.shots[1]=(Projectile){.x=310,.y=285,.z=14,.vx=-5,.life=10,.owner=1};}
    if(scenario==8){a.p[0].z=55;a.p[0].pose=POSE_JUMP;a.p[0].cooldown=0;a.p[0].animation=20;a.p[1].pose=POSE_SPLASH;a.p[1].attack_kind=HIT_SPLASH;a.p[1].cooldown=5;}
    Campaign campaign={0};
    Fighter enemy={.x=340,.y=240,.facing=-1,.hp=120,.ready=1};
    uint16_t p[256*192+2],output[256*192];
    FILE *f=fopen(argv[1],"wb");if(!f)return 1;
    for(int step=0;step<((scenario>=9&&scenario<=16)||scenario>=24?120:1);step++){
    if(scenario==9){a.p[0].x=40+(step*3)%340;a.p[1].x=380-(step*2)%340; a.p[0].y=180+step%120;a.p[1].y=300-step%110; a.p[0].z=step%56;a.p[0].pose=POSE_THROW;a.p[0].animation=step%17;a.countdown=step<30?30-step:0;a.running=1;a.tick=step+90;a.winner=step>110?1:0;}
    if(scenario==10){a.p[0].pose=a.p[1].pose=POSE_BLOCK;a.p[0].cooldown=0;a.p[0].animation=a.p[1].animation=step;}
    if(scenario==11){a.p[0].pose=POSE_ROLL;a.p[1].pose=POSE_AIRSPIN;a.p[1].z=35;a.p[0].animation=a.p[1].animation=step%12;}
    if(scenario==11&&step>=60){
        a.p[0].pose=a.p[1].pose=POSE_BOW;a.p[0].animation=a.p[1].animation=(step-60)%20;
        a.p[0].x=90;a.p[1].x=330;a.p[0].y=a.p[1].y=285;a.p[1].z=0;
        a.shots[0]=(Projectile){.x=50+(step-60)*5,.y=185,.z=20,.vx=10,.life=20,.kind=SHOT_ARROW,.vz=5-(step%30)};
        a.shots[1]=(Projectile){.x=370-(step-60)*5,.y=285,.z=20,.vx=-10,.life=20,.owner=1,.kind=SHOT_ARROW,.vz=5-(step%30)};
    }
    if(scenario==11&&step<40){a.p[0].pose=POSE_AIRATTACK2;a.p[1].pose=POSE_AIRHEAVY;a.p[0].z=30;a.p[0].animation=step%12;a.p[1].animation=step%14;}
    if(scenario==11&&step>=90){
        const int kinds[]={SHOT_ICE,SHOT_FIRE,SHOT_LIGHTNING,SHOT_POISON};
        unsigned id=((step-90)/7)%4;
        a.p[0].pose=a.p[1].pose=POSE_CASTDOWN;a.p[0].animation=a.p[1].animation=(step-90)%13;
        a.p[0].knight_id=id;a.p[1].knight_id=3-id;a.p[0].z=a.p[1].z=45;
        a.shots[0]=(Projectile){.x=110+(step-90)*5,.y=185,.z=50-(step-90)%12*4,.vx=13,.vz=-9,.life=20,.kind=kinds[id]};
        a.shots[1]=(Projectile){.x=310-(step-90)*5,.y=285,.z=50-(step-90)%12*4,.vx=-13,.vz=-9,.life=20,.owner=1,.kind=kinds[3-id]};
    }
    if(scenario==12){a.p[0].pose=a.p[1].pose=POSE_CLASH;a.p[0].animation=a.p[1].animation=step;}
    if(scenario==13){a.p[0].burn=step<ARENA_BURN_TICKS?ARENA_BURN_TICKS-step:0;a.p[1].freeze=step<30?30-step:0;a.p[0].previous=step<60?MAGIC:0;a.p[1].previous=step<90?MAGIC:0;a.tick=step;}
    if(scenario==13){
        a.p[0].equip_timer=step<30?30-step:step>=60&&step<90?90-step:0;
        a.p[0].bow_equipped=step<60;a.p[0].z=step%40;
        a.p[1].equip_timer=step>=30&&step<60?60-step:0;a.p[1].bow_equipped=1;
    }
    if(scenario>=14){a.map_id=scenario==16?(step/40)%3:scenario-13;a.p[0].animation=step%12;a.p[0].pose=POSE_HEAVY2;a.p[1].previous=MAGIC;a.tick=step;}
    if(scenario==16){a.p[0].knight_id=(step/30)%4;a.p[1].knight_id=3-a.p[0].knight_id;
        a.p[1].ready=step<90;a.p[0].previous=a.p[1].previous=MAGIC;}
    if(scenario==24||scenario==25){
        a.map_id=3;a.p[0].x=110;a.p[1].x=145;a.p[0].y=235;a.p[1].y=265;
        a.p[1].ready=scenario==25;a.p[0].previous=a.p[1].previous=0;
        enemy.x=240-step;enemy.y=205+step%85;enemy.z=step%50;
        enemy.pose=(step/5)%POSE_COUNT;enemy.animation=step%12;
        arena_render_enemy(&enemy);
    }
    if(scenario>=26&&scenario<=31){
        a.map_id=3;a.running=1;a.tick=200+step;
        campaign.phase=step<60?KEEP_TRAVEL:KEEP_FIGHT;
        for(int i=0;i<2;i++){campaign.pickup_kind[i]=i?DROP_COIN:DROP_FOOD;campaign.pickup_value[i]=i?1:10;campaign.pickup_timer[i]=30-step%31;}
        a.p[0].x=110+(scenario-26)*380+step;a.p[1].x=a.p[0].x+50;a.p[0].z=a.p[1].z=0;
        for(int i=0;i<2;i++)for(int y=0;y<stage_world_height(a.map_id);y++)if(stage_position_open_map(a.map_id,a.p[i].x,y)){a.p[i].y=y+5;break;}
        for(int e=0;e<CAMPAIGN_ENEMIES;e++){
            Fighter *f=&campaign.enemies[e];*f=enemy;f->x=a.p[0].x+70+e*25;f->pose=(step/8+e)%POSE_COUNT;f->animation=step%8;f->z=step%45;
            for(int y=0;y<stage_world_height(a.map_id);y++)if(stage_position_open_map(a.map_id,f->x,y)){f->y=y+15;break;}
        }
        campaign.enemies[0].freeze=step<30?30-step:0;campaign.enemies[1].burn=step<59?59-step:0;
        campaign.npc_ticks=step;
        campaign.drops[0]=(CampaignDrop){a.p[0].x+40,a.p[0].y,DROP_COIN,1};
        campaign.drops[1]=(CampaignDrop){a.p[0].x+60,a.p[0].y,DROP_FOOD,10};arena_render_campaign(&campaign);
    }
    if(scenario==34||scenario==35){
        a.map_id=4;a.running=1;a.tick=300+step;campaign.phase=scenario==34?KEEP_STORE_FIGHT:KEEP_STORE;
        a.p[0].x=100+step*15;a.p[1].x=a.p[0].x+50;a.p[0].y=a.p[1].y=250;a.p[0].z=a.p[1].z=0;
        a.p[0].previous=a.p[1].previous=0;
        for(int e=0;e<CAMPAIGN_ENEMIES;e++){
            Fighter *f=&campaign.enemies[e];*f=enemy;f->ready=scenario==34;f->x=a.p[0].x+25+e*20;f->y=220+e*10;f->z=step%45;
            f->pose=(step/8+e)%POSE_COUNT;f->animation=step%8;
        }
        campaign.ally=enemy;campaign.ally.x=a.p[0].x-20;campaign.ally.y=260;campaign.ally.pose=POSE_ATTACK;campaign.ally.animation=step%8;
        campaign.drops[0]=(CampaignDrop){a.p[0].x+40,250,DROP_GEM,10};
        campaign.pickup_kind[0]=DROP_GEM;campaign.pickup_value[0]=10;campaign.pickup_timer[0]=30-step%31;
        arena_render_campaign(&campaign);
    }
    if(scenario==36){
        a.map_id=3;a.tick=100+step;a.p[0].x=890;a.p[1].x=930;a.p[0].y=320;a.p[1].y=350;
        a.p[0].z=step%55;a.p[1].z=0;campaign.phase=KEEP_FIGHT;campaign.broken=step>=50;campaign.break_tick=150;
        arena_render_campaign(&campaign);
    }
if(scenario>=37&&scenario<=39){
        a.map_id=5;a.tick=100+step;a.p[0].x=scenario==39?750:310+step*2;a.p[1].x=a.p[0].x+35;
        a.p[0].y=240+step%110;a.p[1].y=300;a.p[0].z=step%40;a.p[1].z=0;
        campaign.phase=scenario==37?KEEP_BARRACKS_INTRO:KEEP_BARRACKS_TRAVEL;campaign.timer=step*4;
        campaign.broken=scenario==38&&step>=50?62:0;campaign.break_tick=150;
        arena_render_campaign(&campaign);
    }
    if(scenario>=40&&scenario<=43){
        a.map_id=3;a.running=1;a.tick=step;campaign.phase=KEEP_FIGHT;
        a.p[0].knight_id=scenario-40;a.p[1].knight_id=43-scenario;
        a.p[0].x=320;a.p[1].x=390;a.p[0].y=260;a.p[1].y=275;
        a.p[0].pose=a.p[1].pose=(step/4)%POSE_COUNT;
        a.p[0].animation=a.p[1].animation=step/4;
        a.p[0].facing=step%2?-1:1;a.p[1].facing=-a.p[0].facing;
        campaign.weapons[0]=step%4;campaign.weapons[1]=3-step%4;
        arena_render_campaign(&campaign);
    }
    if(scenario==44){
        a.map_id=5;a.tick=425+step;campaign.phase=step<61?KEEP_BARRACKS_INTRO:KEEP_BARRACKS_TRAVEL;
        campaign.timer=425+step;campaign.story_flags=64;
        a.p[0].x=336;a.p[1].x=318;a.p[0].y=285;a.p[1].y=261;
        a.p[0].pose=a.p[1].pose=POSE_IDLE;a.p[0].z=a.p[1].z=0;
        arena_render_campaign(&campaign);
    }
    if(scenario>=52&&scenario<=55){
        a.map_id=3;a.running=1;a.tick=step;campaign.phase=KEEP_FIGHT;
        a.p[0].knight_id=scenario-52;a.p[1].knight_id=55-scenario;
        a.p[0].x=320;a.p[1].x=390;a.p[0].y=260;a.p[1].y=275;
        a.p[0].pose=POSE_HEAVY3;a.p[1].pose=POSE_STOMP;
        a.p[0].animation=step%12;a.p[1].animation=step%24;
        a.p[0].facing=(step/24)%2?-1:1;a.p[1].facing=-a.p[0].facing;
        campaign.weapons[0]=(step/24)%4;campaign.weapons[1]=3-(step/24)%4;
        arena_render_campaign(&campaign);
    }
    if(scenario==51){
        a.map_id=3;a.running=1;a.tick=600+step;campaign.phase=KEEP_TRAVEL;campaign.broken=1;
        a.p[0].x=730;a.p[0].y=350;a.p[0].z=0;a.p[0].pose=POSE_IDLE;a.p[1].ready=0;
        campaign.drops[0]=(CampaignDrop){745,350,DROP_COIN,1};campaign.drop_age[0]=step*3;
        arena_render_campaign(&campaign);
    }
    if(scenario==49||scenario==50){
        a.map_id=3;a.running=1;a.tick=600+step;campaign.phase=KEEP_TRAVEL;campaign.group=4;campaign.broken=1;
        a.p[0].x=2180+step*2;a.p[0].y=235;a.p[0].z=step%35;
        a.p[0].pose=POSE_WALK;a.p[0].animation=step;
        a.p[1]=a.p[0];a.p[1].x-=55;a.p[1].y+=35;a.p[1].ready=scenario==50;
        arena_render_campaign(&campaign);
    }
    if(scenario==48){
        a.map_id=5;a.running=1;a.tick=600+step;campaign.phase=KEEP_BARRACKS_TRAVEL;
        a.p[0].x=1020;a.p[0].y=232;a.p[0].z=0;a.p[0].pose=POSE_IDLE;
        a.p[1].x=1005;a.p[1].y=234;a.p[1].z=0;a.p[1].pose=POSE_WALK;a.p[1].animation=step%16;
        arena_render_campaign(&campaign);
    }
    if(scenario==46||scenario==47){
        a.map_id=5;a.running=1;a.tick=600+step;campaign.phase=KEEP_BARRACKS_TRAVEL;
        a.p[0].x=950+step;a.p[0].y=258-step*2/3+(scenario==47?40:0);
        a.p[0].z=0;a.p[0].pose=POSE_WALK;a.p[0].animation=step;
        a.p[1].x=1060;a.p[1].y=220;a.p[1].z=0;a.p[1].pose=POSE_IDLE;
        arena_render_campaign(&campaign);
    }
    if(scenario==45){
        a.map_id=5;a.running=1;a.tick=600+step;campaign.phase=KEEP_BARRACKS_TRAVEL;
        a.p[0].x=1050+step;a.p[0].y=165;a.p[0].z=0;a.p[0].pose=POSE_WALK;a.p[0].animation=step;
        a.p[1].x=1060;a.p[1].y=220;a.p[1].z=0;a.p[1].pose=POSE_IDLE;
        arena_render_campaign(&campaign);
    }
    if(scenario>=56){
        static const unsigned maps[]={6,6,7,8,9,10,8,5};
        static const int xs[]={280,2900,600,360,1500,620,360,155};
        unsigned test=(unsigned)scenario-56;if(test>7)test=0;
        a.map_id=maps[test];a.running=1;a.tick=step;campaign.phase=test<3?WAR_TRAVEL:test==7?KEEP_BARRACKS_TRAVEL:HUB_EXPLORE;
        for(int h=0;h<2;h++){a.p[h].ready=1;a.p[h].x=xs[test]+h*45;a.p[h].y=280+h*15;a.p[h].z=0;a.p[h].pose=POSE_WALK;a.p[h].animation=step;}
        if(test==1){campaign.phase=WAR_MACHINE_FIGHT;campaign.war.machine=(Fighter){.x=3000,.y=290,.hp=350,.ready=1,.facing=-1,.pose=POSE_WALK,.animation=step};}
        if(test==7){a.p[0].pose=POSE_DIG;a.p[0].animation=step%76;campaign.pets[0].equipped=PET_MR_BUDDY;campaign.pet_followers[0]=(PetFollower){.x=130,.y=280,.facing=1};}
        campaign.owned_weapons[0]=weapon_set_low(15);arena_render_campaign(&campaign);
    }
    if(scenario>=64&&scenario<=74){
        unsigned n=scenario-64;a.map_id=BOSS_MAP;a.tick=step+200;a.round=BOSS_MAP;
        campaign.phase=n==0?BOSS_INTRO:n<=2?BOSS_FIGHT:n==3?BOSS_DEATH:n==4?BOSS_TREASURE:n==5?BOSS_RELEASE:n==6?BOSS_DUEL:BOSS_KISS;
        campaign.timer=step*(n==0||n==4?3:2);campaign.npc_ticks=72;campaign.group=0;
        campaign.war.machine=(Fighter){.x=550,.y=315,.ready=n<4,.hp=n<3?350:0,.facing=-1,.pose=POSE_IDLE,.attack_kind=n==2?(step/30)%4+2:n==3?BOSS_DIE:BOSS_WALK,.animation=step};
        if(n==0)campaign.war.machine.ready=campaign.timer>=179;
        if(n>=4)campaign.war.flags=BOSS_CHEST|BOSS_CHEST_OPEN;
        for(unsigned h=0;h<2;h++)a.p[h]=(Fighter){.x=n>=5?BOSS_PRINCESS_X-BOSS_KISS_OFFSET_X+(int)h*70:370+(int)h*45,.y=n>=5?266:315,.ready=h==0||n!=8,.hp=100,.mana=40,.facing=1,.pose=POSE_WALK,.animation=step,.knight_id=(n+h)%4};
        if(n>=7)for(unsigned h=0;h<2;h++){a.p[h].knight_id=(n-7+h)%4;if(!h)a.p[h].z=0;if(h){a.p[h].hp=0;a.p[h].pose=POSE_DEAD;a.p[h].animation=0;}}
        arena_render_campaign(&campaign);
    }
    if(scenario>=75&&scenario<=78){
        a.map_id=3;a.tick=step;campaign.phase=KEEP_FIGHT;
        for(unsigned h=0;h<2;h++){
            a.p[h]=(Fighter){.ready=1,.hp=100,.mana=40,.x=320+(int)h*70,.y=265+(int)h*20,.pose=h?POSE_DASHSLAM:POSE_DASHPUNCH,.animation=step%14,.facing=(step/24)%2?-1:1,.knight_id=(scenario-75+h)%4,.z=h?10:0};
            campaign.weapons[h]=(step/24+h)%4;
        }
        arena_render_campaign(&campaign);
    }
    if(scenario==79){
        a.map_id=3;a.tick=step;campaign.phase=KEEP_FIGHT;campaign.potions[0]=3;campaign.story_flags=2;campaign.tools[1]=1;campaign.shovel_equipped[1]=1;
        for(unsigned h=0;h<2;h++)a.p[h]=(Fighter){.ready=1,.hp=100,.mana=40,.x=230+(int)h*70+step/3,.y=265+(int)h*20,.pose=POSE_WALK,.animation=step,.knight_id=h,.equip_timer=30-step%30,.z=step%35};
        arena_render_campaign(&campaign);
    }
    if(scenario>=80&&scenario<=84){
        a.map_id=scenario==83?3:scenario==84?10:6;a.round=a.map_id;a.running=1;a.tick=step;
        a.p[1].ready=0;a.p[0]=(Fighter){.ready=1,.hp=100,.mana=40,.x=3400,.y=285,.facing=1,.pose=POSE_IDLE};
        campaign.phase=WAR_TRAVEL;campaign.timer=step;
        if(scenario==80){
            campaign.phase=WAR_FIGHT;campaign.group=7;
            a.p[0].x=3550;a.p[1]=(Fighter){.ready=1,.hp=100,.x=3830,.y=305,.pose=POSE_IDLE,.knight_id=1};
            for(int i=0;i<4;i++){const WarSpawn *s=&war_spawns[33+i];campaign.enemies[i]=(Fighter){.ready=1,.hp=50,.x=s->x,.y=s->y,.facing=-1,.knight_id=2};}
        }else if(scenario==81){campaign.phase=WAR_HORSE_CINEMA;a.p[0].x=1300+step*3;a.p[0].pose=POSE_WALK;a.p[0].animation=step;}
        else if(scenario==82){
            campaign.phase=WAR_MACHINE_FIGHT;a.p[0].x=2850;
            campaign.war.machine=(Fighter){.ready=1,.hp=350,.x=2940+step/2,.y=290,.facing=-1,.pose=step<60?POSE_WALK:POSE_CAST,.animation=step<60?step:step-60};
        }else {campaign.phase=scenario==83?KEEP_FIGHT:HUB_EXPLORE;a.p[0].x=520-step*3;if(scenario==84&&a.p[0].x<168)a.p[0].x=168;a.p[0].pose=POSE_WALK;a.p[0].animation=step;}
        arena_render_campaign(&campaign);
    }
    if(scenario==85||scenario==86){
        a.map_id=BOSS_MAP;a.round=BOSS_MAP;a.running=1;a.tick=step;campaign.phase=BOSS_FIGHT;
        a.p[0]=(Fighter){.ready=1,.hp=100,.mana=40,.x=340,.y=285,.facing=1,.knight_id=(step/30)%4};a.p[1].ready=0;
        memset(campaign.enemies,0,sizeof campaign.enemies);
        campaign.war.machine=(Fighter){.ready=1,.hp=350,.x=550,.y=300,.facing=-1,.pose=POSE_IDLE,.attack_kind=BOSS_IDLE,.flash=scenario==86?(step%10<5?5:0):0};
        if(scenario==85){unsigned t=step%30;campaign.break_hp[14]=t<15?8:t<26?8-(t-15)*7/10:0;campaign.break_hit_seq[15][0]=t<15?t*2:t<26?29:30+t-26;}
        arena_render_campaign(&campaign);
    }
    if(scenario==87){
        a.map_id=BOSS_MAP;a.round=BOSS_MAP;a.running=1;a.tick=step*3;campaign.phase=BOSS_INTRO;campaign.timer=step*3;
        a.p[0]=(Fighter){.ready=1,.hp=100,.mana=40,.x=214,.y=316,.facing=1};a.p[1].ready=0;
        memset(campaign.enemies,0,sizeof campaign.enemies);
        campaign.war.machine=(Fighter){.ready=campaign.timer>=179,.hp=350,.x=campaign.timer<279?808-(int)(campaign.timer>179?campaign.timer-179:0)*2:608,.y=315,.facing=-1,.pose=POSE_WALK,.attack_kind=BOSS_WALK,.animation=step};
        arena_render_campaign(&campaign);
    }
    if(scenario==32||scenario==33)a.map_id=3;
#ifdef CHAPTER_ASSETS_TEST
    if(scenario>=17&&scenario<=23)a.map_id=scenario<20?scenario-17:0;
    assert(chapter_assets_load(a.map_id==BOSS_MAP?CHAPTER_BOSS:a.map_id>=8?CHAPTER_HUBS:a.map_id==6||a.map_id==7?CHAPTER_WAR:a.map_id>=3?CHAPTER_KEEP:CHAPTER_ARENAS));
#endif
#ifdef RENDER_TEST_OVERLAY
    /* Match main.c: register/switch terrain before rendering into the overlay. */
    static uint8_t scene[512*512];static uint16_t palette[256];static int uploaded=-1;
    if(step==0)memset(scene+320*512,0x5a,192*512);
    if(uploaded!=a.map_id){arena_upload_background(scene,palette,a.map_id);uploaded=a.map_id;}
#endif
    p[256*192]=0x1234;p[256*192+1]=0x5678;
    unsigned before_sprite_decodes=arena_render_sprite_decodes();
    if(scenario==62){campaign.phase=CAMPAIGN_WORLD_MAP;campaign.group=3;campaign.spawned=1;campaign.timer=step<91?step:0;if(step>=91)campaign.spawned=3;campaign.completed=1;a.p[0].knight_id=1;a.p[1].knight_id=3;arena_render_world_map(p,&campaign,&a);}
    else if(scenario>=32&&scenario<=33){a.map_id=3;arena_render_movie(p,a.map_id,scenario-32,step*2);}
    else if(scenario>=26){arena_render(&a,p);}
    else if(scenario>=24){arena_render(&a,p);const ArenaCamera *v=arena_render_camera();assert(enemy.x>=v->x&&enemy.x<v->x+v->width);}
    else if(scenario>=20)arena_render_knight_selection(p,scenario-20);else if(scenario>=17)arena_render_selection(p,scenario-17);else arena_render(&a,p);
    if(p[256*192]!=0x1234||p[256*192+1]!=0x5678)return 3;
    if(scenario==80&&step>0&&step%15)assert(arena_render_sprite_decodes()==before_sprite_decodes);
    if(scenario==83)assert(arena_render_camera()->x>=0);
    if(scenario==84)assert(arena_render_camera()->x>=136);
for(int i=0;i<256*192;i++)output[i]=p[i];
#ifdef RENDER_TEST_OVERLAY
    arena_present_background();
    for(int i=320*512;i<512*512;i++)assert(scene[i]==0x5a);
    const ArenaCamera *camera=arena_render_background_camera();
    for(int y=0;y<192;y++)for(int x=0;x<256;x++)if(!(p[y*256+x]&0x8000))
        output[y*256+x]=palette[scene[(camera->y+y*camera->height/192)*512+((camera->x+x*camera->width/256)&511)]];
#endif
    fprintf(f,"P6\n256 192\n255\n");
    for(int i=0;i<256*192;i++){fputc((output[i]&31)*255/31,f);fputc(((output[i]>>5)&31)*255/31,f);fputc(((output[i]>>10)&31)*255/31,f);}
    }fclose(f);
    return 0;
}
