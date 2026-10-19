#ifndef SPECIES_H
#define SPECIES_H
/* Enemy species beyond the barbarian (knight_id 0) and thief (knight_id 1) bodies. The species byte rides in the high
 * nibble of each enemy actor's attack_kind on the wire, so sixteen species fit without changing the packet. */
enum {SPECIES_DEFAULT=0,SPECIES_TROLL,SPECIES_TROLL_MOTHER,SPECIES_IMP,SPECIES_COUNT};
#endif
