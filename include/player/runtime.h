#ifndef PLAYER_RUNTIME_H
#define PLAYER_RUNTIME_H

#include <game_state.h>
#include <game_sync.h>

int parse_player_args(int argc, char *argv[], unsigned short *width,
                      unsigned short *height, int *idx);
int run_player_loop(game_state_t *game_state, sync_t *sync, int idx);

#endif
