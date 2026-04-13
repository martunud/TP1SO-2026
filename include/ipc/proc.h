#ifndef IPC_PROC_H
#define IPC_PROC_H

#include <sys/types.h>

#include <game_state.h>

int init_view_players(unsigned short width, unsigned short height, unsigned char players_amount,
                      char *player_paths[], char *view_path, pid_t *view_pid,
                      game_state_t *game_state, int player_pipes[][2]);

pid_t view_fork(unsigned short width, unsigned short height, char *view_path);
pid_t player_fork(unsigned short width, unsigned short height, char *player_path,
                  int player_pipes[][2], int num_players, int player_index,
                  game_state_t *game_state);

#endif