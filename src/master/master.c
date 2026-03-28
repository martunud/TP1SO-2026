#include <master.h>

//sizeof(game_state_t) + width * height * sizeof(signed char)

// -w ancho tablero default y minimo: 10
// -h altura tablero default y minimo: 10
// -d milisegundos que espera el máster cada vez que se imprime el estado default: 200
// -t timeout en segundos para recibir solicitudes de movimientos válidos default: 10
// -s semilla utilizada para la generación del tablero default time(NULL)
// -v ruta del binario de la vista default: sin vista
// -p ruta de los binarios de los jugadores. Mínimo 1, Máximo 9

int parse_args(int argc, char *argv[], unsigned short *width, unsigned short *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players ){
    int opt;
    char *endptr;
    unsigned long val;

    //defaults
    *width = 10;
    *height = 10;
    *delay = 200;
    *timeout = 10;
    *seed = (unsigned int) time(NULL);
    *view = NULL;
    *num_players = 0;

    while((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1){ //distinto a -1 pues getopt() retorna -1 al terminar
        switch(opt){
            case'w':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0' || val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }
                *width = (unsigned short) val;
                break;
            case 'h': 
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0'|| val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }   
                *height = (unsigned short) val;        
                break;
            case 'd':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg); // corregir el msj de error una vez podamos probar la flag view
                    return -1;
                }   
                *delay = (unsigned int) val;        
                break;
            case 't':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg); 
                    return -1;
                }   
                *timeout = (int) val;        
                break;
            case 's':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }
                *seed = (unsigned int) val;
                break;
            case 'v':
                *view = optarg ; //quiero directo el puntero al string
                break;
            case 'p':
                if(*num_players >= MAX_PLAYERS){
                    fprintf(stderr, "Error: At most %d players can be specified using -p.\n", MAX_PLAYERS); 
                    return -1;
                }
                player[*num_players] = optarg; 
                (*num_players)++; 
                break;
            case '?':
                fprintf(stderr, "%s: invalid option -- %c \nUsage: %s [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] [-i] -p player1 player2 ...\n", argv[0], optopt, argv[0]);
                return -1;                
        }
    }
    if (*num_players == 0) {
        fprintf(stderr, "Error: At least one player must be specified using -p.\n");
        return -1;
    }
    
    return 0;
}

int main (int argc, char *argv[]) {
    unsigned short width, height;
    unsigned int delay, seed;
    int timeout, num_players;
    char *view;
    char *player[MAX_PLAYERS];

    int parsed = parse_args(argc, argv, &width, &height, &delay, &timeout, &seed, &view, player, &num_players);

    if(parsed == -1){
        exit(1);
    }
    else{
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
    }
    return 0;
}

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

int checkCoord(game_state_t * game_state,unsigned short width, int x, int y){
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