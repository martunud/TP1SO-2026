#ifndef IPC_SYNC_H
#define IPC_SYNC_H

#include <game_sync.h>

void init_sync(sync_t *sync, unsigned char players_amount);
int destroy_sync(sync_t *sync, unsigned char players_amount);

int reader_enter(sync_t *sync);
int reader_exit(sync_t *sync);

#endif
