#ifndef PETS_H
#define PETS_H
#include <stdint.h>
enum { PET_NONE=0,PET_OWLET=2,PET_TROLL=9,PET_DINO=21,PET_SNOOT=17,PET_MR_BUDDY=24,PET_IGNORE_TICKS=51,PET_LOVE_TICKS=60 };
#define PET_SUPPORTED_MASK ((UINT64_C(1)<<PET_OWLET)|(UINT64_C(1)<<PET_TROLL)|(UINT64_C(1)<<PET_DINO)|(UINT64_C(1)<<PET_SNOOT)|(UINT64_C(1)<<PET_MR_BUDDY))
typedef struct {uint64_t owned;uint8_t equipped;} PetProfile; /* owned: one bit per pet id below CONTENT_PET_IDS */
typedef struct {int16_t x,y;int8_t fraction_x,fraction_y;uint8_t love_timer;int8_t facing;} PetFollower;
typedef struct {int16_t x,y;uint8_t source_id,ignore_timer,unlock,z;} PetDrop;
typedef struct {const char *name;int8_t strength,defense,magic,agility;uint8_t faster_dig;} PetInfo;
const PetInfo *pets_info(unsigned source_id);
int pets_profile_valid(const PetProfile *profile);
/* Canonical nine-byte little-endian save representation; never persist padding. */
int pets_profile_encode(uint8_t out[9],const PetProfile *profile);
int pets_profile_decode(PetProfile *profile,const uint8_t in[9]);
int pets_owned(const PetProfile *profile,unsigned source_id);
/* Call on the host only. The consumed slot holds the old pet after a swap.
 * A source unlock is shared with living participants; physical pet stays with
 * the collecting hero. Locked non-unlock pets cannot be taken from the Ark. */
int pets_pickup(PetProfile profiles[2],unsigned alive_mask,unsigned hero,
                PetFollower *follower,PetDrop *drop,int hero_x,int hero_y);
/* Changing/removing an equipped pet requires an EMPTY drop slot for the old
 * pet. Every failure leaves profile, follower and drop unchanged. */
int pets_equip_owned(PetProfile *profile,PetFollower *follower,unsigned source_id,
                    PetDrop *empty_drop,int hero_x,int hero_y);
int pets_place(PetDrop *empty_drop,unsigned source_id,int x,int y,int unlock);
void pets_drop_step(PetDrop *drop);
void pets_follow_step(PetFollower *pet,int owner_x,int owner_y,int owner_facing);
/* Dig completion is the caller's responsibility; this enforces the original
 * shovel requirement without granting Buddy merely by entering the barracks. */
int pets_unearth_buddy(PetDrop *empty_drop,int has_shovel,int dig_finished,int x,int y);
#endif
