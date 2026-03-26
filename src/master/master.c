// proceso master 

#include "master.h"

/*
/*  -w ancho tablero default y minimo: 10
/* -h altura tablero default y minimo: 10
/* -d milisegundos que espera el máster cada vez que se imprime el estado default: 200
/* -t timeout en segundos para recibir solicitudes de movimientos válidos default: 10
/* -s semilla utilizada para la generación del tablero default time(NULL)
/* -v ruta del binario de la vista default: sin vista
/* -p ruta de los binarios de los jugadores. Mínimo 1, Máximo 9
*/

int parse_args(int argc, char *argv[], const char *optstring, unsigned int *width, unsigned int *height, unsigned int *delay, int *timeout, unsigned int *seed, char **view, char *player[MAX_PLAYERS], int *num_players ){
    int opt;
    char *endptr;
    unsigned long val;

    while((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1){ //distinto a -1 pues getopt() retorna 1 al terminar
        switch(opt){
            case'w':
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0' || val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }
                *width = (unsigned int) val;
                break;
            case 'h': 
                val = strtoul(optarg, &endptr, 10);
                if (*endptr != '\0'|| val < 10) {
                    fprintf(stderr, "Invalid integer %s\n", optarg);
                    return -1;
                }   
                *height = (unsigned int) val;        
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
                fprintf(stderr, "./ChompChamps: invalid option -- %c \nUsage: ./ChompChamps [-w width] [-h height] [-d delay] [-s seed] [-v view] [-t timeout] [-i] -p player1 player2 ...\n", optopt);
                return -1;                 
        }
    }
    return 0;
}