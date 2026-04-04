#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <master.h>

game_state_t * create_game_shm(unsigned short width, unsigned short height);

void init_players(player_t players[], char *player_paths[], int num_players);

void init_game_state(game_state_t * game_state , unsigned short width, unsigned short height, unsigned char players_amount, player_t players[9], unsigned int * seed);

//static int checkCoord(game_state_t * game_state,unsigned short width, int x, int y);

sync_t * create_shm_sync();

void init_sync(sync_t *sync, unsigned char players_amount);

game_state_t * open_game_shm(unsigned short width, unsigned short height);

sync_t * open_shm_sync();

int close_game_shm(game_state_t *game_state, unsigned short width, unsigned short height);

int close_shm_sync(sync_t *sync);

int destroy_sync(sync_t *sync, unsigned char players_amount);

int unlink_game_shm(void);

int unlink_sync_shm(void);

#endif
