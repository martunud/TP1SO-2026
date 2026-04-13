#ifndef MASTER_FINALIZE_H
#define MASTER_FINALIZE_H

#include <sys/types.h>

#include <game_state.h>
#include <game_sync.h>

int set_game_ended(game_state_t *game_state, sync_t *sync);
int notify_view_shutdown(sync_t *sync, pid_t view_pid);
int wake_players(sync_t *sync, int num_players);
void close_player_pipes(int player_pipes[][2], int num_players);
int cleanup_master_resources(game_state_t *game_state, sync_t *sync,
                             unsigned short width, unsigned short height,
                             unsigned char players_amount);

#endif
