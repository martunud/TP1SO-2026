#include <ipc/sync.h>

#include <stdio.h>
#include <stdlib.h>

static void init_sem(sem_t *sem, int value) {
    if (sem_init(sem, 1, (unsigned int)value) == -1) {
        perror("sem_init");
        exit(1);
    }
}

void init_sync(sync_t *sync, unsigned char players_amount) {
    init_sem(&sync->state_changed, 0);
    init_sem(&sync->view_done, 0);
    init_sem(&sync->writer_mutex, 1);
    init_sem(&sync->state_mutex, 1);
    init_sem(&sync->readers_count_mutex, 1);

    for (int i = 0; i < players_amount; i++) {
        init_sem(&sync->move_processed[i], 1);
    }
}

int destroy_sync(sync_t *sync, unsigned char players_amount) {
    if (sync == NULL) {
        return 0;
    }

    if (sem_destroy(&sync->state_changed) == -1) {
        perror("sem_destroy state_changed");
        return -1;
    }
    if (sem_destroy(&sync->view_done) == -1) {
        perror("sem_destroy view_done");
        return -1;
    }
    if (sem_destroy(&sync->writer_mutex) == -1) {
        perror("sem_destroy writer_mutex");
        return -1;
    }
    if (sem_destroy(&sync->state_mutex) == -1) {
        perror("sem_destroy state_mutex");
        return -1;
    }
    if (sem_destroy(&sync->readers_count_mutex) == -1) {
        perror("sem_destroy readers_count_mutex");
        return -1;
    }

    for (int i = 0; i < players_amount; i++) {
        if (sem_destroy(&sync->move_processed[i]) == -1) {
            perror("sem_destroy move_processed");
            return -1;
        }
    }

    return 0;
}

int reader_enter(sync_t *sync) {
    if (sem_wait(&sync->writer_mutex) == -1) {
        return -1;
    }
    if (sem_post(&sync->writer_mutex) == -1) {
        return -1;
    }
    if (sem_wait(&sync->readers_count_mutex) == -1) {
        return -1;
    }

    sync->readers_count++;
    if (sync->readers_count == 1 && sem_wait(&sync->state_mutex) == -1) {
        sync->readers_count--;
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if (sem_post(&sync->readers_count_mutex) == -1) {
        return -1;
    }
    return 0;
}

int reader_exit(sync_t *sync) {
    if (sem_wait(&sync->readers_count_mutex) == -1) {
        return -1;
    }

    sync->readers_count--;
    if (sync->readers_count == 0 && sem_post(&sync->state_mutex) == -1) {
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if (sem_post(&sync->readers_count_mutex) == -1) {
        return -1;
    }
    return 0;
}
