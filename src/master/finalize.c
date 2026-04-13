#include <master/finalize.h>

#include <ipc/shm.h>
#include <ipc/sync.h>

#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>

int set_game_ended(game_state_t *game_state, sync_t *sync) {
    if (sem_wait(&sync->writer_mutex) == -1) {
        perror("sem_wait writer_mutex");
        return -1;
    }
    if (sem_wait(&sync->state_mutex) == -1) {
        perror("sem_wait state_mutex");
        sem_post(&sync->writer_mutex);
        return -1;
    }
    if (sem_post(&sync->writer_mutex) == -1) {
        perror("sem_post writer_mutex");
        sem_post(&sync->state_mutex);
        return -1;
    }

    game_state->ended = true;

    if (sem_post(&sync->state_mutex) == -1) {
        perror("sem_post state_mutex");
        return -1;
    }
    return 0;
}

int notify_view_shutdown(sync_t *sync, pid_t view_pid) {
    if (view_pid == -1) {
        return 0;
    }
    if (sem_post(&sync->state_changed) == -1) {
        perror("sem_post state_changed");
        return -1;
    }
    if (sem_wait(&sync->view_done) == -1) {
        perror("sem_wait view_done");
        return -1;
    }
    return 0;
}

int wake_players(sync_t *sync, int num_players) {
    int status = 0;

    for (int i = 0; i < num_players; i++) {
        if (sem_post(&sync->move_processed[i]) == -1) {
            perror("sem_post move_processed");
            status = -1;
        }
    }

    return status;
}

void close_player_pipes(int player_pipes[][2], int num_players) {
    for (int i = 0; i < num_players; i++) {
        close(player_pipes[i][0]);
    }
}

int cleanup_master_resources(game_state_t *game_state, sync_t *sync,
                             unsigned short width, unsigned short height,
                             unsigned char players_amount) {
    int status = 0;

    if (destroy_sync(sync, players_amount) == -1) {
        status = -1;
    }
    if (unlink_game_shm() == -1) {
        status = -1;
    }
    if (unlink_sync_shm() == -1) {
        status = -1;
    }
    if (close_game_shm(game_state, width, height) == -1) {
        status = -1;
    }
    if (close_shm_sync(sync) == -1) {
        status = -1;
    }

    return status;
}
