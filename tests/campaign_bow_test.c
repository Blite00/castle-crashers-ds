#include "campaign.h"
#include "campaign_forest.h"
#include "campaign_save.h"
#include "item_selector.h"
#include "stage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Source f_EnemyDie item 11: thieves drop the bow while a hero lacks it; f_UnlockItem(bow) equips it. */
static void begin(Campaign *c,Arena *a,int coop){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=coop;
    campaign_start(c,a,coop);c->completed=7;campaign_forest_start(c,a);
}
static void kill(Campaign *c,Arena *a,unsigned slot,unsigned kind,int x,int y){
    c->enemies[slot]=(Fighter){.x=x,.y=y,.ready=1,.hp=0,.pose=POSE_DEAD,.knight_id=kind};
    for(int t=0;t<50;t++)campaign_cinematic_tick(c,a);
}
static unsigned drops_of(const Campaign *c,unsigned kind){unsigned n=0;for(int d=0;d<CAMPAIGN_DROPS;d++)n+=c->drops[d].kind==kind;return n;}
static void packet(const Campaign *c,const Arena *a){
    CampaignPacket p;Campaign guest={0};Arena remote;campaign_pack(&p,c,a,5);assert(campaign_packet_valid(&p));
    campaign_unpack(&p,&guest,&remote);
    for(int h=0;h<2;h++){assert(guest.tools[h]==c->tools[h]);assert(guest.bow_on[h]==((c->tools[h]&2)&&c->bow_on[h]?1:0));assert(campaign_equipped_item(&guest,h)==campaign_equipped_item(c,h));}
}
int main(void){
    Campaign c;Arena a;begin(&c,&a,0);a.p[0].x=400;a.p[0].y=250;
    /* A barbarian never drops it. */
    kill(&c,&a,0,0,420,250);assert(drops_of(&c,DROP_BOW)==0);
    /* The first thief to die on screen drops one bow; a second thief leaves it alone while it lies there. */
    kill(&c,&a,1,1,430,255);assert(drops_of(&c,DROP_BOW)==1);
    kill(&c,&a,2,1,440,260);assert(drops_of(&c,DROP_BOW)==1);
    packet(&c,&a);
    /* Walking over it grants and equips the bow; a pickup is silent loot art, not food or gold. */
    int slot=-1;for(int d=0;d<CAMPAIGN_DROPS;d++)if(c.drops[d].kind==DROP_BOW)slot=d;
    a.p[0].x=c.drops[slot].x;a.p[0].y=c.drops[slot].y;c.drop_age[slot]=30;
    campaign_cinematic_tick(&c,&a);
    assert((c.tools[0]&2)&&c.bow_on[0]&&campaign_equipped_item(&c,0)==ITEM_BOW&&drops_of(&c,DROP_BOW)==0&&a.p[0].bow_equipped);
    assert((campaign_item_mask(&c,0)&(1u<<ITEM_BOW))&&!(campaign_item_mask(&c,1)&(1u<<ITEM_BOW)));
    packet(&c,&a);
    /* With the bow owned no further thief drops one. */
    kill(&c,&a,3,1,450,255);assert(drops_of(&c,DROP_BOW)==0);
    /* Off screen thieves do not drop it for someone who still lacks it. */
    begin(&c,&a,0);a.p[0].x=200;kill(&c,&a,0,1,900,250);assert(drops_of(&c,DROP_BOW)==0);
    /* Equipped, the bow fires an arrow for the hero in a normal campaign step. */
    begin(&c,&a,0);c.tools[0]|=2;campaign_select_item(&c,0,3);assert(c.bow_on[0]);
    a.p[0].x=400;a.p[0].y=250;uint16_t none[2]={0,0},fire[2]={BOW|BOW_EDGE,0};
    campaign_step(&c,&a,none);assert(a.p[0].bow_equipped);
    unsigned shots=0;for(int t=0;t<30;t++){campaign_step(&c,&a,t==0?fire:none);for(int i=0;i<2;i++)shots+=a.shots[i].life&&a.shots[i].kind==SHOT_ARROW&&a.shots[i].owner==0;}
    assert(shots>0);
    /* EQUIP walks through owned items: potion, bow, then empty hands. */
    begin(&c,&a,0);c.tools[0]|=2;c.potions[0]=2;uint16_t equip[2]={EQUIP|EQUIP_EDGE,0};
    campaign_step(&c,&a,equip);assert(campaign_equipped_item(&c,0)==ITEM_POTION);
    campaign_step(&c,&a,none);campaign_step(&c,&a,equip);assert(campaign_equipped_item(&c,0)==ITEM_BOW&&a.p[0].bow_equipped);
    campaign_step(&c,&a,none);campaign_step(&c,&a,equip);assert(campaign_equipped_item(&c,0)==ITEM_NONE&&!a.p[0].bow_equipped);
    /* Ownership persists in the save format and a locked bow cannot be selected. */
    begin(&c,&a,0);assert(!campaign_select_item(&c,0,3));c.tools[0]=2;
    CampaignCharacter save;campaign_character_get(&save,&c,0);assert((save.tools&2)&&campaign_character_valid(&save));
    Campaign restored={0};campaign_character_put(&save,&restored,0);assert(restored.tools[0]&2);
    puts("Bow: thief drop rule, pickup and equip, arrows, item cycling, wire and save pass");
    return 0;
}
