#include <shared_memory.h>
#include <errno.h>
#include <string.h>

static size_t game_state_size(unsigned short width, unsigned short height){
    return sizeof(game_state_t) + width * height * sizeof(signed char);
}

static int checkCoord(game_state_t * game_state,unsigned short width, int x, int y){
    return game_state->board[x*width + y] > 0 ;
}

game_state_t * create_game_shm(unsigned short width, unsigned short height){
    if (width == 0 || height == 0) {
        fprintf(stderr, "Error: width and height must be > 0\n");
        return NULL;
    }
    
    size_t total_size = game_state_size(width, height);
    
    // Limpio la memoria anterior si existe
    unlink_game_shm();
    
    int shm_fd = shm_open("/game_state", O_CREAT | O_RDWR, 0644);
    if(shm_fd < 0){
        perror("shm_open");
        return NULL;
    }

    if(ftruncate(shm_fd, total_size) < 0){
        perror("ftruncate");
        close(shm_fd);
        shm_unlink("/game_state");
        return NULL;
    }

    game_state_t * buf = (game_state_t *) mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        shm_unlink("/game_state");
        return NULL;
    }

    close(shm_fd);  // El fd ya no se necesita después de mmap
    memset(buf, 0, total_size);  // Inicializo a ceros
    
    return buf;
}

void init_game_state(game_state_t * game_state , unsigned short width, unsigned short height, unsigned char players_amount, player_t players[9], unsigned int * seed){
    game_state ->width = width; 
    game_state ->height = height; 
    game_state ->players_amount = players_amount; 

    for(int i=0 ; i<players_amount; i++){
        game_state ->players[i] = players[i]; 
    }

    srand(*seed); //va a ser time(NULL) si no se paso el arg de la seed y si no la que se paso por linea de comando

    for(int i=0 ; i<height ; i++){
        for(int j=0 ; j<width ; j++){
            game_state->board[i*width + j]= rand() % 9 + 1 ; 
        }
    }

    //falta inicializar players en el tablero
    for(int i = 0 ; i<players_amount; i++){
        int aux=0;
        int x, y; 
        while(aux == 0){
            x = rand() % height; 
            y = rand() % width;

            aux = checkCoord(game_state, width, x,y);
        }
        game_state ->board[x*width+y] = -i;
        game_state ->players[i].x = x;
        game_state ->players[i].y = y;
    }    
}
// inicializa una struct player_t por cada jugador que participe, sirve para hacer el arreglo de structs de players que pide init_game_state
void init_players(player_t players[], char *player_paths[], int num_players){
    for(int i = 0; i < num_players; i++){
    strncpy(players[i].players_name, player_paths[i], 16);
    players[i].players_name[15] = '\0';
    players[i].score = 0;
    players[i].valid_moves = 0;
    players[i].invalid_moves = 0;
    players[i].blocked = false;
    }
}//este no deberia ser con firma player_t * players[] ? para que se modifique el struct ?

sync_t * create_shm_sync(){
    size_t total_size = sizeof(sync_t);
    
    unlink_sync_shm();
    
    int shm_fd = shm_open("/game_sync", O_CREAT | O_RDWR, 0644);
    if(shm_fd < 0){
        perror("shm_open");
        return NULL;
    }

    if(ftruncate(shm_fd, total_size) < 0){
        perror("ftruncate");
        close(shm_fd);
        shm_unlink("/game_sync");
        return NULL;
    }

    sync_t * buf = (sync_t *) mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        shm_unlink("/game_sync");
        return NULL;
    }

    close(shm_fd);
    memset(buf, 0, total_size);
    
    return buf;
}

// inicializa los semaforos de sync_t
void init_sync(sync_t *sync, unsigned char players_amount){
    int sem = sem_init(&sync->state_changed, 1, 0); 
    if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }

    sem = sem_init(&sync->view_done, 1, 0); 
    if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }

    sem = sem_init(&sync->writer_mutex, 1, 1); 
    if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }

    sem = sem_init(&sync->state_mutex, 1, 1); 
    if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }

    sem = sem_init(&sync->readers_count_mutex, 1, 1); 
    if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }

    for(int i=0 ; i<players_amount ; i++){
        sem = sem_init(&sync->move_processed[i], 1, 1); 
        if(sem == -1){
        perror("sem_init"); 
        exit(1);
    }
    }
}

game_state_t * open_game_shm(unsigned short width, unsigned short height){
    int shm_fd = shm_open("/game_state", O_RDONLY, 0);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    size_t total_size = game_state_size(width, height);

    game_state_t * buf = (game_state_t *) mmap(NULL, total_size, PROT_READ, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);

    return buf; 
}

sync_t * open_shm_sync(){
    int shm_fd = shm_open("/game_sync",  O_RDWR, 0644);
    if(shm_fd < 0){
        perror("shm_open");
        exit(1);
    }

    size_t total_size = sizeof(sync_t);

    sync_t * buf = (sync_t *) mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(buf == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        exit(1);
    }

    close(shm_fd);

    return buf; 
}

int close_game_shm(game_state_t *game_state, unsigned short width, unsigned short height){
    if(game_state == NULL){
        return 0;
    }

    if(munmap(game_state, game_state_size(width, height)) == -1){
        perror("munmap game_state");
        return -1;
    }

    return 0;
}

int close_shm_sync(sync_t *sync){
    if(sync == NULL){
        return 0;
    }

    if(munmap(sync, sizeof(sync_t)) == -1){
        perror("munmap game_sync");
        return -1;
    }

    return 0;
}

int destroy_sync(sync_t *sync, unsigned char players_amount){
    if(sync == NULL){
        return 0;
    }

    if(sem_destroy(&sync->state_changed) == -1){
        perror("sem_destroy state_changed");
        return -1;
    }

    if(sem_destroy(&sync->view_done) == -1){
        perror("sem_destroy view_done");
        return -1;
    }

    if(sem_destroy(&sync->writer_mutex) == -1){
        perror("sem_destroy writer_mutex");
        return -1;
    }

    if(sem_destroy(&sync->state_mutex) == -1){
        perror("sem_destroy state_mutex");
        return -1;
    }

    if(sem_destroy(&sync->readers_count_mutex) == -1){
        perror("sem_destroy readers_count_mutex");
        return -1;
    }

    for(int i = 0; i < players_amount; i++){
        if(sem_destroy(&sync->move_processed[i]) == -1){
            perror("sem_destroy move_processed");
            return -1;
        }
    }

    return 0;
}

int unlink_game_shm(void){
    if(shm_unlink("/game_state") == -1 && errno != ENOENT){
        perror("shm_unlink /game_state");
        return -1;
    }

    return 0;
}

int unlink_sync_shm(void){
    if(shm_unlink("/game_sync") == -1 && errno != ENOENT){
        perror("shm_unlink /game_sync");
        return -1;
    }

    return 0;
}


