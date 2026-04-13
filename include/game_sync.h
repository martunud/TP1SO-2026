#ifndef GAME_SYNC_H
#define GAME_SYNC_H
#include <semaphore.h>

typedef struct {
    sem_t state_changed;
    sem_t view_done;
    sem_t writer_mutex;
    sem_t state_mutex;
    sem_t readers_count_mutex;
    unsigned int readers_count;
    sem_t move_processed[9];
} sync_t;

#endif
