#pragma GCC optimize("Os") /* Not a per-pixel path; the DS EWRAM budget favors size. */
#include "campaign_save.h"
#include "pets.h"
#include <stdio.h>
#include <string.h>
static CampaignCharacter characters[4];
static char path_prefix[128];
static uint32_t generation;
static int active=-1,dirty;
void campaign_character_default(CampaignCharacter *p,unsigned knight){memset(p,0,sizeof(*p));p->profile.level=1;p->profile.knight=knight;weapon_set_add(&p->owned,0);}
int campaign_character_valid(const CampaignCharacter *p){
    if(!p||p->profile.knight>3||p->profile.level<1||p->profile.level>99||p->profile.xp>131071||p->potions>5||(p->tools&~3u)||!pets_profile_valid(&(PetProfile){.owned=p->pets_owned,.equipped=p->pet}))return 0;
    for(unsigned id=WEAPON_COUNT;id<CONTENT_WEAPON_IDS;id++)if(weapon_set_has(&p->owned,id))return 0; /* Only registered weapons. */
    if(!weapon_set_has(&p->owned,0)||!campaign_weapon_can_equip(&p->owned,p->weapon,p->profile.level))return 0;
    unsigned level=1;while(level<99&&p->profile.xp>keep_xp_threshold(level))level++;
    if(level!=p->profile.level)return 0;
    unsigned used=0;for(int i=0;i<4;i++){if(p->stats[i]>24)return 0;used+=p->stats[i];}
    return used<=campaign_stat_budget(level);
}
void campaign_character_get(CampaignCharacter *p,const Campaign *c,unsigned h){
    *p=(CampaignCharacter){.profile=c->profiles[h],.weapon=c->weapons[h],.owned=c->owned_weapons[h],.potions=c->potions[h],.cleared=c->completed,.pets_owned=c->pets[h].owned,.pet=c->pets[h].equipped,.tools=c->tools[h]};
    weapon_set_add(&p->owned,0);memcpy(p->stats,c->stats[h],4);
}
void campaign_character_put(const CampaignCharacter *p,Campaign *c,unsigned h){
    c->profiles[h]=p->profile;c->weapons[h]=p->weapon;c->owned_weapons[h]=p->owned;weapon_set_add(&c->owned_weapons[h],0);weapon_set_add(&c->owned_weapons[h],p->weapon);
    c->potions[h]=p->potions;c->pets[h]=(PetProfile){.owned=p->pets_owned,.equipped=p->pet};c->tools[h]=p->tools;c->completed|=p->cleared;memcpy(c->stats[h],p->stats,4);
}
static uint32_t get32(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void put32(uint8_t *p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i));}
static uint64_t get64(const uint8_t *p){return get32(p)|((uint64_t)get32(p+4)<<32);}
static void put64(uint8_t *p,uint64_t v){put32(p,(uint32_t)v);put32(p+4,(uint32_t)(v>>32));}
static uint32_t checksum(const uint8_t *p,unsigned n){uint32_t v=0xffffffff;while(n--){v^=*p++;for(int i=0;i<8;i++)v=(v>>1)^(v&1?0xedb88320:0);}return ~v;}
#define SAVE_V5_CHARACTER 48u
#define SAVE_V5_BYTES (12u+4u*SAVE_V5_CHARACTER+4u)
static void encode(uint8_t b[SAVE_V5_BYTES],uint32_t seq){
    memset(b,0,SAVE_V5_BYTES);memcpy(b,"CCDS",4);b[4]=5;b[6]=SAVE_V5_BYTES&255;b[7]=SAVE_V5_BYTES>>8;put32(b+8,seq);
    for(int i=0;i<4;i++){
        const CampaignCharacter *p=&characters[i];uint8_t *d=b+12+i*SAVE_V5_CHARACTER;
        put32(d,p->profile.xp);d[4]=p->profile.gold;d[5]=p->profile.gold>>8;d[6]=p->profile.level;d[7]=p->profile.knight;
        memcpy(d+8,p->stats,4);d[12]=p->weapon;d[13]=p->potions;d[14]=p->pet;d[15]=p->tools;
        put64(d+16,p->pets_owned);put64(d+24,p->cleared);
        for(unsigned k=0;k<CONTENT_WEAPON_IDS/32;k++)put32(d+32+4*k,p->owned.w[k]);
    }
    put32(b+SAVE_V5_BYTES-4,checksum(b,SAVE_V5_BYTES-4));
}
/* v1 (80 bytes, 16-byte records), v2-v4 (112 bytes, 24-byte records) and v5 (208 bytes) all load; the next save is v5. */
static int decode(const uint8_t *b,unsigned length,CampaignCharacter out[4],uint32_t *seq){
    unsigned version=length>5?b[4]:0,record=version==1?16:version==5?SAVE_V5_CHARACTER:24;
    if(!((length==80&&version==1)||(length==112&&version>=2&&version<=4)||(length==SAVE_V5_BYTES&&version==5)))return 0;
    if(memcmp(b,"CCDS",4)||b[5]||(b[6]|((unsigned)b[7]<<8))!=length||get32(b+length-4)!=checksum(b,length-4))return 0;
    for(int i=0;i<4;i++){
        const uint8_t *d=b+12+i*record;CampaignCharacter *p=&out[i];
        *p=(CampaignCharacter){.profile={.xp=get32(d),.gold=d[4]|(d[5]<<8),.level=d[6],.knight=d[7]}};memcpy(p->stats,d+8,4);
        if(version==5){
            p->weapon=d[12];p->potions=d[13];p->pet=d[14];p->tools=d[15];p->pets_owned=get64(d+16);p->cleared=get64(d+24);
            for(unsigned k=0;k<CONTENT_WEAPON_IDS/32;k++)p->owned.w[k]=get32(d+32+4*k);
        }else{
            p->weapon=d[12];p->owned=weapon_set_low(d[13]);p->potions=d[14];p->cleared=d[15];
            if(version==1){if(p->cleared>1)return 0;}
            else{if((version==2&&p->cleared>3)||(version==3&&p->cleared>7))return 0;p->pets_owned=get32(d+16);p->pet=d[20];p->tools=d[21];if(d[22]||d[23])return 0;}
        }
        if(p->profile.knight!=i||!campaign_character_valid(p))return 0;
    }
    *seq=get32(b+8);return (int)version;
}
static void filename(char path[144],int bank){snprintf(path,144,"%s.%d.sav",path_prefix,bank);}
static int read_bank(int bank,CampaignCharacter out[4],uint32_t *seq){
    char path[144];uint8_t b[SAVE_V5_BYTES];filename(path,bank);FILE *f=fopen(path,"rb");if(!f)return 0;
    unsigned n=fread(b,1,sizeof b,f);int extra=fgetc(f),err=ferror(f);fclose(f);
    return (n==80||n==112||n==SAVE_V5_BYTES)&&extra==EOF&&!err?decode(b,n,out,seq):0;
}
int campaign_save_open(const char *prefix){
    active=-1;generation=0;dirty=0;path_prefix[0]=0;
    for(int i=0;i<4;i++)campaign_character_default(&characters[i],i);
    if(!prefix||strlen(prefix)>=sizeof(path_prefix))return 0;
    strcpy(path_prefix,prefix);
    CampaignCharacter candidate[4];uint32_t seq;
    for(int i=0;i<2;i++){int version=read_bank(i,candidate,&seq);if(version&&(active<0||(int32_t)(seq-generation)>0)){memcpy(characters,candidate,sizeof(characters));generation=seq;active=i;dirty=version<5;}}
    return active>=0;
}
const CampaignCharacter *campaign_save_character(unsigned knight){return &characters[knight<4?knight:0];}
void campaign_save_capture(const Campaign *c,unsigned h){
    if(h>1)return;
    CampaignCharacter p;campaign_character_get(&p,c,h);
    if(!campaign_character_valid(&p))return;
    CampaignCharacter *old=&characters[p.profile.knight];p.cleared|=old->cleared;
    if(memcmp(old,&p,sizeof(p))){*old=p;dirty=1;}
}
int campaign_save_dirty(void){return dirty;}
int campaign_save_flush(void){
    if(!dirty)return 1;
    if(!path_prefix[0])return 0;
    uint8_t b[SAVE_V5_BYTES];encode(b,generation+1);char path[144];int bank=active==0?1:0;filename(path,bank);
    FILE *f=fopen(path,"wb");if(!f)return 0;
    int ok=fwrite(b,1,sizeof b,f)==sizeof b;if(fflush(f))ok=0;if(fclose(f))ok=0;
    CampaignCharacter verify[4];uint32_t seq;
    if(!ok||!read_bank(bank,verify,&seq)||seq!=generation+1||memcmp(verify,characters,sizeof(characters)))return 0;
    active=bank;generation=seq;dirty=0;return 1;
}
