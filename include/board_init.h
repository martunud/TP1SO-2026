#ifndef BOARD_INIT_H
#define BOARD_INIT_H

#include <game_state.h>

void fill_board(game_state_t *game_state, unsigned short width, unsigned short height);
void place_player(game_state_t *game_state, int i, unsigned short width, unsigned short height, int players_amount);

#endif