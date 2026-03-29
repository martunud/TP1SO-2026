// proceso vista provisorio para compilar el master
#include <master.h>

int main(int argc, char *argv[]) {
    if(argc !=3){
        fprintf(stderr, "Error parametros vista\n");
        exit(1); 
    }

    unsigned short width = atoi(argv[1]); 
    unsigned short height = atoi(argv[2]);

    int shm_fd = shm_open("/game_state", O_RDONLY, 0);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    size_t total_size = sizeof(game_state_t) + width * height * sizeof(signed char);

    game_state_t * buf = (game_state_t *) mmap(NULL, total_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);

    shm_fd = shm_open("/game_sync",  O_RDWR, 0644);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    total_size = sizeof(sync_t);

    sync_t * bufSync = (sync_t *) mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(bufSync == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);
    
    char symbols[9] = {'@', '#', '$', '%', '^', '&', '*', '(', '!'};
    //la vista tiene que correr mientras el juego siga en curso
    while(buf->ended == 0){
        int aux = sem_wait(&bufSync->state_changed); 
        if(aux == -1){
            perror("sem_wait state changed");
            exit(1);
        }

        //imprimir tablero
        for(int i=0; i< height; i++){
            for(int j=0; j< width; j++){
                signed char cell = buf -> board[i * width + j];
                if(cell > 0){
                    printf("%d", cell);
                }else{
                    printf("%c", symbols[-cell]);
                }
            }
            printf("\n");
        }
        
        //imprimir info jugadores
        for(int a=0; a< buf->players_amount; a++){
            printf("(%c) %s | puntaje: %u | válidos: %u | inválidos: %u | pos: (%hu, %hu) | bloqueado: %s\n",
            symbols[a],
            buf->players[a].players_name,
            buf->players[a].score,
            buf->players[a].valid_moves,
            buf->players[a].invalid_moves,
            buf->players[a].x,
            buf->players[a].y,
            buf->players[a].blocked ? "si" : "no"
            );
        }
        sem_post(&bufSync->view_done);
    }
    int winner = 0;  // índice del ganador
    for(int i = 0; i < buf->players_amount; i++){
        if(buf->players[i].score > buf->players[winner].score){
            winner = i;
        }
    }
    printf("The winner is: %s (%c)\n", buf->players[winner].players_name, symbols[winner]);
    return 0;
}

