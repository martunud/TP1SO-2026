#include <master.h>

game_state_t * create_game_shm(unsigned short width, unsigned short height);

void init_game_state(game_state_t * game_state , unsigned short width, unsigned short height, unsigned char players_amount, player_t players[9], unsigned int * seed);

static int checkCoord(game_state_t * game_state,unsigned short width, int x, int y);

sync_t * create_shm_sync();