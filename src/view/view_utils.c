#include <view/view_utils.h>

#include <errno.h>
#include <string.h>

static const short PLAYER_TEXT_COLORS[MAX_PLAYERS] = {
    COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE,
    COLOR_WHITE, COLOR_RED, COLOR_BLACK, COLOR_BLACK
};

static const short PLAYER_BG_COLORS[MAX_PLAYERS] = {
    COLOR_CYAN, COLOR_MAGENTA, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE,
    COLOR_WHITE, COLOR_RED, COLOR_BLACK, COLOR_BLACK
};

static void init_color_pairs(void){
    start_color();
    use_default_colors();

    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_WHITE, -1);
    init_pair(4, COLOR_RED, -1);

    for(short i = 0; i < MAX_PLAYERS; i++){
        init_pair((short)(10 + i), PLAYER_TEXT_COLORS[i], -1);
        init_pair((short)(20 + i), COLOR_BLACK, PLAYER_BG_COLORS[i]);
    }
}

static int board_width_chars(unsigned short width){
    return width * CELL_WIDTH + 1;
}

static int board_height_chars(unsigned short height){
    return height * CELL_HEIGHT + 1;
}

static int total_ui_height(const game_state_t *game_state){
    return 1 + 1 + board_height_chars(game_state->height) + 2 + game_state->players_amount + 2;
}

static bool terminal_too_small(view_context_t *ctx, const game_state_t *game_state){
    int rows;
    int cols;

    getmaxyx(stdscr, rows, cols);
    return cols < board_width_chars(game_state->width) || rows < total_ui_height(game_state);
}

static void draw_horizontal_border(int row, unsigned short width, chtype left, chtype mid, chtype right){
    mvaddch(row, 0, left);

    for(int x = 0; x < width; x++){
        for(int i = 0; i < CELL_INNER_WIDTH; i++){
            addch(ACS_HLINE);
        }

        if(x == width - 1){
            addch(right);
        } else {
            addch(mid);
        }
    }
}

static int player_at(const game_state_t *game_state, int x, int y){
    for(int i = 0; i < game_state->players_amount; i++){
        if(game_state->players[i].x == y && game_state->players[i].y == x){
            return i;
        }
    }

    return -1;
}

static void draw_cell_fill(int row, int col, int color_pair){
    attron(COLOR_PAIR(color_pair));
    mvaddch(row, col, ' ');
    for(int i = 0; i < CELL_INNER_WIDTH; i++){
        addch(' ');
    }
    attroff(COLOR_PAIR(color_pair));
}

static void draw_cell_number(int row, int col, signed char value){
    attron(COLOR_PAIR(2));
    mvprintw(row, col, " %2d ", value);
    attroff(COLOR_PAIR(2));
}

static void draw_board(view_context_t *ctx, const game_state_t *game_state){
    mvprintw(0, 0, "TABLERO (%hux%hu)", game_state->width, game_state->height);
    attron(A_BOLD);
    mvprintw(0, 0, "TABLERO (%hux%hu)", game_state->width, game_state->height);
    attroff(A_BOLD);

    draw_horizontal_border(1, game_state->width, ACS_ULCORNER, ACS_TTEE, ACS_URCORNER);

    for(int y = 0; y < game_state->height; y++){
        int content_row = 2 + y * CELL_HEIGHT;
        int separator_row = content_row + 1;

        mvaddch(content_row, 0, ACS_VLINE);

        for(int x = 0; x < game_state->width; x++){
            int col = 1 + x * CELL_WIDTH;
            int current_player = player_at(game_state, x, y);
            int trail_player = ctx->trail[y * game_state->width + x];

            if(ctx->colors_enabled && current_player >= 0){
                draw_cell_fill(content_row, col, 20 + current_player);
            } else if(ctx->colors_enabled && trail_player > 0){
                draw_cell_fill(content_row, col, 19 + trail_player);
            } else {
                draw_cell_number(content_row, col, game_state->board[y * game_state->width + x]);
            }

            mvaddch(content_row, col + CELL_INNER_WIDTH, ACS_VLINE);
        }

        if(y == game_state->height - 1){
            draw_horizontal_border(separator_row, game_state->width, ACS_LLCORNER, ACS_BTEE, ACS_LRCORNER);
        } else {
            draw_horizontal_border(separator_row, game_state->width, ACS_LTEE, ACS_PLUS, ACS_RTEE);
        }
    }
}

