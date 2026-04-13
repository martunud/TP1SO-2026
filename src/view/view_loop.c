#include <view/view_loop.h>

#include <errno.h>
#include <semaphore.h>
#include <stdio.h>

#include <ipc/sync.h>

static int wait_for_frame(sync_t *sync) {
    while (true) {
        if (sem_wait(&sync->state_changed) == 0) {
            return 0;
        }
        if (errno != EINTR) {
            perror("sem_wait state_changed");
            return -1;
        }
    }
}

static int render_current_frame(view_context_t *ctx, game_state_t *game_state,
                                sync_t *sync, bool *finished) {
    if (reader_enter(sync) == -1) {
        perror("reader_enter");
        return -1;
    }

    update_player_trail(ctx, game_state);
    *finished = game_state->ended;
    render_game_frame(ctx, game_state, *finished);

    if (reader_exit(sync) == -1) {
        perror("reader_exit");
        return -1;
    }

    if (sem_post(&sync->view_done) == -1) {
        perror("sem_post view_done");
        return -1;
    }

    return 0;
}

static int render_final_snapshot(view_context_t *ctx, game_state_t *game_state,
                                 sync_t *sync) {
    if (reader_enter(sync) == -1) {
        perror("reader_enter");
        return -1;
    }

    update_player_trail(ctx, game_state);
    render_final_frame(ctx, game_state);

    if (reader_exit(sync) == -1) {
        perror("reader_exit");
        return -1;
    }

    return 0;
}

int run_view_loop(view_context_t *ctx, game_state_t *game_state, sync_t *sync) {
    while (true) {
        bool finished = false;

        if (wait_for_frame(sync) == -1) {
            return -1;
        }
        if (render_current_frame(ctx, game_state, sync, &finished) == -1) {
            return -1;
        }
        if (finished) {
            return render_final_snapshot(ctx, game_state, sync);
        }
    }
}
