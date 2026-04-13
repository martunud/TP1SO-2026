#ifndef PLAYER_STRATEGY_H
#define PLAYER_STRATEGY_H

#include <game_state.h>

unsigned char pick_direction(const game_state_t *game_state, unsigned short x,
                             unsigned short y);

#endif
