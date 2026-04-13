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
                      unsigned short *height) {
    if (argc != 3) {
        fprintf(stderr, "Error: expected 2 arguments (width height)\n");
        return -1;
    }

    *width  = (unsigned short)atoi(argv[1]);
    *height = (unsigned short)atoi(argv[2]);
    return 0;
}

int find_player_index(const game_state_t *game_state) {
    pid_t my_pid = getpid();

    for (int i = 0; i < game_state->players_amount; i++) {
        if (game_state->players[i].pid == my_pid) {
            return i;
        }
    }

    fprintf(stderr, "Error: could not find player index for PID %d\n", (int)my_pid);
    return -1;
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

        bool ended   = game_state->ended;
        bool blocked = game_state->players[idx].blocked;
        unsigned short x    = game_state->players[idx].x;
        unsigned short y    = game_state->players[idx].y;
        unsigned char  move = pick_direction(game_state, x, y);

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