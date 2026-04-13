#include <errno.h>
#include <master.h>
#include <shared_memory.h>

static const int DIRECTIONS[8][2] = {
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1},
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}
};


static int in_bounds(unsigned short width, unsigned short height, int x, int y) {
    return x >= 0 && x < height && y >= 0 && y < width;
}

static int reader_enter(sync_t *sync) {
    if (sem_wait(&sync->writer_mutex) == -1)        return -1;
    if (sem_post(&sync->writer_mutex) == -1)        return -1;
    if (sem_wait(&sync->readers_count_mutex) == -1) return -1;

    sync->readers_count++;
    if (sync->readers_count == 1 && sem_wait(&sync->state_mutex) == -1) {
        sync->readers_count--;
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if (sem_post(&sync->readers_count_mutex) == -1) return -1;
    return 0;
}

static int reader_exit(sync_t *sync) {
    if (sem_wait(&sync->readers_count_mutex) == -1) return -1;

    sync->readers_count--;
    if (sync->readers_count == 0 && sem_post(&sync->state_mutex) == -1) {
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if (sem_post(&sync->readers_count_mutex) == -1) return -1;
    return 0;
}


static int count_future_moves(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int future_moves = 0;
    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];
        if (in_bounds(game_state->width, game_state->height, nx, ny)
            && game_state->board[nx * game_state->width + ny] > 0) {
            future_moves++;
        }
    }
    return future_moves;
}


static int pick_best_dir(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int best_dir          = -1;
    int best_reward       = -1;
    int best_future_moves = -1;

    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];

        if (!in_bounds(game_state->width, game_state->height, nx, ny)) continue;
        signed char cell = game_state->board[nx * game_state->width + ny];
        if (cell <= 0) continue;

        int future = count_future_moves(game_state, (unsigned short)nx, (unsigned short)ny);
        if (future == 0) continue;

        if (cell > best_reward || (cell == best_reward && future > best_future_moves)) {
            best_dir          = dir;
            best_reward       = cell;
            best_future_moves = future;
        }
    }
    return best_dir;
}


static int pick_fallback_dir(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int best_dir    = -1;
    int best_reward = -1;

    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];

        if (!in_bounds(game_state->width, game_state->height, nx, ny)) continue;
        signed char cell = game_state->board[nx * game_state->width + ny];
        if (cell <= 0) continue;

        if (cell > best_reward) {
            best_dir    = dir;
            best_reward = cell;
        }
    }
    return best_dir;
}


static unsigned char pick_direction(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int dir = pick_best_dir(game_state, x, y);
    if (dir == -1)
        dir = pick_fallback_dir(game_state, x, y);
    if (dir == -1)
        dir = 0;
    return (unsigned char)dir;
}


static int write_move(unsigned char move) {
    size_t written_total = 0;
    while (written_total < sizeof(move)) {
        ssize_t written = write(STDOUT_FILENO, ((unsigned char *)&move) + written_total, sizeof(move) - written_total);
        if (written == -1) {
            if (errno == EINTR) continue;
            perror("write move");
            return -1;
        }
        written_total += (size_t)written;
    }
    return 0;
}


static int parse_player_args(int argc, char *argv[], unsigned short *width, unsigned short *height, int *idx) {
    if (argc != 4) {
        fprintf(stderr, "Error: se esperan 3 argumentos (width height index)\n");
        return -1;
    }
    *width  = (unsigned short)atoi(argv[1]);
    *height = (unsigned short)atoi(argv[2]);
    *idx    = atoi(argv[3]);
    return 0;
}


static int player_loop(game_state_t *buf_game, sync_t *buf_sync, int idx) {
    while (true) {
        if (sem_wait(&buf_sync->move_processed[idx]) == -1) {
            if (errno == EINTR) continue;
            perror("sem_wait move_processed");
            return -1;
        }

        if (reader_enter(buf_sync) == -1) { perror("reader_enter"); return -1; }

        bool ended  = buf_game->ended;
        bool blocked = buf_game->players[idx].blocked;
        unsigned short x = buf_game->players[idx].x;
        unsigned short y = buf_game->players[idx].y;
        unsigned char move = pick_direction(buf_game, x, y);

        if (reader_exit(buf_sync) == -1) { perror("reader_exit"); return -1; }

        if (ended || blocked) break;

        if (write_move(move) == -1) return -1;
    }
    return 0;
}


int main(int argc, char *argv[]) {
    unsigned short width, height;
    int idx;

    if (parse_player_args(argc, argv, &width, &height, &idx) == -1)
        return 1;

    game_state_t *buf_game = open_game_shm(width, height);
    sync_t       *buf_sync = open_shm_sync();

    if (idx < 0 || idx >= buf_game->players_amount) {
        fprintf(stderr, "Indice de jugador invalido: %d\n", idx);
        close_game_shm(buf_game, width, height);
        close_shm_sync(buf_sync);
        return 1;
    }

    int exit_status = player_loop(buf_game, buf_sync, idx);

    if (close_game_shm(buf_game, width, height) == -1) exit_status = 1;
    if (close_shm_sync(buf_sync) == -1)                exit_status = 1;

    return exit_status == 0 ? 0 : 1;
}