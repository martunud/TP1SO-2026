#include <shared_memory.h>

game_state_t * create_game_shm(unsigned short width, unsigned short height){
    if (width == 0 || height == 0) {
        fprintf(stderr, "Error: width and height must be > 0\n");
        return NULL;
    }
    
    size_t total_size = sizeof(game_state_t) + width * height * sizeof(signed char);
    
    // Limpio la memoria anterior si existe
    shm_unlink("/game_state");
    
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
            x = rand() % width; 
            y = rand() % height;

            aux = checkCoord(game_state, width, x,y);
        }
        game_state ->board[x*width+y] = -i;
        game_state ->players[i].x = x;
        game_state ->players[i].y = y;
    }    
}

static int checkCoord(game_state_t * game_state,unsigned short width, int x, int y){
    return game_state->board[x*width + y] > 0 ;
}

sync_t * create_shm_sync(){
    size_t total_size = sizeof(sync_t);
    
    shm_unlink("/game_sync");
    
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