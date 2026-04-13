#ifndef IPC_SHM_H
#define IPC_SHM_H

#include <game_state.h>
#include <game_sync.h>

game_state_t *create_game_shm(unsigned short width, unsigned short height);
sync_t *create_shm_sync(void);

game_state_t *open_game_shm(unsigned short width, unsigned short height);
sync_t *open_shm_sync(void);

int close_game_shm(game_state_t *game_state, unsigned short width, unsigned short height);
int close_shm_sync(sync_t *sync);

int unlink_game_shm(void);
int unlink_sync_shm(void);

#endif
