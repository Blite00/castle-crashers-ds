#include "campaign.h"
#include "campaign_forest.h"
#include "pets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* The rotating extended state: weapon ids reach a guest within four snapshots, pets and level clears within 32,
 * unregistered ids are refused and a guest keeps what it has learned between snapshots. */
static void host(Campaign *c,Arena *a){
    memset(c,0,sizeof *c);arena_init(a);a->p[0].knight_id=1;a->p[1].knight_id=2;a->p[1].ready=1;
    campaign_start(c,a,1);c->completed=7;campaign_forest_start(c,a);
}
int main(void){
    Campaign c,guest;Arena a,remote;host(&c,&a);memset(&guest,0,sizeof guest);
    c.weapons[0]=WEAPON_LEAF;c.weapons[1]=WEAPON_BARBARIAN;weapon_set_add(&c.owned_weapons[0],WEAPON_LEAF);weapon_set_add(&c.owned_weapons[1],WEAPON_BARBARIAN);
    c.pets[0]=(PetProfile){1u<<PET_SNOOT,PET_SNOOT};c.pets[1]=(PetProfile){1u<<PET_OWLET,PET_OWLET};
    c.completed=(1ull<<0)|(1ull<<5)|(1ull<<40)|(1ull<<63);
    CampaignPacket p;
    for(uint32_t seq=0;seq<4;seq++){campaign_pack(&p,&c,&a,seq);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&remote);}
    assert(guest.weapons[0]==WEAPON_LEAF&&guest.weapons[1]==WEAPON_BARBARIAN); /* Four snapshots. */
    for(uint32_t seq=4;seq<32;seq++){campaign_pack(&p,&c,&a,seq);assert(campaign_packet_valid(&p));campaign_unpack(&p,&guest,&remote);}
    assert(guest.pets[0].equipped==PET_SNOOT&&guest.pets[1].equipped==PET_OWLET);
    assert(guest.completed==c.completed); /* All 64 level ids. */
    /* A new equip travels while everything else is retained. */
    c.weapons[0]=WEAPON_THIEF;uint32_t seq=32;
    for(;seq<36;seq++){campaign_pack(&p,&c,&a,seq);campaign_unpack(&p,&guest,&remote);}
    assert(guest.weapons[0]==WEAPON_THIEF&&guest.weapons[1]==WEAPON_BARBARIAN&&guest.completed==c.completed&&guest.pets[0].equipped==PET_SNOOT);
    /* Clearing a bit on the host clears it on the guest after a cycle. */
    c.completed&=~(1ull<<40);campaign_sync_cycle(&c,&a,&guest,&remote,64);assert(guest.completed==c.completed);
    /* Unregistered ids never apply and the packet is refused. */
    campaign_pack(&p,&c,&a,0);CampaignPacket bad=p;bad.actors[0].hp=(int16_t)((bad.actors[0].hp&2047)|(WEAPON_COUNT<<11));bad.actors[1].hp=(int16_t)(bad.actors[1].hp&2047);
    assert(!campaign_packet_valid(&bad));
    campaign_pack(&p,&c,&a,19);bad=p;bad.actors[0].hp=(int16_t)((bad.actors[0].hp&2047)|(15<<11));bad.actors[1].hp=(int16_t)((bad.actors[1].hp&2047)|(1<<11));assert(!campaign_packet_valid(&bad)); /* pet id 31 is not registered */
    /* Every sequence number is a valid carrier for the real state. */
    for(uint32_t s=0;s<96;s++){campaign_pack(&p,&c,&a,s);assert(campaign_packet_valid(&p));}
    puts("PASS: rotating wire state: weapon ids in four snapshots, pets and 64 level clears in a 32-snapshot cycle, refused unknown ids");
    return 0;
}
