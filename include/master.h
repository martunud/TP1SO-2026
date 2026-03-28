#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//Para la shared memory -> en man shm_open
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           

#include <game_state.h>
#include <game_sync.h> 

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

#define MAX_PLAYERS 9

int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players );
game_state_t * create_shm(unsigned short width, unsigned short height); 
void init_game_state(game_state_t * game_state , unsigned short width, unsigned short height, unsigned char players_amount, player_t players[9], unsigned int * seed);
