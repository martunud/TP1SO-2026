#include <view/view_utils.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const short PLAYER_TEXT_COLORS[MAX_PLAYERS] = {
    COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE,
    COLOR_WHITE, COLOR_RED, COLOR_MAGENTA, COLOR_CYAN
};

static const short PLAYER_BG_COLORS[MAX_PLAYERS] = {
    COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE,
    COLOR_WHITE, COLOR_RED, COLOR_MAGENTA, COLOR_CYAN
};

static void init_color_pairs(void) {
    start_color();
    use_default_colors();

    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_WHITE, -1);
    init_pair(4, COLOR_RED, -1);

    for (short i = 0; i < MAX_PLAYERS; i++) {
        init_pair((short)(10 + i), PLAYER_TEXT_COLORS[i], -1);
        init_pair((short)(20 + i), COLOR_BLACK, PLAYER_BG_COLORS[i]);
    }
}

static int board_width_chars(unsigned short width) {
    return width * CELL_WIDTH + 1;
}

static int board_height_chars(unsigned short height) {
    return height * CELL_HEIGHT + 1;
}

static int total_ui_height(const game_state_t *game_state) {
    return 1 + 1 + board_height_chars(game_state->height) + 2 + game_state->players_amount + 2;
}

static bool terminal_too_small(const game_state_t *game_state) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    return cols < board_width_chars(game_state->width) || rows < total_ui_height(game_state);
}

static void draw_horizontal_border(int row, unsigned short width, chtype left, chtype mid, chtype right) {
    mvaddch(row, 0, left);
    for (int x = 0; x < width; x++) {
        for (int i = 0; i < CELL_INNER_WIDTH; i++)
            addch(ACS_HLINE);
        addch(x == width - 1 ? right : mid);
    }
}

static int player_at(const game_state_t *game_state, int x, int y) {
    for (int i = 0; i < game_state->players_amount; i++) {
        if (game_state->players[i].x == y && game_state->players[i].y == x)
            return i;
    }
    return -1;
}

static void draw_cell_fill(int row, int col, int color_pair) {
    attron(COLOR_PAIR(color_pair));
    mvaddch(row, col, ' ');
    for (int i = 0; i < CELL_INNER_WIDTH; i++)
        addch(' ');
    attroff(COLOR_PAIR(color_pair));
}

static void draw_cell_number(int row, int col, signed char value) {
    attron(COLOR_PAIR(2));
    mvprintw(row, col, " %2d ", value);
    attroff(COLOR_PAIR(2));
}

static void draw_cell(view_context_t *ctx, const game_state_t *game_state, int x, int y, int row) {
    int col           = 1 + x * CELL_WIDTH;
    int current_player = player_at(game_state, x, y);
    int trail_player   = ctx->trail[y * game_state->width + x];
    signed char cell   = game_state->board[y * game_state->width + x];

    if (ctx->colors_enabled && current_player >= 0)
        draw_cell_fill(row, col, 20 + current_player);
    else if (ctx->colors_enabled && trail_player > 0)
        draw_cell_fill(row, col, 19 + trail_player);
    else if (cell > 0)
        draw_cell_number(row, col, cell);
    else if (ctx->colors_enabled && -cell < game_state->players_amount)
        draw_cell_fill(row, col, 20 + (-(int)cell));
}

static void draw_board_row(view_context_t *ctx, const game_state_t *game_state, int y, int content_row) {
    mvaddch(content_row, 0, ACS_VLINE);
    for (int x = 0; x < game_state->width; x++) {
        draw_cell(ctx, game_state, x, y, content_row);
        mvaddch(content_row, 1 + x * CELL_WIDTH + CELL_INNER_WIDTH, ACS_VLINE);
    }
}

static void draw_board(view_context_t *ctx, const game_state_t *game_state) {
    attron(A_BOLD);
    mvprintw(0, 0, "BOARD (%hux%hu)", game_state->width, game_state->height);
    attroff(A_BOLD);

    draw_horizontal_border(1, game_state->width, ACS_ULCORNER, ACS_TTEE, ACS_URCORNER);

    for (int y = 0; y < game_state->height; y++) {
        int content_row   = 2 + y * CELL_HEIGHT;
        int separator_row = content_row + 1;

        draw_board_row(ctx, game_state, y, content_row);

        chtype left  = (y == game_state->height - 1) ? ACS_LLCORNER : ACS_LTEE;
        chtype mid   = (y == game_state->height - 1) ? ACS_BTEE     : ACS_PLUS;
        chtype right = (y == game_state->height - 1) ? ACS_LRCORNER : ACS_RTEE;
        draw_horizontal_border(separator_row, game_state->width, left, mid, right);
    }
}

static void draw_players_panel(const game_state_t *game_state) {
    int row = board_height_chars(game_state->height) + 3;

    attron(A_BOLD);
    mvprintw(row++, 0, "PLAYERS (%u):", game_state->players_amount);
    attroff(A_BOLD);

    for (int i = 0; i < game_state->players_amount; i++) {
        int base_pair = 10 + i;
        attron(COLOR_PAIR(base_pair));
        mvprintw(row, 0, "[%d] %-15s %4u %3u/%-3u (%hu,%hu) ",
                 i,
                 game_state->players[i].players_name,
                 game_state->players[i].score,
                 game_state->players[i].valid_moves,
                 game_state->players[i].invalid_moves,
                 game_state->players[i].x,
                 game_state->players[i].y);
        attroff(COLOR_PAIR(base_pair));

        if (game_state->players[i].blocked) {
            attron(A_BOLD | COLOR_PAIR(4));
            printw("BLOCKED");
            attroff(A_BOLD | COLOR_PAIR(4));
        } else {
            attron(COLOR_PAIR(base_pair));
            printw("OK");
            attroff(COLOR_PAIR(base_pair));
        }
        row++;
    }
}

