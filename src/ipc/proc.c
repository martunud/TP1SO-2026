#include <ipc/proc.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ipc/shm.h>

static void close_player_pipe_pair(int pipefd[2]) {
    close(pipefd[0]);
    close(pipefd[1]);
}

int init_view_players(unsigned short width, unsigned short height, unsigned char players_amount,
                      char *player_paths[], char *view_path, pid_t *view_pid,
                      game_state_t *game_state, int player_pipes[][2]) {
    if (view_path != NULL) {
        *view_pid = view_fork(width, height, view_path);
        if (*view_pid < 0) {
            return -1;
        }
    } else {
        *view_pid = -1;
    }

    for (int i = 0; i < players_amount; i++) {
        if (pipe(player_pipes[i]) == -1) {
            perror("pipe");
            for (int j = 0; j < i; j++) {
                close_player_pipe_pair(player_pipes[j]);
            }
            return -1;
        }
    }

    for (int i = 0; i < players_amount; i++) {
        pid_t player_pid = player_fork(width, height, player_paths[i], player_pipes,
                                       players_amount, i, game_state);
        if (player_pid < 0) {
            for (int j = 0; j < players_amount; j++) {
                close_player_pipe_pair(player_pipes[j]);
            }
            return -1;
        }

        close(player_pipes[i][1]);
    }

    return 0;
}

pid_t view_fork(unsigned short width, unsigned short height, char *view_path) {
    pid_t pid_view = fork();
    if (pid_view == 0) {
        char *args[4];
        char buf_w[6];
        char buf_h[6];

        snprintf(buf_w, sizeof(buf_w), "%hu", width);
        snprintf(buf_h, sizeof(buf_h), "%hu", height);

        args[0] = view_path;
        args[1] = buf_w;
        args[2] = buf_h;
        args[3] = NULL;

        execv(view_path, args);
        perror("execv view");
        exit(1);
    }
    if (pid_view < 0) {
        perror("fork view");
    }

    return pid_view;
}

pid_t player_fork(unsigned short width, unsigned short height, char *player_path,
                  int player_pipes[][2], int num_players, int player_index,
                  game_state_t *game_state) {
    pid_t pid_player = fork();
    if (pid_player == 0) {
        char *args[4];
        char buf_w[6];
        char buf_h[6];

        /* Write our own PID into shared memory so the player process
         * can locate its slot by calling find_player_index(). */
        game_state->players[player_index].pid = getpid();

        for (int i = 0; i < num_players; i++) {
            if (i == player_index) {
                continue;
            }
            close(player_pipes[i][0]);
            close(player_pipes[i][1]);
        }

        dup2(player_pipes[player_index][1], STDOUT_FILENO);
        close(player_pipes[player_index][1]);
        close(player_pipes[player_index][0]);

        snprintf(buf_w, sizeof(buf_w), "%hu", width);
        snprintf(buf_h, sizeof(buf_h), "%hu", height);

        args[0] = player_path;
        args[1] = buf_w;
        args[2] = buf_h;
        args[3] = NULL;

        execv(player_path, args);
        perror("execv player");
        exit(1);
    }
    if (pid_player < 0) {
        perror("fork player");
    }

    return pid_player;
}