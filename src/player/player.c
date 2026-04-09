#include <errno.h>
#include <master.h>
#include <shared_memory.h>

static const int DIRECTIONS[8][2] = {
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1},
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}
};

static int in_bounds(unsigned short width, unsigned short height, int x, int y){
    return x >= 0 && x < height && y >= 0 && y < width;
}

static int reader_enter(sync_t *sync){
    if(sem_wait(&sync->writer_mutex) == -1){
        return -1;
    }

    if(sem_post(&sync->writer_mutex) == -1){
        return -1;
    }

    if(sem_wait(&sync->readers_count_mutex) == -1){
        return -1;
    }

    sync->readers_count++;
    if(sync->readers_count == 1 && sem_wait(&sync->state_mutex) == -1){
        sync->readers_count--;
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if(sem_post(&sync->readers_count_mutex) == -1){
        return -1;
    }

    return 0;
}

static int reader_exit(sync_t *sync){
    if(sem_wait(&sync->readers_count_mutex) == -1){
        return -1;
    }

    sync->readers_count--;
    if(sync->readers_count == 0 && sem_post(&sync->state_mutex) == -1){
        sem_post(&sync->readers_count_mutex);
        return -1;
    }

    if(sem_post(&sync->readers_count_mutex) == -1){
        return -1;
    }

    return 0;
}

static int count_future_moves(const game_state_t *game_state, unsigned short x, unsigned short y){
    int future_moves = 0;

    for(int direction = 0; direction < 8; direction++){
        int next_x = (int)x + DIRECTIONS[direction][0];
        int next_y = (int)y + DIRECTIONS[direction][1];

        if(!in_bounds(game_state->width, game_state->height, next_x, next_y)){
            continue;
        }

        if(game_state->board[next_x * game_state->width + next_y] > 0){
            future_moves++;
        }
    }

    return future_moves;
}

static unsigned char pick_direction(const game_state_t *game_state, unsigned short x, unsigned short y){
    int best_dir = -1;
    int best_reward = -1;
    int best_future_moves = -1;
    int fallback_dir = -1;
    int fallback_reward = -1;

    for(int direction = 0; direction < 8; direction++){
        int next_x = (int)x + DIRECTIONS[direction][0];
        int next_y = (int)y + DIRECTIONS[direction][1];

        if(!in_bounds(game_state->width, game_state->height, next_x, next_y)){
            continue;
        }

        signed char cell = game_state->board[next_x * game_state->width + next_y];
        if(cell <= 0){
            continue;
        }

        if(cell > fallback_reward){
            fallback_reward = cell;
            fallback_dir = direction;
        }

        int future_moves = count_future_moves(game_state, (unsigned short)next_x, (unsigned short)next_y);
        if(future_moves == 0){
            continue;
        }

        if(cell > best_reward || (cell == best_reward && future_moves > best_future_moves)){
            best_dir = direction;
            best_reward = cell;
            best_future_moves = future_moves;
        }
    }

    if(best_dir != -1){
        return (unsigned char)best_dir;
    }

    if(fallback_dir != -1){
        return (unsigned char)fallback_dir;
    }

    return 0;
}

static int write_move(unsigned char move){
    size_t written_total = 0;

    while(written_total < sizeof(move)){
        ssize_t written = write(STDOUT_FILENO, ((unsigned char *)&move) + written_total, sizeof(move) - written_total);
        if(written == -1){
            if(errno == EINTR){
                continue;
            }

            perror("write move");
            return -1;
        }

        written_total += (size_t)written;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int exit_status = 0;
    game_state_t *buf_game = NULL;
    sync_t *buf_sync = NULL;

    // Esto elimina la espera activa de find_player_index (que violaba el enunciado), ya que el jugador no puede hacer nada hasta que el master lo haya inicializado en el estado del juego.

    if(argc != 4){
        fprintf(stderr, "Error parametros jugador (esperados: width height index)\n");
        return 1;
    }

    unsigned short width = (unsigned short)atoi(argv[1]);
    unsigned short height = (unsigned short)atoi(argv[2]);
    int idx = atoi(argv[3]); 

    buf_game = open_game_shm(width, height);
    buf_sync = open_shm_sync();
   

    if(idx < 0 || idx >= buf_game->players_amount){
        fprintf(stderr, "Indice de jugador invalido: %d\n", idx);
        exit_status = 1;
        goto cleanup;
    }

    while(true){
        if(sem_wait(&buf_sync->move_processed[idx]) == -1){
            if(errno == EINTR){
                continue;
            }

            perror("sem_wait move_processed");
            exit_status = 1;
            goto cleanup;
        }

        if(reader_enter(buf_sync) == -1){
            perror("reader_enter");
            exit_status = 1;
            goto cleanup;
        }

        bool ended = buf_game->ended;
        bool blocked = buf_game->players[idx].blocked;
        unsigned short x = buf_game->players[idx].x;
        unsigned short y = buf_game->players[idx].y;
        unsigned char move = pick_direction(buf_game, x, y);

        if(reader_exit(buf_sync) == -1){
            perror("reader_exit");
            exit_status = 1;
            goto cleanup;
        }

        if(ended || blocked){
            break;
        }

        if(write_move(move) == -1){
            exit_status = 1;
            goto cleanup;
        }
    }

cleanup:
    if(close_game_shm(buf_game, width, height) == -1){
        exit_status = 1;
    }

    if(close_shm_sync(buf_sync) == -1){
        exit_status = 1;
    }

    return exit_status;
}
