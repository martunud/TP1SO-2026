#include <shared_memory.h>
#include <args.h>
#include <create_processes.h>

int round_robin(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int *player_start, fd_set *read_fds, unsigned int delay){
    for(int offset=0 ; offset < num_players; offset++){
        int i = (*player_start + offset) % num_players; 

        if(game_state->players[i].blocked){
            continue; 
        }
        
        if(!FD_ISSET(players_pipe[i][0], read_fds)){
            continue; 
        }

        unsigned char move; 
        ssize_t n = read(players_pipe[i][0], &move, 1); 

        if(n==0){
            game_state->players[i].blocked = true; 
            *player_start = (i + 1) % num_players;
            
            return 0; 
        }else if(n==-1){
            perror("read pipe"); 
            return -1; 
        }

        bool valid = apply_move(game_state, i , move); 

        sem_post(&sync->state_changed); 
        sem_wait(&sync->view_done); 

        usleep(delay * 1000); 

        sem_post(&sync->move_processed[i]); 

        *player_start = (i + 1) % num_players; 
        return valid ? 1 : 0; 
    }

    return 0; 
}

int main (int argc, char *argv[]) {
    int exit_status = 0;
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS] ;

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        return 1;
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

    init_view_players(width, height, (unsigned char)num_players, player, view, &view_pid, game_state , players_pipe);

    int player_start = 0; 
    time_t last_valid_move = time(NULL); 
    bool game_over = false; 

    while(!game_over){
        fd_set read_fds; 
        int max_fd = -1; 
        FD_ZERO(&read_fds); 

        for(int i=0 ; i<num_players; i++){
            if(!game_state->players[i].blocked){
                FD_SET(players_pipe[i][0], &read_fds);
                if(players_pipe[i][0] > max_fd){
                    max_fd = players_pipe[i][0]; 
                }

            }
        }

        if(max_fd == -1){
            game_over == true; 
            break; 
        }

        time_t elapsed = time(NULL) - last_valid_move; 
        int remaining = timeout - (int) elapsed; 
        if(remaining <= 0){
            game_over = true; 
            break; 
        }

        struct timeval tv; 
        tv.tv_sec = remaining;
        tv.tv_usec = 0; 

        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        if(ready == -1){
            perror("select");
            game_over = true; 
            break;
        }else if(ready == 0){
            game_over = true; 
            break; 
        }

        if(round_robin(game_state, sync, players_pipe, num_players, &player_start, &read_fds, delay) == 1){
            last_valid_move = time(NULL); 
        }    
    }

    game_state->ended = true; 
    sem_post(&sync->state_changed); 
    sem_wait(&sync->view_done); 


    if(close_game_shm(game_state, width, height) == -1){
        exit_status = 1;
    }

    if(close_shm_sync(sync) == -1){
        exit_status = 1;
    }

    return exit_status;
}
