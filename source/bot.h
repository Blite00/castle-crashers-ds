#ifndef BOT_H
#define BOT_H
#include "arena.h"
enum { BOT_EASY, BOT_MEDIUM, BOT_HARD, BOT_COUNT };
const char *bot_difficulty_name(int difficulty);
uint16_t bot_input(const Arena *arena,int difficulty);
#endif
