#include <master/results.h>

#include <stdbool.h>
#include <stdio.h>
#include <sys/wait.h>

void wait_for_children(pid_t view_pid, game_state_t *game_state, int num_players) {
    if (view_pid != -1) {
        int status;
        waitpid(view_pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("view exit: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("view killed by signal: %d\n", WTERMSIG(status));
        }
    }

    for (int i = 0; i < num_players; i++) {
        if (game_state->players[i].pid <= 0) {
            continue;
        }

        int status;
        waitpid(game_state->players[i].pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("player[%d] (%s) exit: %d score: %u\n",
                   i, game_state->players[i].players_name,
                   WEXITSTATUS(status), game_state->players[i].score);
        } else if (WIFSIGNALED(status)) {
            printf("player[%d] (%s) killed by signal: %d score: %u\n",
                   i, game_state->players[i].players_name,
                   WTERMSIG(status), game_state->players[i].score);
        }
    }
}

void print_winner(const game_state_t *game_state, int num_players) {
    int winner = 0;

    for (int i = 1; i < num_players; i++) {
        const player_t *curr = &game_state->players[i];
        const player_t *best = &game_state->players[winner];

        if (curr->score > best->score) {
            winner = i;
        } else if (curr->score == best->score) {
            if (curr->valid_moves < best->valid_moves) {
                winner = i;
            } else if (curr->valid_moves == best->valid_moves &&
                       curr->invalid_moves < best->invalid_moves) {
                winner = i;
            }
        }
    }

    bool tie = false;
    for (int i = 0; i < num_players; i++) {
        if (i == winner) {
            continue;
        }

        const player_t *curr = &game_state->players[i];
        const player_t *best = &game_state->players[winner];
        if (curr->score == best->score &&
            curr->valid_moves == best->valid_moves &&
            curr->invalid_moves == best->invalid_moves) {
            tie = true;
            break;
        }
    }

    if (tie) {
        printf("Result: TIE\n");
    } else {
        printf("Winner: player[%d] (%s) with score %u\n",
               winner, game_state->players[winner].players_name,
               game_state->players[winner].score);
    }
}
