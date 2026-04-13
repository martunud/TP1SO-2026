#ifndef GAME_STATE_H
#define GAME_STATE_H

#define MAX_PLAYERS 9

#include <stdbool.h>
#include <sys/types.h>

typedef struct {
    char players_name[16];
    unsigned int score;
    unsigned int invalid_moves;
    unsigned int valid_moves;
    unsigned short x, y;
    pid_t pid;
    bool blocked;
} player_t;

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned char players_amount;
    player_t players[MAX_PLAYERS];
    bool ended;
    signed char board[];
} game_state_t;

#endif
