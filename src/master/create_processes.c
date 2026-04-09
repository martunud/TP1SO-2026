#include <create_processes.h>

void init_view_players(unsigned short width, unsigned short height, unsigned char players_amount, char *player_paths[], char * view_path, pid_t * view_pid, game_state_t * game_state, int player_pipes[][2]){
    if(view_path != NULL){
        *view_pid = view_fork(width, height, view_path);
    }else{
        *view_pid = -1 ;
    }

    // primero creo todos los pipes
    for(int i=0 ; i<players_amount ; i++){
        pipe(player_pipes[i]);
    }
    
    // después hacemos los forks
    for(int i=0 ; i<players_amount ; i++){
        pid_t player_pid = player_fork(width, height, player_paths[i], player_pipes, players_amount, i);
        game_state->players[i].pid = player_pid;
        close(player_pipes[i][1]); 
    }
}

pid_t view_fork(unsigned short width, unsigned short height, char * view_path){
    pid_t pid_view = fork();
if(pid_view == 0) {
    // la vista es el hijo
    char *args[4];
    char buf_w[6], buf_h[6];
    sprintf(buf_w, "%d", width);
    sprintf(buf_h, "%d", height);
    args[0] = view_path;
    args[1] = buf_w;
    args[2] = buf_h;
    args[3] = NULL;
    execv(view_path, args);
    // si execv falla, llegamos acá
    perror("execv vista");
    exit(1);
} else if(pid_view < 0) {
    // fork falló
    perror("fork vista");
    exit(1);
}
// padre sigue
return pid_view; 
}

pid_t player_fork(unsigned short width, unsigned short height, char * player_path, int player_pipes[][2], int num_players, int player_index){
    pid_t pid_player = fork();
    if(pid_player == 0) {
        // el player es el hijo
        // cerrar todos los pipes excepto el propio
        for(int i =0; i < num_players; i++){
            if(i != player_index){
                close(player_pipes[i][0]);
                close(player_pipes[i][1]);
            }
        }

        //usar el pipe propio
        dup2(player_pipes[player_index][1], 1);
        close(player_pipes[player_index][1]);
        close(player_pipes[player_index][0]);
    
        
        char *args[5];
        char buf_w[6], buf_h[6], buf_idx[4];
        sprintf(buf_w, "%d", width);
        sprintf(buf_h, "%d", height);
        sprintf(buf_idx, "%d", player_index);
        args[0] = player_path;
        args[1] = buf_w;
        args[2] = buf_h;
        args[3] = buf_idx;
        args[4] = NULL;
        execv(player_path, args);
        // si execv falla, llegamos acá
        perror("execv player");
        exit(1);
    } else if(pid_player < 0) {
        // fork falló
        perror("fork player");
    }
    // padre sigue
    return pid_player; 
}
