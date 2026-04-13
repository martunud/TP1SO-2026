#include <shared_memory.h>
#include <errno.h>
#include <string.h>

static const int DIRECTIONS[8][2] = {
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1},
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}
};


static bool player_can_move(const game_state_t *game_state, int player_index) {
    int x = game_state->players[player_index].x;
    int y = game_state->players[player_index].y;

    for (int dir = 0; dir < 8; dir++) {
        int nx = x + DIRECTIONS[dir][0];
        int ny = y + DIRECTIONS[dir][1];
        if (nx < 0 || nx >= game_state->height || ny < 0 || ny >= game_state->width)
            continue;
        if (game_state->board[nx * game_state->width + ny] > 0)
            return true;
    }
    return false;
}

static void update_blocked_status(game_state_t *game_state, int player_index) {
    game_state->players[player_index].blocked = !player_can_move(game_state, player_index);
}

static size_t game_state_size(unsigned short width, unsigned short height) {
    return sizeof(game_state_t) + width * height * sizeof(signed char);
}


static void init_sem(sem_t *sem, int value) {
    if (sem_init(sem, 1, (unsigned int)value) == -1) {
        perror("sem_init");
        exit(1);
    }
}


static void *open_and_map_shm(const char *name, size_t size, int oflag, int prot) {
    int fd = shm_open(name, oflag, 0644);
    if (fd < 0) {
        perror("shm_open");
        return NULL;
    }

    if ((oflag & O_CREAT) && ftruncate(fd, (off_t)size) < 0) {
        perror("ftruncate");
        close(fd);
        shm_unlink(name);
        return NULL;
    }

    void *buf = mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    close(fd);
    if (buf == MAP_FAILED) {
        perror("mmap");
        if (oflag & O_CREAT) shm_unlink(name);
        return NULL;
    }
    return buf;
}


game_state_t *create_game_shm(unsigned short width, unsigned short height) {
    if (width == 0 || height == 0) {
        fprintf(stderr, "Error: width and height must be > 0\n");
        return NULL;
    }
    unlink_game_shm();
    size_t size = game_state_size(width, height);
    game_state_t *buf = open_and_map_shm("/game_state", size, O_CREAT | O_RDWR, PROT_READ | PROT_WRITE);
    if (buf) memset(buf, 0, size);
    return buf;
}

sync_t *create_shm_sync(void) {
    unlink_sync_shm();
    sync_t *buf = open_and_map_shm("/game_sync", sizeof(sync_t), O_CREAT | O_RDWR, PROT_READ | PROT_WRITE);
    if (buf) memset(buf, 0, sizeof(sync_t));
    return buf;
}


game_state_t *open_game_shm(unsigned short width, unsigned short height) {
    game_state_t *buf = open_and_map_shm("/game_state", game_state_size(width, height), O_RDONLY, PROT_READ);
    if (buf == NULL) exit(1);
    return buf;
}

sync_t *open_shm_sync(void) {
    sync_t *buf = open_and_map_shm("/game_sync", sizeof(sync_t), O_RDWR, PROT_READ | PROT_WRITE);
    if (buf == NULL) exit(1);
    return buf;
}

int close_game_shm(game_state_t *game_state, unsigned short width, unsigned short height) {
    if (game_state == NULL) return 0;
    if (munmap(game_state, game_state_size(width, height)) == -1) {
        perror("munmap game_state");
        return -1;
    }
    return 0;
}

int close_shm_sync(sync_t *sync) {
    if (sync == NULL) return 0;
    if (munmap(sync, sizeof(sync_t)) == -1) {
        perror("munmap game_sync");
        return -1;
    }
    return 0;
}

int unlink_game_shm(void) {
    if (shm_unlink("/game_state") == -1 && errno != ENOENT) {
        perror("shm_unlink /game_state");
        return -1;
    }
    return 0;
}

