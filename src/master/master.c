#include <master/args.h>
#include <master/finalize.h>
#include <master/game.h>
#include <master/game_loop.h>
#include <master/results.h>

#include <ipc/proc.h>
#include <ipc/shm.h>
#include <ipc/sync.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int main (int argc, char *argv[]) {
    int exit_status = 0;
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS] ;

    signal(SIGPIPE, SIG_IGN);

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        return 1;
    }

    if(height < (unsigned short)num_players){
        fprintf(stderr, "Error: height (%d) must be >= number of players (%d)\n", height, num_players);
        return 1;
    }

    game_state_t *game_state = create_game_shm(width, height);
    if(game_state == NULL){
        return 1;
    }

    sync_t *sync = create_shm_sync();
    if(sync == NULL){
        close_game_shm(game_state, width, height);
        return 1;
    }

    init_sync(sync, num_players);

    int players_pipe[num_players][2]; 

    player_t players[MAX_PLAYERS];
    init_players(players, player, num_players);
    init_game_state(game_state, width, height, (unsigned char)num_players, players, &seed);

    pid_t view_pid;

    if (init_view_players(width, height, (unsigned char)num_players, player, view, &view_pid,       
                                        game_state, players_pipe) == -1) {
        cleanup_master_resources(game_state, sync, width, height, (unsigned char)num_players);
        return 1;
    }

    run_game_loop(game_state, sync, players_pipe, num_players, timeout, delay, view_pid);

    if (set_game_ended(game_state, sync) == -1) {
        exit_status = 1;
    }

    if (notify_view_shutdown(sync, view_pid) == -1) {
        exit_status = 1;
    }

    if (wake_players(sync, num_players) == -1) {
        exit_status = 1;
    }

    wait_for_children(view_pid, game_state, num_players);

    print_winner(game_state, num_players);
    close_player_pipes(players_pipe, num_players);

    if (cleanup_master_resources(game_state, sync, width, height,(unsigned char)num_players) == -1) {
        exit_status = 1;
    }

    return exit_status;
}
