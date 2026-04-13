#include <player/strategy.h>

static const int DIRECTIONS[8][2] = {
    {-1, 0}, {-1, 1}, {0, 1}, {1, 1},
    {1, 0}, {1, -1}, {0, -1}, {-1, -1}
};

static int in_bounds(unsigned short width, unsigned short height, int x, int y) {
    return x >= 0 && x < height && y >= 0 && y < width;
}

static int count_future_moves(const game_state_t *game_state, unsigned short x,
                              unsigned short y) {
    int future_moves = 0;

    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];

        if (in_bounds(game_state->width, game_state->height, nx, ny) &&
            game_state->board[nx * game_state->width + ny] > 0) {
            future_moves++;
        }
    }

    return future_moves;
}

static int pick_best_dir(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int best_dir = -1;
    int best_reward = -1;
    int best_future_moves = -1;

    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];

        if (!in_bounds(game_state->width, game_state->height, nx, ny)) {
            continue;
        }

        signed char cell = game_state->board[nx * game_state->width + ny];
        if (cell <= 0) {
            continue;
        }

        int future = count_future_moves(game_state, (unsigned short)nx, (unsigned short)ny);
        if (future == 0) {
            continue;
        }

        if (cell > best_reward || (cell == best_reward && future > best_future_moves)) {
            best_dir = dir;
            best_reward = cell;
            best_future_moves = future;
        }
    }

    return best_dir;
}

static int pick_fallback_dir(const game_state_t *game_state, unsigned short x, unsigned short y) {
    int best_dir = -1;
    int best_reward = -1;

    for (int dir = 0; dir < 8; dir++) {
        int nx = (int)x + DIRECTIONS[dir][0];
        int ny = (int)y + DIRECTIONS[dir][1];

        if (!in_bounds(game_state->width, game_state->height, nx, ny)) {
            continue;
        }

        signed char cell = game_state->board[nx * game_state->width + ny];
        if (cell <= 0) {
            continue;
        }

        if (cell > best_reward) {
            best_dir = dir;
            best_reward = cell;
        }
    }

    return best_dir;
}

unsigned char pick_direction(const game_state_t *game_state, unsigned short x,
                             unsigned short y) {
    int dir = pick_best_dir(game_state, x, y);
    if (dir == -1) {
        dir = pick_fallback_dir(game_state, x, y);
    }
    if (dir == -1) {
        dir = 0;
    }
    return (unsigned char)dir;
}
