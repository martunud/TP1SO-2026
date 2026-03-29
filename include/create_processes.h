#ifndef CREATE_PRROCESS_H
#define CREATE_PRROCESS_H

#include <master.h>

pid_t view_fork(unsigned short width, unsigned short height, char * view_path);
pid_t player_fork(unsigned short width, unsigned short height, char * player_path, int * player_pipe);
void init_view_players(unsigned short width, unsigned short height, unsigned char players_amount, char *player_paths[], char * view_path, pid_t * view_pid, pid_t players_pid[], int player_pipes[][2]);

#endif