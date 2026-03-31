// proceso vista provisorio para compilar el master
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

    //close(shm_fd); quedo de un merge con conflict ver que onda
    
    char symbols[9] = {'@', '#', '$', '%', '^', '&', '*', '(', '!'};
    sync_t * buf_sync = open_shm_sync();

    //la vista tiene que correr mientras el juego siga en curso
    while(buf_game->ended == 0){
        int aux = sem_wait(&buf_sync->state_changed); 
        if(aux == -1){
            perror("sem_wait state changed");
            exit(1);
        }

        //imprimir tablero
        for(int i=0; i< height; i++){
            for(int j=0; j< width; j++){
                signed char cell = buf_game -> board[i * width + j];
                if(cell > 0){
                    printf("%d", cell);
                }else{
                    printf("%c", symbols[-cell]);
                }
            }
            printf("\n");
        }
        
        //imprimir info jugadores
        for(int a=0; a< buf_game->players_amount; a++){
            printf("(%c) %s | puntaje: %u | válidos: %u | inválidos: %u | pos: (%hu, %hu) | bloqueado: %s\n",
            symbols[a],
            buf_game->players[a].players_name,
            buf_game->players[a].score,
            buf_game->players[a].valid_moves,
            buf_game->players[a].invalid_moves,
            buf_game->players[a].x,
            buf_game->players[a].y,
            buf_game->players[a].blocked ? "si" : "no"
            );
        }
        sem_post(&buf_sync->view_done);
    }
    int winner = 0;  // índice del ganador
    for(int i = 0; i < buf_game->players_amount; i++){
        if(buf_game->players[i].score > buf_game->players[winner].score){
            winner = i;
        }
    }
    printf("The winner is: %s (%c)\n", buf_game->players[winner].players_name, symbols[winner]);
    return 0;
}