static void draw_players_panel(const game_state_t *game_state){
    int row = board_height_chars(game_state->height) + 3;

    attron(A_BOLD);
    mvprintw(row++, 0, "JUGADORES (%u):", game_state->players_amount);
    attroff(A_BOLD);

    for(int i = 0; i < game_state->players_amount; i++){
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

        if(game_state->players[i].blocked){
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

static void draw_footer(const game_state_t *game_state, bool finished){
    int row = board_height_chars(game_state->height) + game_state->players_amount + 5;
    mvprintw(row, 0, "=== finished=%s (esperando al master) ===", finished ? "true" : "false");
}

static void draw_too_small_message(const game_state_t *game_state){
    clear();
    attron(A_BOLD | COLOR_PAIR(4));
    mvprintw(0, 0, "Terminal demasiado pequena");
    attroff(A_BOLD | COLOR_PAIR(4));
    mvprintw(2, 0, "Tablero requerido: %dx%d", board_width_chars(game_state->width), board_height_chars(game_state->height));
    mvprintw(3, 0, "Agrandar la ventana o reducir el tablero.");
}

static void draw_final_summary(const game_state_t *game_state, int start_row){
    attron(A_BOLD);
    mvprintw(start_row++, 0, "RESUMEN FINAL");
    mvprintw(start_row++, 0, "idx   nombre           score   validas   invalidas   estado");
    attroff(A_BOLD);

    for(int i = 0; i < game_state->players_amount; i++){
        int base_pair = 10 + i;
        attron(COLOR_PAIR(base_pair));
        mvprintw(start_row, 0, "%-5d %-15s %-7u %-9u %-11u ",
                 i,
                 game_state->players[i].players_name,
                 game_state->players[i].score,
                 game_state->players[i].valid_moves,
                 game_state->players[i].invalid_moves);
        attroff(COLOR_PAIR(base_pair));

        if(game_state->players[i].blocked){
            attron(A_BOLD | COLOR_PAIR(4));
            printw("BLOCKED");
            attroff(A_BOLD | COLOR_PAIR(4));
        } else {
            attron(COLOR_PAIR(base_pair));
            printw("OK");
            attroff(COLOR_PAIR(base_pair));
        }

        start_row++;
    }
}

static void print_headless_summary(const game_state_t *game_state){
    fprintf(stderr, "=== RESUMEN FINAL ===\n");
    fprintf(stderr, "idx   nombre           score   validas   invalidas   estado\n");

    for(int i = 0; i < game_state->players_amount; i++){
        fprintf(stderr, "%-5d %-15s %-7u %-9u %-11u %s\n",
                i,
                game_state->players[i].players_name,
                game_state->players[i].score,
                game_state->players[i].valid_moves,
                game_state->players[i].invalid_moves,
                game_state->players[i].blocked ? "BLOCKED" : "OK");
    }
}

int view_reader_enter(sync_t *sync){
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

int view_reader_exit(sync_t *sync){
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

int init_view_context(view_context_t *ctx, unsigned short width, unsigned short height){
    memset(ctx, 0, sizeof(*ctx));
    ctx->width = width;
    ctx->height = height;

    ctx->trail = calloc((size_t)width * height, sizeof(uint8_t));
    if(ctx->trail == NULL){
        perror("calloc trail");
        return -1;
    }

    if(getenv("TERM") == NULL){
        setenv("TERM", "xterm-256color", 1);
    }

    ctx->screen = newterm(NULL, stdout, stdin);
    if(ctx->screen == NULL){
        ctx->headless = true;
        return 0;
    }

    set_term(ctx->screen);
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    if(has_colors()){
        ctx->colors_enabled = true;
        init_color_pairs();
    }

    return 0;
}

void destroy_view_context(view_context_t *ctx){
    if(ctx->screen != NULL){
        endwin();
        delscreen(ctx->screen);
    }

    free(ctx->trail);
    ctx->trail = NULL;
}

void update_player_trail(view_context_t *ctx, const game_state_t *game_state){
    if(ctx->headless || !ctx->colors_enabled){
        return;
    }

    for(int i = 0; i < game_state->players_amount; i++){
        size_t idx = (size_t)game_state->players[i].x * game_state->width + game_state->players[i].y;
        ctx->trail[idx] = (uint8_t)(i + 1);
    }
}

void render_game_frame(view_context_t *ctx, const game_state_t *game_state, bool finished){
    if(ctx->headless){
        return;
    }

    if(terminal_too_small(ctx, game_state)){
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

void render_final_frame(view_context_t *ctx, const game_state_t *game_state){
    if(ctx->headless){
        print_headless_summary(game_state);
        return;
    }

    clear();

    if(terminal_too_small(ctx, game_state)){
        draw_too_small_message(game_state);
        mvprintw(5, 0, "Juego terminado.");
        refresh();
        return;
    }

    draw_board(ctx, game_state);
    draw_final_summary(game_state, board_height_chars(game_state->height) + 3);
    attron(A_BOLD);
    mvprintw(board_height_chars(game_state->height) + game_state->players_amount + 6, 0,
             "Juego terminado. Presiona cualquier tecla para salir...");
    attroff(A_BOLD);
    refresh();
    getch();
}
