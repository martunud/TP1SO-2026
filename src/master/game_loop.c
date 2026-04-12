#include <game_loop.h>
#include <shared_memory.h>
#include <sys/select.h>

static int read_player_move(game_state_t *game_state, int player_pipes[][2], int i, unsigned char *move) {
    ssize_t n = read(player_pipes[i][0], move, 1);
    if (n == 0) {
        game_state->players[i].blocked = true;
        return 0;
    }
    if (n == -1) {
        perror("read pipe");
        return -1;
    }
    return 1;
}

static int locked_apply_move(game_state_t *game_state, sync_t *sync, int i, unsigned char move) {
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

    bool valid = apply_move(game_state, i, move);

    if (sem_post(&sync->state_mutex) == -1) {
        perror("sem_post state_mutex");
        return -1;
    }
    return valid ? 1 : 0;
}

static int notify_view(sync_t *sync, unsigned int delay) {
    if (sem_post(&sync->state_changed) == -1) {
        perror("sem_post state_changed");
        return -1;
    }
    if (sem_wait(&sync->view_done) == -1) {
        perror("sem_wait view_done");
        return -1;
    }
    usleep(delay * 1000);
    return 0;
}

static int build_fd_set(game_state_t *game_state, int player_pipes[][2], int num_players, fd_set *read_fds) {
    FD_ZERO(read_fds);
    int max_fd = -1;
    for (int i = 0; i < num_players; i++) {
        if (!game_state->players[i].blocked) {
            FD_SET(player_pipes[i][0], read_fds);
            if (player_pipes[i][0] > max_fd)
                max_fd = player_pipes[i][0];
        }
    }
    return max_fd;
}

static int remaining_timeout(time_t last_valid_move, int timeout) {
    int elapsed = (int)(time(NULL) - last_valid_move);
    if (elapsed >= timeout)
        return -1;
    return timeout - elapsed;
}

static int round_robin(game_state_t *game_state, sync_t *sync, int player_pipes[][2], int num_players, int *player_start, fd_set *read_fds, unsigned int delay, pid_t view_pid) {
    for (int offset = 0; offset < num_players; offset++) {
        int i = (*player_start + offset) % num_players;

        if (game_state->players[i].blocked)          continue;
        if (!FD_ISSET(player_pipes[i][0], read_fds)) continue;

        unsigned char move;
        int r = read_player_move(game_state, player_pipes, i, &move);
        if (r <= 0) {
            *player_start = (i + 1) % num_players;
            return r; // 0 = pipe cerrado, -1 = error
        }

        int valid = locked_apply_move(game_state, sync, i, move);
        if (valid == -1) return -1;

        if (valid && view_pid != -1 && notify_view(sync, delay) == -1) return -1;

        sem_post(&sync->move_processed[i]);
        *player_start = (i + 1) % num_players;
        return valid; // 1 = válido, 0 = inválido
    }
    return 0;
}

void run_game_loop(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int timeout, unsigned int delay, pid_t view_pid) {
    int player_start = 0;
    time_t last_valid_move = time(NULL);

    while (true) {
        fd_set read_fds;
        int max_fd = build_fd_set(game_state, players_pipe, num_players, &read_fds);
        if (max_fd == -1) break; // todos bloqueados

        int secs = remaining_timeout(last_valid_move, timeout);
        if (secs == -1) break; // timeout expirado

        struct timeval tv = { .tv_sec = secs, .tv_usec = 0 };
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready <= 0) break; // timeout o error

        int result = round_robin(game_state, sync, players_pipe, num_players, &player_start, &read_fds, delay, view_pid);
        if (result == -1) break; // error fatal
        if (result == 1)  last_valid_move = time(NULL);
    }
}