// proceso jugador provisorio para compilar el master
#include <master.h>
#include <shared_memory.h>

int main(int argc, char *argv[]) {
     if(argc !=3){
        fprintf(stderr, "Error parametros vista\n");
        exit(1); 
    }

    unsigned short width = atoi(argv[1]); 
    unsigned short height = atoi(argv[2]);

    game_state_t * buf_game = open_game_shm(width,height);

    sync_t * buf_sync = open_shm_sync();

    pid_t pid_player = getpid(); 
    int idx = -1; 

    for(int i=0 ; i<buf_game->players_amount && idx == -1 ; i++){
        if(pid_player == buf_game->players[i].pid){
            idx = i; 
        }
    }

    while(buf_game->ended==0){ //el juego sigue corriendo
        if(sem_wait(&buf_sync->move_processed[idx]) == -1){
            perror("sem wait failed");
            exit(1); 
        }

        sem_wait(&buf_sync->writer_mutex);
        sem_wait(&buf_sync->readers_count_mutex);
        buf_sync->readers_count++; 

        if(buf_sync->readers_count == 1){
            sem_wait(&buf_sync->state_mutex);
        }

        sem_post(&buf_sync->readers_count_mutex);
        sem_post(&buf_sync->writer_mutex);

        unsigned short x = buf_game->players[idx].x;
        unsigned short y = buf_game->players[idx].y;

        sem_wait(&buf_sync->readers_count_mutex);
        buf_sync->readers_count--;

        if(buf_sync->readers_count == 0){
            sem_post(&buf_sync->state_mutex);
        }

        sem_post(&buf_sync->readers_count_mutex);
        
        unsigned char move = rand() % 8;
        write(STDOUT_FILENO, &move, sizeof(unsigned char));

    }

    return 0;


}