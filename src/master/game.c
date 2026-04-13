#include <master/game.h>

#include <master/board_init.h>

#include <stdlib.h>
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

        if (nx < 0 || nx >= game_state->height || ny < 0 || ny >= game_state->width) {
            continue;
        }
        if (game_state->board[nx * game_state->width + ny] > 0) {
            return true;
        }
    }

    return false;
}

static void update_blocked_status(game_state_t *game_state, int player_index) {
    game_state->players[player_index].blocked = !player_can_move(game_state, player_index);
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

    if (nx < 0 || nx >= game_state->height || ny < 0 || ny >= game_state->width ||
        game_state->board[nx * game_state->width + ny] <= 0) {
        game_state->players[player_index].invalid_moves++;
        update_blocked_status(game_state, player_index);
        return false;
    }

    *out_x = nx;
    *out_y = ny;
    return true;
}

void init_players(player_t players[], char *player_paths[], int num_players) {
    for (int i = 0; i < num_players; i++) {
        strncpy(players[i].players_name, player_paths[i], sizeof(players[i].players_name));
        players[i].players_name[sizeof(players[i].players_name) - 1] = '\0';
        players[i].score = 0;
        players[i].valid_moves = 0;
        players[i].invalid_moves = 0;
        players[i].blocked = false;
        players[i].pid = 0;
    }
}

void init_game_state(game_state_t *game_state, unsigned short width, unsigned short height,
                     unsigned char players_amount, player_t players[MAX_PLAYERS],
                     unsigned int *seed) {
    game_state->width = width;
    game_state->height = height;
    game_state->players_amount = players_amount;
    game_state->ended = false;

    for (int i = 0; i < players_amount; i++) {
        game_state->players[i] = players[i];
    }

    srand(*seed);
    fill_board(game_state, width, height);

    for (int i = 0; i < players_amount; i++) {
        place_player(game_state, i, width, height, players_amount);
    }

    for (int i = 0; i < players_amount; i++) {
        update_blocked_status(game_state, i);
    }
}

bool apply_move(game_state_t *game_state, int player_index, unsigned char move) {
    int nx;
    int ny;

    if (!validate_move(game_state, player_index, move, &nx, &ny)) {
        return false;
    }

    signed char cell = game_state->board[nx * game_state->width + ny];
    game_state->board[nx * game_state->width + ny] = (signed char)(-player_index);
    game_state->players[player_index].score += (unsigned int)cell;
    game_state->players[player_index].valid_moves += 1;
    game_state->players[player_index].x = (unsigned short)nx;
    game_state->players[player_index].y = (unsigned short)ny;
    update_blocked_status(game_state, player_index);
    return true;
}
