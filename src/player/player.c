#include <stdio.h>

#include <ipc/shm.h>
#include <player/runtime.h>


int main(int argc, char *argv[]) {
    unsigned short width, height;
    int idx;

    if (parse_player_args(argc, argv, &width, &height, &idx) == -1)
        return 1;

    game_state_t *buf_game = open_game_shm(width, height);
    sync_t       *buf_sync = open_shm_sync();

    if (idx < 0 || idx >= buf_game->players_amount) {
        fprintf(stderr, "Invalid player index: %d\n", idx);
        close_game_shm(buf_game, width, height);
        close_shm_sync(buf_sync);
        return 1;
    }

    int exit_status = run_player_loop(buf_game, buf_sync, idx);

    if (close_game_shm(buf_game, width, height) == -1) exit_status = 1;
    if (close_shm_sync(buf_sync) == -1)                exit_status = 1;

    return exit_status == 0 ? 0 : 1;
}