int unlink_sync_shm(void) {
    if (shm_unlink("/game_sync") == -1 && errno != ENOENT) {
        perror("shm_unlink /game_sync");
        return -1;
    }
    return 0;
}


void init_players(player_t players[], char *player_paths[], int num_players) {
    for (int i = 0; i < num_players; i++) {
        strncpy(players[i].players_name, player_paths[i], 16);
        players[i].players_name[15] = '\0';
        players[i].score        = 0;
        players[i].valid_moves  = 0;
        players[i].invalid_moves = 0;
        players[i].blocked      = false;
    }
}

void init_game_state(game_state_t *game_state, unsigned short width, unsigned short height,
                     unsigned char players_amount, player_t players[9], unsigned int *seed) {
    game_state->width          = width;
    game_state->height         = height;
    game_state->players_amount = players_amount;

    for (int i = 0; i < players_amount; i++)
        game_state->players[i] = players[i];

    srand(*seed);
    fill_board(game_state, width, height);

    for (int i = 0; i < players_amount; i++)
        place_player(game_state, i, width, height, players_amount);

    for (int i = 0; i < players_amount; i++)
        update_blocked_status(game_state, i);
}


void init_sync(sync_t *sync, unsigned char players_amount) {
    init_sem(&sync->state_changed,       0);
    init_sem(&sync->view_done,           0);
    init_sem(&sync->writer_mutex,        1);
    init_sem(&sync->state_mutex,         1);
    init_sem(&sync->readers_count_mutex, 1);

    for (int i = 0; i < players_amount; i++)
        init_sem(&sync->move_processed[i], 1);
}

int destroy_sync(sync_t *sync, unsigned char players_amount) {
    if (sync == NULL) return 0;

    if (sem_destroy(&sync->state_changed)       == -1) { perror("sem_destroy state_changed");       return -1; }
    if (sem_destroy(&sync->view_done)            == -1) { perror("sem_destroy view_done");            return -1; }
    if (sem_destroy(&sync->writer_mutex)         == -1) { perror("sem_destroy writer_mutex");         return -1; }
    if (sem_destroy(&sync->state_mutex)          == -1) { perror("sem_destroy state_mutex");          return -1; }
    if (sem_destroy(&sync->readers_count_mutex)  == -1) { perror("sem_destroy readers_count_mutex");  return -1; }

    for (int i = 0; i < players_amount; i++) {
        if (sem_destroy(&sync->move_processed[i]) == -1) {
            perror("sem_destroy move_processed");
            return -1;
        }
    }
    return 0;
}


static bool validate_move(game_state_t *game_state, int player_index, unsigned char move,
                           int *out_x, int *out_y) {
    if (move > 7) {
        game_state->players[player_index].invalid_moves++;
        update_blocked_status(game_state, player_index);
        return false;
    }

    int nx = (int)game_state->players[player_index].x + DIRECTIONS[move][0];
    int ny = (int)game_state->players[player_index].y + DIRECTIONS[move][1];

    if (nx < 0 || nx >= game_state->height || ny < 0 || ny >= game_state->width
        || game_state->board[nx * game_state->width + ny] <= 0) {
        game_state->players[player_index].invalid_moves++;
        update_blocked_status(game_state, player_index);
        return false;
    }

    *out_x = nx;
    *out_y = ny;
    return true;
}

bool apply_move(game_state_t *game_state, int player_index, unsigned char move) {
    int nx, ny;
    if (!validate_move(game_state, player_index, move, &nx, &ny))
        return false;

    signed char cell = game_state->board[nx * game_state->width + ny];
    game_state->board[nx * game_state->width + ny]  = (signed char)(-player_index);
    game_state->players[player_index].score         += (unsigned int)cell;
    game_state->players[player_index].valid_moves   += 1;
    game_state->players[player_index].x              = (unsigned short)nx;
    game_state->players[player_index].y              = (unsigned short)ny;
    update_blocked_status(game_state, player_index);
    return true;
}