static void draw_footer(const game_state_t *game_state, bool finished) {
    int row = board_height_chars(game_state->height) + game_state->players_amount + 5;
    mvprintw(row, 0, "=== finished=%s (esperando al master) ===", finished ? "true" : "false");
}

static void draw_too_small_message(const game_state_t *game_state) {
    clear();
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "Terminal too small");
    attroff(A_BOLD | COLOR_PAIR(4));
    mvprintw(2, 0, "Required board size: %dx%d", board_width_chars(game_state->width), board_height_chars(game_state->height));
    mvprintw(3, 0, "Resize the window or reduce the board size.");
}

static void draw_player_summary_row(const game_state_t *game_state, int i, int row) {
    int base_pair = 10 + i;
    attron(COLOR_PAIR(base_pair));
    mvprintw(row, 0, "%-5d %-15s %-7u %-9u %-11u ",
             i,
             game_state->players[i].players_name,
             game_state->players[i].score,
             game_state->players[i].valid_moves,
             game_state->players[i].invalid_moves);
    attroff(COLOR_PAIR(base_pair));

    if (game_state->players[i].blocked) {
        attron(A_BOLD | COLOR_PAIR(4));
        printw("BLOCKED");
        attroff(A_BOLD | COLOR_PAIR(4));
    } else {
        attron(COLOR_PAIR(base_pair));
        printw("OK");
        attroff(COLOR_PAIR(base_pair));
    }
}

static void draw_final_summary(const game_state_t *game_state, int start_row) {
    attron(A_BOLD);
    mvprintw(start_row++, 0, "FINAL SUMMARY");
    mvprintw(start_row++, 0, "idx   name             score   valid     invalid     status");
    attroff(A_BOLD);

    for (int i = 0; i < game_state->players_amount; i++)
        draw_player_summary_row(game_state, i, start_row++);
}

static void print_headless_summary(const game_state_t *game_state) {
    fprintf(stderr, "=== FINAL SUMMARY ===\n");
    fprintf(stderr, "idx   name             score   valid     invalid     status\n");
    for (int i = 0; i < game_state->players_amount; i++) {
        fprintf(stderr, "%-5d %-15s %-7u %-9u %-11u %s\n",
                i,
                game_state->players[i].players_name,
                game_state->players[i].score,
                game_state->players[i].valid_moves,
                game_state->players[i].invalid_moves,
                game_state->players[i].blocked ? "BLOCKED" : "OK");
    }
}

int init_view_context(view_context_t *ctx, unsigned short width, unsigned short height) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->width  = width;
    ctx->height = height;

    ctx->trail = calloc((size_t)width * height, sizeof(uint8_t));
    if (ctx->trail == NULL) {
        perror("calloc trail");
        return -1;
    }

    if (getenv("TERM") == NULL)
        setenv("TERM", "xterm-256color", 1);

    ctx->screen = newterm(NULL, stdout, stdin);
    if (ctx->screen == NULL) {
        ctx->headless = true;
        return 0;
    }

    set_term(ctx->screen);
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if (has_colors()) {
        ctx->colors_enabled = true;
        init_color_pairs();
    }

    return 0;
}

void destroy_view_context(view_context_t *ctx) {
    if (ctx->screen != NULL) {
        endwin();
        delscreen(ctx->screen);
    }
    free(ctx->trail);
    ctx->trail = NULL;
}

void update_player_trail(view_context_t *ctx, const game_state_t *game_state) {
    if (ctx->headless || !ctx->colors_enabled) return;
    for (int i = 0; i < game_state->players_amount; i++) {
        size_t idx = (size_t)game_state->players[i].x * game_state->width + game_state->players[i].y;
        ctx->trail[idx] = (uint8_t)(i + 1);
    }
}

void render_game_frame(view_context_t *ctx, const game_state_t *game_state, bool finished) {
    if (ctx->headless) return;

    if (terminal_too_small(game_state)) {
        draw_too_small_message(game_state);
        refresh();
        return;
    }

    clear();
    draw_board(ctx, game_state);
    draw_players_panel(game_state);
    draw_footer(game_state, finished);
    refresh();
}

void render_final_frame(view_context_t *ctx, const game_state_t *game_state) {
    if (ctx->headless) {
        print_headless_summary(game_state);
        return;
    }

    clear();

    if (terminal_too_small(game_state)) {
        draw_too_small_message(game_state);
        mvprintw(5, 0, "Game finished.");
        refresh();
        return;
    }

    draw_board(ctx, game_state);
    draw_final_summary(game_state, board_height_chars(game_state->height) + 3);
    attron(A_BOLD);
    mvprintw(board_height_chars(game_state->height) + game_state->players_amount + 6, 0, "Game finished.");
    attroff(A_BOLD);
    refresh();
    sleep(5);
}
