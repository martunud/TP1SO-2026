#include <shared_memory.h>
#include <args.h>
#include <create_processes.h>

int main (int argc, char *argv[]) {
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS] ;

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        exit(1);
    }

    printf("width: %hu\nheight: %hu\ndelay: %u\ntimeout: %d\nseed: %u\n", width, height, delay, timeout, seed);
    if(view == NULL){
        printf("view: -\n");
    }else{
        printf("view: %s\n", view);
    }
    printf("num_players: %d\n", num_players);
    for(int i=0; i<num_players; i++){
        printf("  %s\n", player[i]);
    }

    game_state_t *game_state = create_game_shm(width, height);
    if(game_state == NULL){
        exit(1);
    }

    sync_t *sync = create_shm_sync();
    if(sync == NULL){
     exit(1);
    }

    init_sync(sync, num_players);

    int players_pipe[num_players][2]; 

    player_t players[MAX_PLAYERS];
    init_players(players, player, num_players);
    init_game_state(game_state, width, height, (unsigned char)num_players, players, &seed);

    pid_t players_pid[MAX_PLAYERS];
    pid_t view_pid;

    init_view_players(width, height, (unsigned char)num_players, player, view, &view_pid, players_pid, players_pipe);
    return 0;
}
