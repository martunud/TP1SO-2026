#include <shared_memory.h>
#include <args.h>
#include <create_processes.h>
#include <sys/wait.h>
#include <signal.h>

int round_robin(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int *player_start, fd_set *read_fds, unsigned int delay, pid_t view_pid);

static void wait_for_children(pid_t view_pid, game_state_t *game_state, int num_players){
    if(view_pid != -1){
        int status;
        waitpid(view_pid, &status, 0);
        if(WIFEXITED(status))
            printf("view exit: %d\n", WEXITSTATUS(status));
        else if(WIFSIGNALED(status))
            printf("view killed by signal: %d\n", WTERMSIG(status));
    }

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
}

static void run_game_loop(game_state_t *game_state, sync_t *sync, int players_pipe[][2], int num_players, int timeout, unsigned int delay, pid_t view_pid){
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

        int result = round_robin(game_state, sync, players_pipe, num_players, &player_start, &read_fds, delay, view_pid);
        if(result == -1){
            game_over = true;
            break;
        }

        if(result == 1){
            last_valid_move = time(NULL);
        }
    }
}

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
        if(sem_wait(&sync->writer_mutex) == -1){
            perror("sem_wait writer_mutex");
            return -1;
        }

        if(sem_wait(&sync->state_mutex) == -1){
            perror("sem_wait state_mutex");
            sem_post(&sync->writer_mutex);
            return -1;
        }

        if(sem_post(&sync->writer_mutex) == -1){
            perror("sem_post writer_mutex");
            sem_post(&sync->state_mutex);
            return -1;
        }

        bool valid = apply_move(game_state, i , move); 

        if(sem_post(&sync->state_mutex) == -1){
            perror("sem_post state_mutex");
            return -1;
        }
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

// determinar y mostrar el ganador 
static void print_winner(game_state_t *game_state, int num_players){
    int winner = 0;
    for(int i = 1; i < num_players; i++){
        player_t *curr = &game_state->players[i];
        player_t *best = &game_state->players[winner];
 
        if(curr->score > best->score){
            winner = i;
        } else if(curr->score == best->score){
            if(curr->valid_moves < best->valid_moves){
                winner = i;
            } else if(curr->valid_moves == best->valid_moves){
                if(curr->invalid_moves < best->invalid_moves){
                    winner = i;
                }
            }
        }
    }
 
    // Chequear empate total
    bool tie = false;
    for(int i = 0; i < num_players; i++){
        if(i == winner) continue;
        player_t *curr = &game_state->players[i];
        player_t *best = &game_state->players[winner];
        if(curr->score == best->score && curr->valid_moves == best->valid_moves && curr->invalid_moves == best->invalid_moves){
            tie = true;
            break;
        }
    }
 
    if(tie){
        printf("Result: TIE\n");
    } else {
        printf("Winner: player[%d] (%s) with score %u\n", winner, game_state->players[winner].players_name, game_state->players[winner].score);
    }
}

int main (int argc, char *argv[]) {
    int exit_status = 0;
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS] ;

    // Ignoramos SIGPIPE para que el master no muera si un jugador cierra su pipe inesperadamente.
    // Sin esto, un write/read sobre un pipe roto envía SIGPIPE y termina el proceso.
    signal(SIGPIPE, SIG_IGN);

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        return 1;
    }

     //validar que el tablero sea suficientemente grande para distribuir jugadores
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

    init_view_players(width, height, (unsigned char)num_players, player, view, &view_pid, game_state , players_pipe);

    run_game_loop(game_state, sync, players_pipe, num_players, timeout, delay, view_pid);

    game_state->ended = true; 

    if(view_pid != -1){
        sem_post(&sync->state_changed); 
        sem_wait(&sync->view_done); 
    }

    for(int i = 0; i < num_players; i++){
        sem_post(&sync->move_processed[i]);
    }

    wait_for_children(view_pid, game_state, num_players);

    print_winner(game_state, num_players);
    
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
