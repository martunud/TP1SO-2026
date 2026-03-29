#ifndef ARGS_H
#define ARGS_H

#include <master.h>


int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players );

#endif