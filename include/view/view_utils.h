#ifndef VIEW_UTILS_H
#define VIEW_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include <master.h>

#if __has_include(<ncurses.h>)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#define CELL_WIDTH 5
#define CELL_HEIGHT 2
#define CELL_INNER_WIDTH 4

typedef struct {
    SCREEN *screen;
    bool headless;
    bool colors_enabled;
    unsigned short width;
    unsigned short height;
    uint8_t *trail;
} view_context_t;

int view_reader_enter(sync_t *sync);
int view_reader_exit(sync_t *sync);

int init_view_context(view_context_t *ctx, unsigned short width, unsigned short height);
void destroy_view_context(view_context_t *ctx);

void update_player_trail(view_context_t *ctx, const game_state_t *game_state);
void render_game_frame(view_context_t *ctx, const game_state_t *game_state, bool finished);
void render_final_frame(view_context_t *ctx, const game_state_t *game_state);

#endif
