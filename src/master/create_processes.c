#include <create_processes.h>

void init_view_players(unsigned short width, unsigned short height, unsigned char players_amount, char *player_paths[], char * view_path, pid_t * view_pid, pid_t players_pid[], int player_pipes[][2]){
    if(view_path != NULL){
        *view_pid = view_fork(width, height, view_path);
    }else{
        *view_pid = -1 ;
    }

    for(int i=0 ; i<players_amount ; i++){
        pipe(player_pipes[i]);
        pid_t player_pid = player_fork(width, height, player_paths[i], player_pipes[i]);
        players_pid[i]= player_pid;
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
    execve(view_path, args, NULL);
    // si execve falla, llegamos acá
    perror("execve player");
    exit(1);
} else if(pid_view < 0) {
    // fork falló
    perror("fork player");
    exit(1);
}
// padre sigue
return pid_view; 
}

pid_t player_fork(unsigned short width, unsigned short height, char * player_path, int * player_pipe){
    pid_t pid_player = fork();
if(pid_player == 0) {
    // el player es el hijo
    char *args[4];
    char buf_w[6], buf_h[6];
    sprintf(buf_w, "%d", width);
    sprintf(buf_h, "%d", height);
    args[0] = player_path;
    args[1] = buf_w;
    args[2] = buf_h;
    args[3] = NULL;
    dup2(player_pipe[1], 1);
    close(player_pipe[1]);
    close(player_pipe[0]);
    execve(player_path, args, NULL);
    // si execve falla, llegamos acá
    perror("execve vista");
    exit(1);
} else if(pid_player < 0) {
    // fork falló
    perror("fork vista");
}
// padre sigue
return pid_player; 
}