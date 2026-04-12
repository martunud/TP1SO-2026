#include <board_init.h>
#include <stdlib.h>  

void fill_board(game_state_t *game_state, unsigned short width, unsigned short height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            game_state->board[i * width + j] = rand() % 9 + 1;
        }
    }
}

void place_player(game_state_t *game_state, int i, unsigned short width, unsigned short height, int players_amount) {
    int sector_h  = height / players_amount;
    int row_start = i * sector_h;
    int row_end   = (i == players_amount - 1) ? height : row_start + sector_h;
    int cx = row_start + (row_end - row_start) / 2;
    int cy = width / 2;
    int x = -1, y = -1;

    for (int radius = 0; radius <= height + width && x == -1; radius++) {
        for (int dx = -radius; dx <= radius && x == -1; dx++) {
            for (int dy = -radius; dy <= radius && x == -1; dy++) {
                if (abs(dx) != radius && abs(dy) != radius) continue;
                int nx = cx + dx, ny = cy + dy;
                if (nx >= 0 && nx < height && ny >= 0 && ny < width
                    && game_state->board[nx * width + ny] > 0) {
                    x = nx;
                    y = ny;
                }
            }
        }
    }

    game_state->board[x * width + y] = (signed char)(-i);
    game_state->players[i].x = (unsigned short)x;
    game_state->players[i].y = (unsigned short)y;
}