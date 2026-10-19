#ifndef CAMPAIGN_SAVE_H
#define CAMPAIGN_SAVE_H
#include "campaign.h"
/* Per-knight record (save file v5 and the guest's profile message). Sets are sized for the whole game. */
typedef struct {
    CampaignProfile profile;
    uint8_t stats[4],weapon,potions,pet,tools;
    uint64_t pets_owned; /* bit per pet id */
    uint64_t cleared;    /* bit per level id */
    WeaponSet owned;     /* bit per weapon id */
} CampaignCharacter;
_Static_assert(sizeof(CampaignCharacter)==48,"Character storage layout");
void campaign_character_default(CampaignCharacter *p,unsigned knight);
int campaign_character_valid(const CampaignCharacter *p);
void campaign_character_get(CampaignCharacter *p,const Campaign *c,unsigned hero);
void campaign_character_put(const CampaignCharacter *p,Campaign *c,unsigned hero);
/* Two checksummed generations: a torn write never replaces the active bank. */
int campaign_save_open(const char *prefix);
const CampaignCharacter *campaign_save_character(unsigned knight);
void campaign_save_capture(const Campaign *c,unsigned hero);
int campaign_save_flush(void);
int campaign_save_dirty(void);
#endif
