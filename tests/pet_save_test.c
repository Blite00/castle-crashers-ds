#include "campaign_link.h"
#include "pets.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static const char *prefix="build/pet-save-fixture";
static void put32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static uint32_t crc(const uint8_t *p,unsigned n){uint32_t c=~0u;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320u:0);}return ~c;}
static void bank(int slot,const uint8_t *p,unsigned n){char path[100];snprintf(path,sizeof path,"%s.%d.sav",prefix,slot);FILE *f=fopen(path,"wb");assert(f);assert(fwrite(p,1,n,f)==n);assert(!fclose(f));}
static void remove_banks(void){remove("build/pet-save-fixture.0.sav");remove("build/pet-save-fixture.1.sav");}
static unsigned read_bank(int slot,uint8_t *p){char path[100];snprintf(path,sizeof path,"%s.%d.sav",prefix,slot);FILE *f=fopen(path,"rb");assert(f);unsigned n=fread(p,1,300,f);assert(!ferror(f));fclose(f);return n;}
static void legacy(uint8_t b[80],unsigned seq){
    memset(b,0,80);memcpy(b,"CCDS",4);b[4]=1;b[6]=80;put32(b+8,seq);
    for(int i=0;i<4;i++){uint8_t *d=b+12+i*16;d[4]=40+i;d[6]=1;d[7]=i;d[13]=1;d[14]=i;d[15]=i&1;}
    put32(b+76,crc(b,76));
}
/* A version 2-4 bank as the earlier builds wrote it: 24-byte records with the knight's pet in the last two bytes. */
static void v4(uint8_t b[112],unsigned version,unsigned seq,unsigned cleared){
    memset(b,0,112);memcpy(b,"CCDS",4);b[4]=(uint8_t)version;b[6]=112;put32(b+8,seq);
    for(int i=0;i<4;i++){uint8_t *d=b+12+i*24;d[4]=40+i;d[6]=1;d[7]=i;d[12]=0;d[13]=1;d[14]=i;d[15]=(uint8_t)cleared;put32(d+16,(1u<<17)|(1u<<24));d[20]=24;d[21]=1;}
    put32(b+108,crc(b,108));
}
static CampaignCharacter rich(unsigned knight){
    CampaignCharacter p;campaign_character_default(&p,knight);p.pets_owned=PET_SUPPORTED_MASK;p.pet=PET_MR_BUDDY;p.tools=1;p.cleared=3;
    weapon_set_add(&p.owned,WEAPON_CARROT);weapon_set_add(&p.owned,WEAPON_LEAF);return p;
}
#define SAVE5 208u
int main(void){
    _Static_assert(sizeof(CampaignCharacter)==48,"48-byte character");
    _Static_assert(sizeof(CampaignRequest)==sizeof(InputPacket),"guest profile rides the input reply");
    _Static_assert(offsetof(CampaignCharacter,pets_owned)==16&&offsetof(CampaignCharacter,cleared)==24&&offsetof(CampaignCharacter,owned)==32,"record layout");
    remove_banks();uint8_t old[80],bytes[300],saved[300];legacy(old,9);bank(0,old,80);
    assert(campaign_save_open(prefix)&&campaign_save_dirty());
    for(int i=0;i<4;i++){const CampaignCharacter *p=campaign_save_character(i);assert(p->profile.gold==40+i&&p->potions==i&&p->cleared==(uint64_t)(i&1));assert(!p->pet&&!p->pets_owned&&!p->tools&&weapon_set_has(&p->owned,0));}
    assert(campaign_save_flush()&&!campaign_save_dirty());assert(read_bank(1,bytes)==SAVE5&&bytes[4]==5&&bytes[6]==(SAVE5&255)&&bytes[7]==0&&bytes[8]==10);
    assert(read_bank(0,bytes)==80&&!memcmp(bytes,old,80)); /* Migration never overwrites its source bank. */
    assert(campaign_save_open(prefix)&&!campaign_save_dirty());
    Campaign c={0};CampaignCharacter p=rich(2);campaign_character_put(&p,&c,0);
    assert(c.pets[0].owned==PET_SUPPORTED_MASK&&c.pets[0].equipped==24&&c.tools[0]==1&&c.completed==3&&weapon_set_has(&c.owned_weapons[0],WEAPON_LEAF));
    CampaignCharacter roundtrip;campaign_character_get(&roundtrip,&c,0);assert(!memcmp(&p,&roundtrip,sizeof p));
    c.completed=2;campaign_save_capture(&c,0);assert(campaign_save_dirty()&&campaign_save_flush());
    assert(read_bank(0,bytes)==SAVE5);unsigned offset=12+48*2;
    assert(bytes[offset+12]==0&&bytes[offset+14]==24&&bytes[offset+15]==1);
    assert(bytes[offset+16]==4&&bytes[offset+17]==2&&bytes[offset+18]==34&&bytes[offset+19]==1&&!bytes[offset+20]&&!bytes[offset+23]); /* pets 2, 9, 17, 21, 24 */
    assert(bytes[offset+24]==2&&!bytes[offset+25]);
    assert(bytes[offset+32]==(1|(1<<WEAPON_CARROT)|(1<<WEAPON_LEAF))&&!bytes[offset+33]&&!bytes[offset+36]&&!bytes[offset+47]);
    assert(campaign_save_open(prefix));assert(campaign_save_character(2)->pet==24&&campaign_save_character(2)->tools==1);
    c.completed=1;campaign_save_capture(&c,0);assert(campaign_save_flush()&&campaign_save_character(2)->cleared==3);
    /* A torn newest bank falls back to the earlier complete pet profile. */
    bank(1,(const uint8_t *)"broken",6);assert(campaign_save_open(prefix));assert(campaign_save_character(2)->pet==24&&campaign_save_character(2)->cleared==2);
    assert(read_bank(0,saved)==SAVE5);
    /* Semantic corruption with a valid checksum is rejected, not normalized. */
    const unsigned bad_offsets[]={offset+16,offset+14,offset+15,offset+12,offset+32,offset+36};
    const uint8_t bad_values[]={1,255,4,2,0,0x40};
    for(unsigned i=0;i<6;i++){
        memcpy(bytes,saved,SAVE5);bytes[bad_offsets[i]]=bad_values[i];put32(bytes+SAVE5-4,crc(bytes,SAVE5-4));bank(0,bytes,SAVE5);
        assert(!campaign_save_open(prefix));assert(!campaign_save_character(2)->pet&&!campaign_save_character(2)->tools);
    }
    bank(0,saved,SAVE5);bank(1,saved,SAVE5-1);assert(campaign_save_open(prefix)&&campaign_save_character(2)->pet==24);
    memcpy(bytes,saved,SAVE5);bytes[SAVE5]=0;bank(1,bytes,SAVE5+1);assert(campaign_save_open(prefix)&&campaign_save_character(2)->pet==24);
    /* Versions 2, 3 and 4 (112 bytes) keep their profile and load into the larger record. */
    for(unsigned version=2;version<=4;version++){
        remove_banks();v4(bytes,version,20,version==2?2:version==3?6:10);bank(0,bytes,112);
        assert(campaign_save_open(prefix)&&campaign_save_dirty());
        const CampaignCharacter *m=campaign_save_character(2);
        assert(m->pet==24&&m->tools==1&&m->pets_owned==((1u<<17)|(1u<<24))&&m->cleared==(version==2?2u:version==3?6u:10u)&&weapon_set_has(&m->owned,0));
        assert(campaign_save_flush()&&read_bank(1,bytes)==SAVE5&&bytes[4]==5&&bytes[8]==21);
    }
    c.completed=7;campaign_save_capture(&c,0);assert(campaign_save_flush());
    assert(campaign_save_open(prefix)&&campaign_save_character(2)->cleared==15);
    /* Clear bits above the first byte survive a save (level ids up to 63). */
    c.completed=(uint64_t)1<<40|1;campaign_save_capture(&c,0);assert(campaign_save_flush());
    assert(campaign_save_open(prefix)&&(campaign_save_character(2)->cleared>>40)==1);
    /* Profile handoff installs only guest state, merges clear progress and
       rejects duplicate/later running-lobby profile replacement. */
    Arena a;arena_init(&a);a.running=0;c=(Campaign){0};c.completed=1;c.tools[0]=2;
    CampaignRequest request={.magic=CAMPAIGN_REQUEST_MAGIC,.version=CAMPAIGN_VERSION,.sequence=9,.operation=CAMPAIGN_REQUEST_PROFILE,.character=rich(3)};
    CampaignRequestState state={0};assert(campaign_request_valid(&request));assert(campaign_request_apply(&state,&request,&c,&a));
    assert(c.pets[1].equipped==24&&c.pets[1].owned==PET_SUPPORTED_MASK&&c.tools[1]==1&&c.tools[0]==2&&c.completed==3&&weapon_set_has(&c.owned_weapons[1],WEAPON_LEAF));
    assert(!campaign_request_apply(&state,&request,&c,&a));
    request.reserved0[1]=1;assert(!campaign_request_valid(&request));request.reserved0[1]=0;
    request.character.tools=4;assert(!campaign_request_valid(&request));request.character.tools=1;
    request.character.pets_owned=0;assert(!campaign_request_valid(&request));request.character.pets_owned=PET_SUPPORTED_MASK;
    weapon_set_add(&request.character.owned,90);assert(!campaign_request_valid(&request));request.character.owned=rich(3).owned;
    request.sequence++;a.running=1;request.character.pet=17;assert(!campaign_request_apply(&state,&request,&c,&a)&&c.pets[1].equipped==24&&c.menu_ack[1]==10);
    remove_banks();puts("PASS: v1-v4 migration, 208-byte v5 saves with exact pet/weapon/clear bytes, malformed/torn banks, 64 level ids and 64-byte guest profile handoff");
}
