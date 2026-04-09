#ifndef CREATE_PROCESSES_H
#define CREATE_PROCESSES_H

#include <master.h>

void init_view_players(unsigned short width, unsigned short height, unsigned char players_amount, char *player_paths[], char * view_path, pid_t * view_pid, game_state_t * game_state, int player_pipes[][2]);

pid_t view_fork(unsigned short width, unsigned short height, char * view_path);

pid_t player_fork(unsigned short width, unsigned short height, char * player_path, int player_pipes[][2], int num_players, int player_index);

#endif