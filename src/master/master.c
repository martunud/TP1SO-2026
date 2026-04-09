#include <shared_memory.h>
#include <args.h>
#include <create_processes.h>
#include <sys/wait.h>

int round_robin(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int *player_start, fd_set *read_fds, unsigned int delay, pid_t view_pid){

     for(int offset = 0; offset < num_players; offset++){
        int i = (*player_start + offset) % num_players;

        if(game_state->players[i].blocked) continue; 
        if(!FD_ISSET(players_pipe[i][0], read_fds)) continue; 

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

        //writer lock antes de modificar el estado
        sem_wait(&sync->writer_mutex);
        sem_wait(&sync->state_mutex);
        sem_post(&sync->writer_mutex);

        bool valid = apply_move(game_state, i , move); 

        sem_post(&sync->state_mutex);
        //fin de writer lock

        if (valid && view_pid != -1){
        sem_post(&sync->state_changed); 
        sem_wait(&sync->view_done); 
        usleep(delay * 1000);
        }

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
            game_over = true; 
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

        if(round_robin(game_state, sync, players_pipe, num_players, &player_start, &read_fds, delay, view_pid) == 1){
            last_valid_move = time(NULL); 
        }    
    }

    game_state->ended = true; 

    if(view_pid != -1){
        sem_post(&sync->state_changed); 
        sem_wait(&sync->view_done); 
    }

    // waitpid de la vista
    if(view_pid != -1){
        int status;
        waitpid(view_pid, &status, 0);
        if(WIFEXITED(status))
            printf("view exit: %d\n", WEXITSTATUS(status));
        else if(WIFSIGNALED(status))
            printf("view killed by signal: %d\n", WTERMSIG(status));
    }

    // waitpid de cada jugador
    for(int i = 0; i < num_players; i++){
        int status;
        waitpid(game_state->players[i].pid, &status, 0);
        if(WIFEXITED(status))
            printf("player[%d] (%s) exit: %d score: %u\n",
                   i, game_state->players[i].players_name,
                   WEXITSTATUS(status), game_state->players[i].score);
        else if(WIFSIGNALED(status))
            printf("player[%d] (%s) killed by signal: %d score: %u\n",
                   i, game_state->players[i].players_name,
                   WTERMSIG(status), game_state->players[i].score);
    }
    
    // cerrar pipes de jugadores
    for(int i = 0; i < num_players; i++){
        close(players_pipe[i][0]);
    }

    // limpieza de recursos
    destroy_sync(sync, (unsigned char)num_players);
    unlink_game_shm();
    unlink_sync_shm();

    close_game_shm(game_state, width, height);
    close_shm_sync(sync);


    return exit_status;
}
