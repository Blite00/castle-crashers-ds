#ifndef KNIGHTS_H
#define KNIGHTS_H
enum { KNIGHT_BLUE, KNIGHT_ORANGE, KNIGHT_RED, KNIGHT_GREEN, KNIGHT_COUNT };
const char *arena_knight_name(unsigned knight);
int arena_spell_kind(unsigned knight);
#endif
