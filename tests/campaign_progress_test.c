#include "campaign_link.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void tick(Campaign *c,Arena *a){uint16_t k[2]={0};campaign_step(c,a,k);}
int main(void){
    assert(campaign_stat_budget(1)==0&&campaign_stat_budget(2)==2&&campaign_stat_budget(20)==38&&campaign_stat_budget(21)==39&&campaign_stat_budget(99)==96);
    Arena a;arena_init(&a);Campaign c={0};campaign_start(&c,&a,1);
    assert(!campaign_spend(&c,&a,0,0));
    c.profiles[0]=(CampaignProfile){.xp=191,.level=2,.knight=0,.gold=123};
    int hp=a.p[0].hp;assert(campaign_spend(&c,&a,0,1)&&campaign_health(&c,0)==131&&a.p[0].hp==hp);
    assert(campaign_spend(&c,&a,0,2)&&campaign_mana(&c,0)==45&&!campaign_spend(&c,&a,0,3));
    CampaignPacket packet;campaign_pack(&packet,&c,&a,1);assert(campaign_packet_valid(&packet));
    Campaign copy={0};Arena replica;campaign_unpack(&packet,&copy,&replica);assert(!memcmp(copy.stats,c.stats,8)&&copy.profiles[0].xp==191);
    for(unsigned stat=0;stat<4;stat++){
        memset(c.stats,0,sizeof(c.stats));c.profiles[0].level=99;c.profiles[0].xp=keep_xp_threshold(98)+1;c.stats[0][stat]=24;
        a.p[0].hp=campaign_health(&c,0);a.p[0].mana=campaign_mana(&c,0);
        campaign_pack(&packet,&c,&a,stat);assert(campaign_packet_valid(&packet));campaign_unpack(&packet,&copy,&replica);assert(!memcmp(copy.stats,c.stats,8));
    }
    CampaignRequestState state={0};a.running=0;
    CampaignRequest r={.magic=CAMPAIGN_REQUEST_MAGIC,.version=CAMPAIGN_VERSION,.sequence=1,.operation=CAMPAIGN_REQUEST_PROFILE};
    campaign_character_default(&r.character,2);r.character.profile.level=2;r.character.profile.xp=191;r.character.owned=weapon_set_low(3);
    assert(campaign_request_apply(&state,&r,&c,&a));a.p[1].knight_id=2;campaign_start(&c,&a,1);
    assert(c.profiles[1].xp==191&&c.menu_ack[1]==1);
    r.sequence++;r.operation=CAMPAIGN_REQUEST_STAT;r.slot=0;assert(campaign_request_apply(&state,&r,&c,&a));assert(!campaign_request_apply(&state,&r,&c,&a)&&c.stats[1][0]==1);
    r.sequence++;r.operation=CAMPAIGN_REQUEST_EQUIP;r.slot=1;assert(!campaign_request_apply(&state,&r,&c,&a)&&c.weapons[1]==0);
    campaign_pack(&packet,&c,&a,99);assert(campaign_packet_valid(&packet));campaign_unpack(&packet,&copy,&replica);assert(copy.menu_ack[1]==3&&copy.stats[1][0]==1&&copy.weapons[1]==0);
    r.sequence++;r.operation=CAMPAIGN_REQUEST_EQUIP;r.slot=2;assert(!campaign_request_apply(&state,&r,&c,&a)&&c.menu_ack[1]==4);
    c.potions[1]=2;r.sequence++;r.operation=CAMPAIGN_REQUEST_ITEM;r.slot=1;
    assert(campaign_request_apply(&state,&r,&c,&a)&&campaign_potion_equipped(&c,1)&&a.p[1].equip_timer==30);
    assert(!campaign_request_apply(&state,&r,&c,&a)&&campaign_potion_equipped(&c,1));
    campaign_pack(&packet,&c,&a,100);assert(campaign_packet_valid(&packet));campaign_unpack(&packet,&copy,&replica);
    assert(campaign_potion_equipped(&copy,1)&&replica.p[1].equip_timer==30);
    r.sequence++;r.slot=2;assert(!campaign_request_apply(&state,&r,&c,&a)&&!(c.tools[1]&1));
    /* Fresh/alternating saves and interrupted newest bank recovery. */
    a.p[0].pose=POSE_WALK;a.p[0].animation=65;a.p[0].previous=RIGHT|UP;a.p[0].hp=100;
    c.enemies[0]=(Fighter){.ready=1,.pose=POSE_DEAD,.animation=23,.x=336,.y=285};
    campaign_pack(&packet,&c,&a,100);assert(campaign_packet_valid(&packet));campaign_unpack(&packet,&copy,&replica);
    assert(arena_fighter_running(&replica.p[0])&&(replica.p[0].previous&(RIGHT|UP))==(RIGHT|UP));
    assert(copy.enemies[0].animation==23&&!campaign_npc_visible(&copy.enemies[0]));
    remove("build/save-fixture.0.sav");remove("build/save-fixture.1.sav");
    assert(!campaign_save_open("build/save-fixture"));campaign_save_capture(&c,1);assert(campaign_save_dirty()&&campaign_save_flush());
    c.profiles[1].gold=555;campaign_save_capture(&c,1);assert(campaign_save_flush());
    assert(campaign_save_open("build/save-fixture")&&campaign_save_character(2)->profile.gold==555);
    FILE *f=fopen("build/save-fixture.1.sav","wb");assert(f);fputs("torn write",f);fclose(f);
    assert(campaign_save_open("build/save-fixture")&&campaign_save_character(2)->profile.gold==0);
    assert(!campaign_save_open("build/nonexistent-directory/save"));campaign_save_capture(&c,1);assert(!campaign_save_flush()&&campaign_save_dirty());
    remove("build/save-fixture.0.sav");remove("build/save-fixture.1.sav");
    /* High-defense actors retain health above the former409 arena cap on hit. */
    c.phase=KEEP_BARRACKS_TRAVEL;a.p[0].hp=1000;c.stats[0][1]=24;c.profiles[0].level=99;c.profiles[0].xp=keep_xp_threshold(98)+1;tick(&c,&a);assert(a.p[0].hp==1000);
    puts("PASS: source stat budgets, two-player attributes/ack packets, idempotent inventory requests, per-knight saves and torn-write recovery");
}
