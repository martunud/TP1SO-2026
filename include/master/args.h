#ifndef MASTER_ARGS_H
#define MASTER_ARGS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <game_state.h>

int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height,
               unsigned int *delay, int *timeout, unsigned int *seed, char **view,
               char *player[MAX_PLAYERS], int *num_players);

#endif
