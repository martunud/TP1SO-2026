#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define MAX_PLAYERS 9

int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players );
