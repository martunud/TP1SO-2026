#include <player/runtime.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ipc/sync.h>
#include <player/strategy.h>

static int write_move(unsigned char move) {
    size_t written_total = 0;

    while (written_total < sizeof(move)) {
        ssize_t written = write(STDOUT_FILENO, ((unsigned char *)&move) + written_total,
                                sizeof(move) - written_total);
        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("write move");
            return -1;
        }
        written_total += (size_t)written;
    }

    return 0;
}

int parse_player_args(int argc, char *argv[], unsigned short *width,
                      unsigned short *height, int *idx) {
    if (argc != 4) {
        fprintf(stderr, "Error: expected 3 arguments (width height index)\n");
        return -1;
    }

    *width = (unsigned short)atoi(argv[1]);
    *height = (unsigned short)atoi(argv[2]);
    *idx = atoi(argv[3]);
    return 0;
}

int run_player_loop(game_state_t *game_state, sync_t *sync, int idx) {
    while (true) {
        if (sem_wait(&sync->move_processed[idx]) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("sem_wait move_processed");
            return -1;
        }

        if (reader_enter(sync) == -1) {
            perror("reader_enter");
            return -1;
        }

        bool ended = game_state->ended;
        bool blocked = game_state->players[idx].blocked;
        unsigned short x = game_state->players[idx].x;
        unsigned short y = game_state->players[idx].y;
        unsigned char move = pick_direction(game_state, x, y);

        if (reader_exit(sync) == -1) {
            perror("reader_exit");
            return -1;
        }

        if (ended || blocked) {
            break;
        }

        if (write_move(move) == -1) {
            return -1;
        }
    }

    return 0;
}
