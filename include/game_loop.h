#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <game_state.h>
#include <game_sync.h>
#include <sys/types.h>  // pid_t

void run_game_loop(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int timeout, unsigned int delay, pid_t view_pid);

#endif