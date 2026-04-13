#ifndef MASTER_RESULTS_H
#define MASTER_RESULTS_H

#include <sys/types.h>

#include <game_state.h>

void wait_for_children(pid_t view_pid, game_state_t *game_state, int num_players);
void print_winner(const game_state_t *game_state, int num_players);

#endif
