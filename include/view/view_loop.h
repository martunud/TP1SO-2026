#ifndef VIEW_LOOP_H
#define VIEW_LOOP_H

#include <game_state.h>
#include <game_sync.h>
#include <view/view_utils.h>

int run_view_loop(view_context_t *ctx, game_state_t *game_state, sync_t *sync);

#endif
