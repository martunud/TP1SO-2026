#ifndef MASTER_GAME_H
#define MASTER_GAME_H

#include <stdbool.h>

#include <game_state.h>

void init_players(player_t players[], char *player_paths[], int num_players);
void init_game_state(game_state_t *game_state, unsigned short width, unsigned short height,
                     unsigned char players_amount, player_t players[MAX_PLAYERS],
                     unsigned int *seed);

bool apply_move(game_state_t *game_state, int player_index, unsigned char move);

#endif
